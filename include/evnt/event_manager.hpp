#pragma once

#include "signal.hpp"
#include <memory>
#include <atomic>
#include <functional>
#include <type_traits>
#include <cassert>
#include <utility>
#include <mutex>

namespace evnt
{
class event_info
{
    inline static std::size_t dynamic_type_index_()
    {
        static std::atomic_size_t index = 0;
        return index++;
    }

public:
    template <class event_type>
    inline static std::size_t type_index()
    {
        static const std::size_t index = dynamic_type_index_();
        return index;
    }
};

//-------

class event_manager;
class async_event_queue;
class event_dispatcher;

class event_listener_base
{
protected:
    inline ~event_listener_base()
    {
        event_manager_ = nullptr;
    }

    inline event_manager* evt_manager() { return event_manager_; }

    inline void invalidate()
    {
        assert(event_manager_);
        if (--counter_ == 0)
            event_manager_ = nullptr;
    }

    template <class event_type>
    inline void disconnect_from_event(std::size_t connection);

private:
    friend event_manager;

    inline void set_event_manager(event_manager& evt_manager)
    {
        assert(!event_manager_ || event_manager_ == &evt_manager);
        event_manager_ = &evt_manager;
        ++counter_;
    }

private:
    event_manager* event_manager_ = nullptr;
    std::atomic_uint16_t counter_ = 0;
};

template <class... event_types>
class event_listener;

template <class event_type>
class event_listener<event_type> : public event_listener_base
{
public:
    ~event_listener()
    {
        disconnect<event_type>();
    }

    template <class evt_type>
    requires std::is_same_v<evt_type, event_type>
    inline void disconnect() { this->event_listener_base::template disconnect_from_event<event_type>(connection_); }

    void disconnect_all() { disconnect<event_type>(); }

protected:
    inline event_listener<event_type>* as_listener(const event_type*) { return this; }

private:
    friend event_manager;

    void set_connection(std::size_t connection)
    {
        connection_ = connection;
    }

    std::size_t connection_;
};

template <class event_type, class... event_types>
class event_listener<event_type, event_types...> : public event_listener<event_types...>
{
public:
    ~event_listener()
    {
        disconnect<event_type>();
    }

    template <class evt_type>
    requires std::is_same_v<evt_type, event_type>
    inline void disconnect() { this->event_listener<event_types...>::template disconnect_from_event<event_type>(connection_); }

    void disconnect_all() { disconnect<event_type>(); this->event_listener<event_types...>::disconnect_all(); }

protected:
    using event_listener<event_types...>::as_listener;
    inline event_listener<event_type, event_types...>* as_listener(const event_type*) { return this; }

private:
    friend event_manager;

    void set_connection(std::size_t connection)
    {
        connection_ = connection;
    }

    std::size_t connection_;
};

class event_manager
{
private:
    class event_signal_interface
    {
    public:
        virtual ~event_signal_interface() {}
    };
    using event_signal_interface_uptr = std::unique_ptr<event_signal_interface>;

    template <class event_type>
    class event_signal : public event_signal_interface
    {
        using evt_signal = signal<void(event_type&)>;

    public:
        using listener_function = typename evt_signal::CbFunction;

        virtual ~event_signal() {}

        template <class evt_listener>
        void connect(evt_listener& listener)
        {
            void(evt_listener::*receive)(event_type&) = &evt_listener::receive;
            listener_function function = std::bind(receive, &listener, std::placeholders::_1);
            std::size_t connection = signal_.connect(std::move(function));
            listener.as_listener(static_cast<const event_type*>(nullptr))->set_connection(connection);
        }

        inline void connect(listener_function&& listener)
        {
            signal_.connect(listener);
        }

        inline void disconnect(std::size_t connection)
        {
            signal_.disconnect(connection);
        }

        inline void emit(event_type& event)
        {
            signal_.emit(event);
        }

    private:
         evt_signal signal_;
    };

public:
    template <class event_type>
    using receiver_function = typename event_signal<event_type>::listener_function;

    event_manager() {}

    event_manager(const event_manager&) = delete;
    event_manager& operator=(const event_manager&) = delete;

    ~event_manager();

    template <class event_type, class receiver_type>
    inline void connect(receiver_type& listener)
    {
        get_or_create_event_signal_<event_type>().connect(listener);
        listener.set_event_manager(*this);
    }

    template <class event_type>
    inline void connect(receiver_function<event_type>&& listener)
    {
        get_or_create_event_signal_<event_type>().connect(std::move(listener));
    }

    void connect(event_dispatcher& dispatcher);

    template <class event_type>
    inline void disconnect(std::size_t connection)
    {
        event_signal_<event_type>().disconnect(connection);
    }

    void disconnect(event_dispatcher& dispatcher);

public:
    template <class event_type>
    inline void emit(event_type& event)
    {
        std::size_t index = event_info::type_index<event_type>();
        if (index < event_signals_.size())
        {
            event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
            if (event_signal_uptr)
                static_cast<event_signal<event_type>*>(event_signal_uptr.get())->emit(event);
        }
        emit_to_dispatchers_(event);
    }

    template <class event_type>
    inline void emit(event_type&& event)
    {
        event_type evt = std::move(event);
        emit<event_type>(std::ref(evt));
    }

