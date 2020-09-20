#pragma once 

namespace evnt
{
class Event_manager;
class Async_event_queue;
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
