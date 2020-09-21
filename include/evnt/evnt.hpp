#pragma once 

namespace evnt
{
class event_manager;
class async_event_queue;
class Event_dispatcher;
}

#include "event_listener.hpp"
#include "event_manager.hpp"
#include "event_listener.inl"

namespace evnt
{
}

#include <string>

std::string module_name();
