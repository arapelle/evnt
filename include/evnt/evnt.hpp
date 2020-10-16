#pragma once 

#include "event_listener.hpp"
#include "event_manager.hpp"
#include "async_event_queue.hpp"
#include "event_box.hpp"

namespace evnt
{
template <class event_type>
inline void event_listener_base::break_connection(std::size_t connection)
{
    event_manager* evt_manager = this->evt_manager();
    if (evt_manager)
    {
        this->invalidate();
        evt_manager->template disconnect<event_type>(connection);
    }
}

template <class event_type>
void event_manager::emit_to_event_boxes_(event_type& event)
{
    std::lock_guard lock(mutex_);
    for (event_box* dispatcher : event_boxs_)
    {
        assert(dispatcher);
        dispatcher->push_event<event_type>(event);
    }
}
}
