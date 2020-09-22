#include <evnt/event_dispatcher.hpp>

namespace evnt
{
event_dispatcher::~event_dispatcher()
{
    if (parent_event_manager_)
        parent_event_manager_->disconnect(*this);
}

void event_dispatcher::dispatch()
{
    event_queue_.sync();
    event_manager_.emit(event_queue_);
}

void event_dispatcher::set_parent_event_manager(event_manager& evt_manager)
{
    assert(!parent_event_manager_);
    parent_event_manager_ = &evt_manager;
}

void event_dispatcher::set_parent_event_manager(std::nullptr_t)
{
    assert(parent_event_manager_);
    parent_event_manager_ = nullptr;
}
}
