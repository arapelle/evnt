#include <evnt/event_manager.hpp>
#include <evnt/event_box.hpp>

namespace evnt
{
event_manager::~event_manager()
{
    std::lock_guard lock(mutex_);
    for (event_box* dispatcher : event_boxs_)
    {
        assert(dispatcher);
        dispatcher->set_parent_event_manager(nullptr);
    }
}

void event_manager::connect(event_box& dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dispatcher.set_parent_event_manager(*this);
    event_boxs_.push_back(&dispatcher);
}

void event_manager::disconnect(event_box& dispatcher)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = std::find(event_boxs_.begin(), event_boxs_.end(), &dispatcher);
    if (iter != event_boxs_.end())
    {
        dispatcher.set_parent_event_manager(nullptr);
        std::iter_swap(iter, std::prev(event_boxs_.end()));
        event_boxs_.pop_back();
    }
}

void event_manager::reserve(std::size_t number_of_event_types)
{
    event_signals_.reserve(number_of_event_types);
}

void event_manager::emit(event_box& event_box, bool pre_sync)
{
    if (pre_sync)
        event_box.event_queue().sync_and_emit_events(*this);
    else
        event_box.event_queue().emit_events(*this);
}

}