    template <class event_type>
    inline void emit(std::vector<event_type>& events)
    {
        std::size_t index = event_info::type_index<event_type>();
        if (index < event_signals_.size())
        {
            event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
            if (event_signal_uptr)
            {
                event_signal<event_type>* e_signal = static_cast<event_signal<event_type>*>(event_signal_uptr.get());
                for (event_type& event : events)
                    e_signal->emit(event);
            }
        }
    }

    void emit(async_event_queue& event_queue);

    void reserve(std::size_t number_of_event_types);

private:
    template <class event_type>
    inline event_signal<event_type>& event_signal_()
    {
        return *static_cast<event_signal<event_type>*>(event_signals_[event_info::type_index<event_type>()].get());
    }

    template <class event_type>
    inline event_signal<event_type>& get_or_create_event_signal_()
    {
        std::size_t index = event_info::type_index<event_type>();
        if (index >= event_signals_.size())
            event_signals_.resize(index + 1);

        event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
        if (!event_signal_uptr)
        {
            event_signal_interface_uptr n_event = std::make_unique<event_signal<event_type>>();
            event_signal_uptr = std::move(n_event);
        }

        return *static_cast<event_signal<event_type>*>(event_signal_uptr.get());
    }

    template <class event_type>
    void emit_to_dispatchers_(event_type& event);

private:
    std::vector<event_signal_interface_uptr> event_signals_;
    std::vector<event_dispatcher*> event_dispatchers_;
    std::mutex mutex_;
};

class async_event_queue
{
private:
    class async_event_queue_interface
    {
    public:
        virtual ~async_event_queue_interface();
        virtual void emit(event_manager& evt_manager) = 0;
        virtual void sync() = 0;
    };
    using async_event_queue_interface_uptr = std::unique_ptr<async_event_queue_interface>;

    template <class event_type>
    class tmpl_async_event_queue : public async_event_queue_interface
    {
    public:
        virtual ~tmpl_async_event_queue() {}

        void reserve(std::size_t capacity)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.reserve(capacity);
        }

        const std::vector<event_type>& events() const { return events_; }
        std::vector<event_type>& events() { return events_; }

        void push(event_type&& event)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.push_back(std::move(event));
        }

        virtual void sync() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.swap(pending_events_);
            pending_events_.clear();
            pending_events_.reserve(events_.capacity());
        }

        virtual void emit(event_manager& evt_manager) override
        {
            evt_manager.emit(events());
        }

    private:
        std::vector<event_type> events_;
        std::vector<event_type> pending_events_;
        std::mutex mutex_;
    };

public:
    template <class event_type>
    inline const std::vector<event_type>& events()
    {
        return get_or_create_event_queue<event_type>().events();
    }

    template <class event_type>
    inline void push(event_type&& event)
    {
        get_or_create_event_queue<event_type>().push(std::move(event));
    }

    template <class event_type>
    void reserve(std::size_t capacity)
    {
        get_or_create_event_queue<event_type>().reserve(capacity);
    }

    void sync();

private:
    friend class event_manager;

    void emit(event_manager& evt_manager);

    template <class event_type>
    inline tmpl_async_event_queue<event_type>& get_or_create_event_queue()
    {
        std::size_t index = event_info::type_index<event_type>();
        if (index >= event_queues_.size())
            event_queues_.resize(index + 1);

        async_event_queue_interface_uptr& async_event_queue_uptr = event_queues_[index];
        if (!async_event_queue_uptr)
        {
            async_event_queue_interface_uptr n_queue = std::make_unique<tmpl_async_event_queue<event_type>>();
            async_event_queue_uptr = std::move(n_queue);
        }

        return *static_cast<tmpl_async_event_queue<event_type>*>(async_event_queue_uptr.get());
    }

private:
    std::vector<async_event_queue_interface_uptr> event_queues_;
};

class event_dispatcher
{
public:
    ~event_dispatcher();

    template <class event_type, class receiver_type>
    inline void connect(receiver_type& listener)
    {
        event_manager_.connect<event_type>(listener);
    }

    template <class event_type>
    inline void connect(event_manager::receiver_function<event_type>&& listener)
    {
        event_manager_.connect(std::move(listener));
    }

    template <class event_type>
    inline void disconnect(std::size_t connection)
    {
        event_manager_.disconnect<event_type>(connection);
    }

    void dispatch();

private:
    friend class event_manager;

    void set_parent_event_manager(event_manager& evt_manager);

    void set_parent_event_manager(std::nullptr_t);

    template <class event_type>
    inline void push_event(event_type& event)
    {
        event_queue_.push(event_type(event));
    }

private:
    event_manager* parent_event_manager_ = nullptr;
    async_event_queue event_queue_;
    event_manager event_manager_;
};

//////////

template <class event_type>
inline void event_listener_base::disconnect_from_event(std::size_t connection)
{
    event_manager* evt_manager = this->evt_manager();
    if (evt_manager)
    {
        this->invalidate();
        evt_manager->template disconnect<event_type>(connection);
    }
}

template <class event_type>
void event_manager::emit_to_dispatchers_(event_type& event)
{
    std::lock_guard lock(mutex_);
    for (event_dispatcher* dispatcher : event_dispatchers_)
    {
        assert(dispatcher);
        dispatcher->push_event<event_type>(event);
    }
}

}
