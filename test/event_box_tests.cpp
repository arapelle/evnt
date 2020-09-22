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

    int value = 0;
    event_box.connect<int_event>([&value](int_event& event)
    {
        value = event.value;
    });

    event_manager.emit(int_event{ 5 });
    ASSERT_EQ(value, 0);

    event_manager.connect(event_box);
    event_manager.emit(int_event{ 5 });
    ASSERT_EQ(value, 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
