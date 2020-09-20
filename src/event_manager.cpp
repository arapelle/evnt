#include <evnt/event_manager.hpp>

namespace evnt
{
Event_manager::~Event_manager ()
{
    std::lock_guard lock(mutex_);
    for (Event_dispatcher* dispatcher : event_dispatchers_)
    {
        assert(dispatcher);
        dispatcher->set_parent_event_manager(nullptr);
    }
}

void Event_manager::emit (Async_event_queue& event_queue)
{
    event_queue.emit(*this);
}

void Event_manager::connect (Event_dispatcher& dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dispatcher.set_parent_event_manager(*this);
    event_dispatchers_.push_back(&dispatcher);
}

void Event_manager::disconnect (Event_dispatcher& dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = std::find(event_dispatchers_.begin(), event_dispatchers_.end(), &dispatcher);
    if (iter != event_dispatchers_.end())
    {
        dispatcher.set_parent_event_manager(nullptr);
        std::iter_swap(iter, std::prev(event_dispatchers_.end()));
        event_dispatchers_.pop_back();
    }
}

void Event_manager::reserve (std::size_t number_of_event_types)
{
    event_signals_.reserve(number_of_event_types);
}

//////////

Event_dispatcher::~Event_dispatcher()
{
    if (parent_event_manager_)
        parent_event_manager_->disconnect(*this);
}

void Event_dispatcher::dispatch ()
{
    event_queue_.sync();
    event_manager_.emit(event_queue_);
}

void Event_dispatcher::set_parent_event_manager (Event_manager& event_manager)
{
    assert(!parent_event_manager_);
    parent_event_manager_ = &event_manager;
}

void Event_dispatcher::set_parent_event_manager (std::nullptr_t)
{
    assert(parent_event_manager_);
    parent_event_manager_ = nullptr;
}

//////////

Async_event_queue::Async_event_queue_interface::~Async_event_queue_interface ()
{
}

/////

void Async_event_queue::sync ()
{
    for (Async_event_queue_interface_uptr& event_queue : event_queues_)
        if (event_queue)
            event_queue->sync();
}

void Async_event_queue::emit (Event_manager& event_manager)
{
    for (const Async_event_queue_interface_uptr& event_queue : event_queues_)
        if (event_queue)
            event_queue->emit(event_manager);
}

}
