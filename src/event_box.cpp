#include <evnt/event_box.hpp>

namespace evnt
{
event_box::~event_box()
{
    std::lock_guard lock(mutex_);
    if (parent_event_manager_)
    {
        parent_event_manager_->disconnect(*this);
        parent_event_manager_ = nullptr;
    }
}

void event_box::emit_received_events()
{
    event_queue_.sync_and_emit_events(event_manager_);
}

void event_box::set_parent_event_manager(event_manager& evt_manager)
{
    std::lock_guard lock(mutex_);
    assert(!parent_event_manager_);
    parent_event_manager_ = &evt_manager;
}

void event_box::set_parent_event_manager(std::nullptr_t)
{
    if (mutex_.try_lock())
    {
        std::lock_guard lock(mutex_, std::adopt_lock);
        assert(parent_event_manager_);
        parent_event_manager_ = nullptr;
    }
}
}
