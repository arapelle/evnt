#pragma once

#include "event_manager.hpp"
#include <mutex>
#include <vector>
#include <memory>

namespace evnt
{
class async_event_queue
{
private:
    class async_event_queue_interface
    {
    public:
        virtual ~async_event_queue_interface();
        virtual void emit(event_manager& evt_manager) = 0;
        virtual void sync() = 0;
    };
    using async_event_queue_interface_uptr = std::unique_ptr<async_event_queue_interface>;

    template <class event_type>
    class tmpl_async_event_queue : public async_event_queue_interface
    {
    public:
        virtual ~tmpl_async_event_queue() {}

        void reserve(std::size_t capacity)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.reserve(capacity);
        }

        const std::vector<event_type>& events() const { return events_; }
        std::vector<event_type>& events() { return events_; }

        void push(event_type&& event)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_events_.push_back(std::move(event));
        }

        virtual void sync() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.swap(pending_events_);
            pending_events_.clear();
            pending_events_.reserve(events_.capacity());
        }

        virtual void emit(event_manager& evt_manager) override
        {
            evt_manager.emit(events());
        }

    private:
        std::vector<event_type> events_;
        std::vector<event_type> pending_events_;
        std::mutex mutex_;
    };

public:
    template <class event_type>
    inline const std::vector<event_type>& events()
    {
        return get_or_create_event_queue_<event_type>().events();
    }

    template <class event_type>
    inline void push(event_type&& event)
    {
        get_or_create_event_queue_<event_type>().push(std::move(event));
    }

    template <class event_type>
    void reserve(std::size_t capacity)
    {
        get_or_create_event_queue_<event_type>().reserve(capacity);
    }

    void sync();
    void emit_events(event_manager& evt_manager);
    void sync_and_emit_events(event_manager& evt_manager);

private:
    template <class event_type>
    inline tmpl_async_event_queue<event_type>& get_or_create_event_queue_()
    {
        std::size_t index = event_info::type_index<event_type>();
        if (index >= event_queues_.size())
            event_queues_.resize(index + 1);

        async_event_queue_interface_uptr& async_event_queue_uptr = event_queues_[index];
        if (!async_event_queue_uptr)
        {
            async_event_queue_interface_uptr n_queue = std::make_unique<tmpl_async_event_queue<event_type>>();
            async_event_queue_uptr = std::move(n_queue);
        }

        return *static_cast<tmpl_async_event_queue<event_type>*>(async_event_queue_uptr.get());
    }

private:
    std::vector<async_event_queue_interface_uptr> event_queues_;
};
}

