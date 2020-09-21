#pragma once

#include <atomic>

namespace evnt
{
class event_info
{
    inline static std::size_t generate_type_index_()
    {
        static std::atomic_size_t index = 0;
        return index++;
    }

public:
    template <class event_type>
    inline static std::size_t type_index()
    {
        static const std::size_t index = generate_type_index_();
        return index;
    }
};
}
