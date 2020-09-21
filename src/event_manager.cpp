#include <evnt/event_manager.hpp>

namespace evnt
{
// Event manager:

event_manager::~event_manager()
{
    std::lock_guard lock(mutex_);
    for (event_dispatcher* dispatcher : event_dispatchers_)
    {
        assert(dispatcher);
        dispatcher->set_parent_event_manager(nullptr);
    }
}

void event_manager::emit(async_event_queue& event_queue)
{
    event_queue.emit(*this);
}

void event_manager::connect(event_dispatcher& dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dispatcher.set_parent_event_manager(*this);
    event_dispatchers_.push_back(&dispatcher);
}

void event_manager::disconnect(event_dispatcher& dispatcher)
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

void event_manager::reserve(std::size_t number_of_event_types)
{
    event_signals_.reserve(number_of_event_types);
}

// Event manager:

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

//////////

async_event_queue::async_event_queue_interface::~async_event_queue_interface()
{
}

/////

void async_event_queue::sync()
{
    for (async_event_queue_interface_uptr& event_queue : event_queues_)
        if (event_queue)
            event_queue->sync();
}

void async_event_queue::emit(event_manager& evt_manager)
{
    for (const async_event_queue_interface_uptr& event_queue : event_queues_)
        if (event_queue)
            event_queue->emit(evt_manager);
}

}
