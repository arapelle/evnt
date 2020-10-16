#include <evnt/evnt.hpp>
#include <gtest/gtest.h>
#include <cstdlib>

class int_event
{
public:
    int value;
};

TEST(event_box_tests, test_connection)
{
    evnt::event_manager event_manager;
    evnt::event_box event_box;
    evnt::event_manager other_event_manager;

    int value = 0;
    other_event_manager.connect<int_event>([&value](int_event& event)
    {
        value = event.value;
    });

    event_manager.emit(int_event{ 5 });
    ASSERT_EQ(value, 0);

    event_manager.connect(event_box);
    event_manager.emit(int_event{ 5 });
    ASSERT_EQ(value, 0);

    other_event_manager.emit(event_box);
    ASSERT_EQ(value, 5);
}

TEST(event_box_tests, test_connection_2)
{
    evnt::event_manager event_manager;

    evnt::event_box event_box;
    evnt::event_manager other_event_manager;
    event_manager.connect(event_box);
    event_manager.emit(int_event{ 5 });

    int value = 0;
    other_event_manager.connect<int_event>([&value](int_event& event)
    {
        value = event.value;
    });
    ASSERT_EQ(value, 0);

    other_event_manager.emit(event_box);
    ASSERT_EQ(value, 5);
}

TEST(event_box_tests, test_auto_deconnection)
{
    evnt::event_manager event_manager;
    int value = 0;

    {
        evnt::event_box event_box;
        evnt::event_manager other_event_manager;
        other_event_manager.connect<int_event>([&value](int_event& event)
        {
            value = event.value;
        });
        event_manager.connect(event_box);
        event_manager.emit(int_event{ 5 });
        other_event_manager.emit(event_box);
        ASSERT_EQ(value, 5);
    }

    event_manager.emit(int_event{ 8 });
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
