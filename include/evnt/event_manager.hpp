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
class Events
{
    inline static std::size_t dynamic_type_index_()
    {
        static std::atomic_size_t index = 0;
        return index++;
    }

public:
    template <class Event>
    inline static std::size_t event_type_index()
    {
        static const std::size_t index = dynamic_type_index_();
        return index;
    }
};

//-------

class Event_manager;
class Async_event_queue;
class Event_dispatcher;

class Event_listener_base
{
protected:
    inline Event_manager* event_manager () { return event_manager_; }

    inline void invalidate ()
    {
        assert(event_manager_);
        event_manager_ = nullptr;
    }

    template <class Event>
    inline void disconnect_from_event (std::size_t connection);

private:
    friend Event_manager;

    inline void set_event_manager (Event_manager& event_manager)
    {
        assert(!event_manager_ || event_manager_ == &event_manager);
        event_manager_ = &event_manager;
    }

    Event_manager* event_manager_ = nullptr;
};

template <class... Events>
class Event_listener;

template <class Event>
class Event_listener<Event> : public Event_listener_base
{
public:
    ~Event_listener ()
    {
        disconnect();
    }

    inline void disconnect () { this->Event_listener_base::template disconnect_from_event<Event>(connection_); }

protected:
    inline Event_listener<Event>* as_listener (const Event*) { return this; }

private:
    friend Event_manager;

    void set_connection (std::size_t connection)
    {
        connection_ = connection;
    }

    std::size_t connection_;
};

template <class Event, class... Events>
class Event_listener<Event, Events...> : public Event_listener<Events...>
{
public:
    ~Event_listener ()
    {
        disconnect();
    }

    inline void disconnect () { this->Event_listener<Events...>::template disconnect_from_event<Event>(connection_); }

protected:
    using Event_listener<Events...>::as_listener;
    inline Event_listener<Event, Events...>* as_listener (const Event*) { return this; }

private:
    friend Event_manager;

    void set_connection (std::size_t connection)
    {
        connection_ = connection;
    }

    std::size_t connection_;
};

class Event_manager
{
private:
    class Event_signal_interface
    {
    public:
        virtual ~Event_signal_interface () {}
    };
    using Event_signal_interface_uptr = std::unique_ptr<Event_signal_interface>;

    template <class Event>
    class Event_signal : public Event_signal_interface
    {
        using Evt_signal = Signal<void (Event&)>;

    public:
        using Function = typename Evt_signal::CbFunction;

        virtual ~Event_signal () {}

        template <class EvtListener>
        void connect (EvtListener& listener)
        {
            void (EvtListener::*receive)(Event &) = &EvtListener::receive;
            Function function = std::bind(receive, &listener, std::placeholders::_1);
            std::size_t connection = signal_.connect(std::move(function));
            listener.as_listener(static_cast<const Event*>(nullptr))->set_connection(connection);
        }

        inline void connect (Function&& listener)
        {
            signal_.connect(listener);
        }

        inline void disconnect (std::size_t connection)
        {
            signal_.disconnect(connection);
        }

        inline void emit (Event& event)
        {
            signal_.emit(event);
        }

    private:
         Evt_signal signal_;
    };

public:
    template <class Event>
    using Receiver_function = typename Event_signal<Event>::Function;

    Event_manager () {}

    Event_manager (const Event_manager&) = delete;
    Event_manager& operator= (const Event_manager&) = delete;

    ~Event_manager ();

    template <class Event, class Receiver_>
    inline void connect (Receiver_& listener)
    {
        get_or_create_event_signal<Event>().connect(listener);
        listener.set_event_manager(*this);
    }

    template <class Event>
    inline void connect (typename Event_signal<Event>::Function&& listener)
    {
        get_or_create_event_signal<Event>().connect(std::move(listener));
    }

    void connect (Event_dispatcher& dispatcher);

    template <class Event>
    inline void disconnect (std::size_t connection)
    {
        event_signal<Event>().disconnect(connection);
    }

    void disconnect (Event_dispatcher& dispatcher);

public:
    template <class Event>
    inline void emit (Event& event)
    {
        std::size_t index = Events::event_type_index<Event>();
        if (index < event_signals_.size())
        {
            Event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
            if (event_signal_uptr)
                static_cast<Event_signal<Event>*>(event_signal_uptr.get())->emit(event);
        }
        emit_to_dispatchers(event);
    }

    template <class Event>
    inline void emit (Event&& event)
    {
        Event evt = std::move(event);
        emit<Event>(std::ref(evt));
    }

    template <class Event>
    inline void emit (std::vector<Event>& events)
    {
        std::size_t index = Events::event_type_index<Event>();
        if (index < event_signals_.size())
        {
            Event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
            if (event_signal_uptr)
            {
                Event_signal<Event>* event_signal = static_cast<Event_signal<Event>*>(event_signal_uptr.get());
                for (Event& event : events)
                    event_signal->emit(event);
            }
        }
    }

