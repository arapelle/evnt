#pragma once

#include <atomic>
#include <cassert>

namespace evnt
{
class event_manager;

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
    inline void break_connection(std::size_t connection);

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
    inline void disconnect() { this->event_listener_base::template break_connection<event_type>(connection_); }

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
    inline void disconnect() { this->event_listener<event_types...>::template break_connection<event_type>(connection_); }

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
}
