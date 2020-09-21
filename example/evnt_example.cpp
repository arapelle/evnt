#include <evnt/evnt.hpp>
#include <iostream>

class int_event
{
public:
    int value;
};

int main()
{
    evnt::event_manager event_manager;
    event_manager.connect<int_event>([](int_event& event)
    {
        std::cout << "I received an int_event: " << event.value << std::endl;
    });

    event_manager.emit(int_event{ 42 });
    return EXIT_SUCCESS;
}
