#include <evnt/event_manager.hpp>
#include <evnt/event_dispatcher.hpp>

namespace evnt
{
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
}
