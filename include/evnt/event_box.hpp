#pragma once

#include "async_event_queue.hpp"

namespace evnt
{
class event_box
{
public:
    ~event_box();

private:
    friend class event_manager;

    void set_parent_event_manager(event_manager& evt_manager);
    void set_parent_event_manager(std::nullptr_t);

    template <class event_type>
    inline void push_event(event_type& event)
    {
        event_queue_.push(event_type(event));
    }

    inline async_event_queue& event_queue() { return event_queue_; }

private:
    event_manager* parent_event_manager_ = nullptr;
    async_event_queue event_queue_;
    std::mutex mutex_;
};
}
