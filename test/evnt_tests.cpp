#include <evnt/evnt.hpp>
#include <gtest/gtest.h>
#include <cstdlib>

TEST(evnt_tests, basic_test)
{
    ASSERT_EQ(module_name(), "evnt");
}

using event_manager = evnt::event_manager;

class Event
{
public:
    int value;
};

void test_event_manager ()
{
    event_manager event_manager;
    int value = 0;
    int expected_value = 5;
    event_manager.connect<Event>([&value](Event& event)
    {
        value = event.value;
    });

    Event evt{ expected_value };
    event_manager.emit(evt);
    ASSERT_EQ(value, expected_value);
}

class Event_Listener : public evnt::event_listener<Event>
{
public:
    Event_Listener (int& value)
    : value_ptr(&value)
    {}

    ~Event_Listener ()
    {
        if (value_ptr)
            *value_ptr *= 10;
    }

    void receive (Event& event)
    {
        *value_ptr = event.value;
    }

    int* value_ptr;
};

void test_event_manager_2 ()
{
    int value = 0;
    int expected_value = 5;

    {
        event_manager event_manager;

        {
            Event_Listener listener(value);
            event_manager.connect<Event>(listener);

            Event evt{ expected_value };
            event_manager.emit(evt);
            ASSERT_EQ(value, expected_value);
        }
        ASSERT_EQ(value, expected_value * 10);

        event_manager.emit(Event{expected_value * 20});
        ASSERT_EQ(value, expected_value * 10);
    }
    ASSERT_EQ(value, expected_value * 10);
}

void test_event_manager_3 ()
{
    int value = 0;
    int expected_value = 5;

    {
        event_manager event_manager;

        {
            Event_Listener listener(value);
            event_manager.connect<Event>(listener);

            Event evt{ expected_value };
            event_manager.emit(evt);
            ASSERT_EQ(value, expected_value);

            listener.disconnect();
            ASSERT_EQ(value, expected_value);
        }
        ASSERT_EQ(value, expected_value * 10);

        event_manager.emit(Event{expected_value * 20});
        ASSERT_EQ(value, expected_value * 10);
    }
    ASSERT_EQ(value, expected_value * 10);
}

class Event_2
{
public:
    int value;
};

class Multi_event_listener : public evnt::event_listener<Event, Event_2>
{
public:
    Multi_event_listener (int& value, int& value_2)
    : value_ptr(&value), value_ptr_2(&value_2)
    {}

    void receive (Event& event)
    {
        *value_ptr = event.value + 1;
    }

    void receive (Event_2& event)
    {
        *value_ptr_2 = event.value + 2;
    }

    int* value_ptr;
    int* value_ptr_2;
};

void test_event_manager_4 ()
{
    event_manager event_manager;
    int value = 0;
    int value_2 = 0;
    Multi_event_listener listener(value, value_2);
    event_manager.connect<Event>(listener);
    event_manager.connect<Event_2>(listener);

    int expected_value = 6;
    Event event{ 5 };
    event_manager.emit(event);

    int expected_value_2 = 9;
    Event_2 event_2{ 7 };
    event_manager.emit(event_2);

    ASSERT_EQ(value, expected_value);
    ASSERT_EQ(value_2, expected_value_2);
}

TEST(evnt_tests, legacy_tests)
{
    test_event_manager();
    test_event_manager_2();
    test_event_manager_3();
    test_event_manager_4();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