    void emit (Async_event_queue& event_queue);

    void reserve (std::size_t number_of_event_types);

private:
    template <class Event>
    inline Event_signal<Event>& event_signal ()
    {
        return *static_cast<Event_signal<Event>*>(event_signals_[Events::event_type_index<Event>()].get());
    }

    template <class Event>
    inline Event_signal<Event>& get_or_create_event_signal ()
    {
        std::size_t index = Events::event_type_index<Event>();
        if (index >= event_signals_.size())
            event_signals_.resize(index + 1);

        Event_signal_interface_uptr& event_signal_uptr = event_signals_[index];
        if (!event_signal_uptr)
        {
            Event_signal_interface_uptr n_event = std::make_unique<Event_signal<Event>>();
            event_signal_uptr = std::move(n_event);
        }

        return *static_cast<Event_signal<Event>*>(event_signal_uptr.get());
    }

    template <class Event>
    void emit_to_dispatchers (Event& event);

        std::vector<Event_signal_interface_uptr> event_signals_;
    std::vector<Event_dispatcher*> event_dispatchers_;
    std::mutex mutex_;
};

class Async_event_queue
{
private:
    class Async_event_queue_interface
    {
    public:
        virtual ~Async_event_queue_interface ();
        virtual void emit (Event_manager& event_manager) = 0;
        virtual void sync () = 0;
    };
    using Async_event_queue_interface_uptr = std::unique_ptr<Async_event_queue_interface>;

    template <class Event_>
    class TAsync_event_queue : public Async_event_queue_interface
    {
    public:
        using Event = Event_;

        virtual ~TAsync_event_queue () {}

        void reserve (std::size_t capacity)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.reserve(capacity);
        }

        const std::vector<Event>& events () const { return events_; }
        std::vector<Event>& events () { return events_; }

        void push (Event&& event)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.push_back(std::move(event));
        }

        virtual void sync () override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.swap(pending_events_);
            pending_events_.clear();
            pending_events_.reserve(events_.capacity());
        }

        virtual void emit (Event_manager& event_manager) override
        {
            event_manager.emit(events());
        }

    private:
        std::vector<Event> events_;
        std::vector<Event> pending_events_;
        std::mutex mutex_;
    };

public:
    template <class Event>
    inline const std::vector<Event>& events ()
    {
        return get_or_create_event_queue<Event>().events();
    }

    template <class Event>
    inline void push (Event&& event)
    {
        get_or_create_event_queue<Event>().push(std::move(event));
    }

    template <class Event>
    void reserve (std::size_t capacity)
    {
        get_or_create_event_queue<Event>().reserve(capacity);
    }

    void sync ();

private:
    friend class Event_manager;

    void emit (Event_manager& event_manager);

    template <class Event>
    inline TAsync_event_queue<Event>& get_or_create_event_queue ()
    {
        std::size_t index = Events::event_type_index<Event>();
        if (index >= event_queues_.size())
            event_queues_.resize(index + 1);

        Async_event_queue_interface_uptr& async_event_queue_uptr = event_queues_[index];
        if (!async_event_queue_uptr)
        {
            Async_event_queue_interface_uptr n_queue = std::make_unique<TAsync_event_queue<Event>>();
            async_event_queue_uptr = std::move(n_queue);
        }

        return *static_cast<TAsync_event_queue<Event>*>(async_event_queue_uptr.get());
    }

private:
    std::vector<Async_event_queue_interface_uptr> event_queues_;
};

class Event_dispatcher
{
public:
    ~Event_dispatcher();

    template <class Event, class Receiver_>
    inline void connect (Receiver_& listener)
    {
        event_manager_.connect<Event>(listener);
    }

    template <class Event>
    inline void connect (Event_manager::Receiver_function<Event>&& listener)
    {
        event_manager_.connect(std::move(listener));
    }

    template <class Event>
    inline void disconnect (std::size_t connection)
    {
        event_manager_.disconnect<Event>(connection);
    }

    void dispatch ();

private:
    friend class Event_manager;

    void set_parent_event_manager (Event_manager& event_manager);

    void set_parent_event_manager (std::nullptr_t);

    template <class Event>
    inline void push_event (Event& event)
    {
        event_queue_.push(Event(event));
    }

private:
    Event_manager* parent_event_manager_ = nullptr;
    Async_event_queue event_queue_;
    Event_manager event_manager_;
};

//////////

template <class Evt>
inline void Event_listener_base::disconnect_from_event (std::size_t connection)
{
    Event_manager* event_manager = this->event_manager();
    if (event_manager)
    {
        this->invalidate();
        event_manager->template disconnect<Evt>(connection);
    }
}

template <class Event>
void Event_manager::emit_to_dispatchers (Event& event)
{
    std::lock_guard lock(mutex_);
    for (Event_dispatcher* dispatcher : event_dispatchers_)
    {
        assert(dispatcher);
        dispatcher->push_event<Event>(event);
    }
}

}
