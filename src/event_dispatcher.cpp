#include <evnt/event_dispatcher.hpp>

namespace evnt
{
event_dispatcher::~event_dispatcher()
{
    std::lock_guard lock(mutex_);
    if (parent_event_manager_)
        parent_event_manager_->disconnect(*this);
}

void event_dispatcher::dispatch()
{
    event_queue_.sync_and_emit_events(event_manager_);
}

void event_dispatcher::set_parent_event_manager(event_manager& evt_manager)
{
    std::lock_guard lock(mutex_);
    assert(!parent_event_manager_);
    parent_event_manager_ = &evt_manager;
}

void event_dispatcher::set_parent_event_manager(std::nullptr_t)
{
    std::lock_guard lock(mutex_);
    assert(parent_event_manager_);
    parent_event_manager_ = nullptr;
}
}
