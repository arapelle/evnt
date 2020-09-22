#pragma once

#include "async_event_queue.hpp"

namespace evnt
{
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
}
