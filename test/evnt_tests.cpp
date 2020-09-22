#include <evnt/evnt.hpp>
#include <gtest/gtest.h>
#include <cstdlib>

class int_event
{
public:
    int value;
};

TEST(event_manager_tests, test_emit_event_lvalue_reference)
{
    evnt::event_manager event_manager;
    int value = 0;
    event_manager.connect<int_event>([&value](int_event& event)
    {
        value = event.value;
    });

    int_event evt{ 5 };
    event_manager.emit(evt);
    ASSERT_EQ(value, 5);
}

TEST(event_manager_tests, test_emit_event_rvalue_reference)
{
    evnt::event_manager event_manager;
    int value = 0;
    event_manager.connect<int_event>([&value](int_event& event)
    {
        value = event.value;
    });

    int_event evt{ 5 };
    event_manager.emit(evt);
    ASSERT_EQ(value, 5);
}

class int_event_Listener : public evnt::event_listener<int_event>
{
public:
    int_event_Listener(int& value)
    : value_ptr(&value)
    {}

    ~int_event_Listener()
    {
        if (value_ptr)
            *value_ptr *= 10;
    }

    void receive(int_event& event)
    {
        *value_ptr = event.value;
    }

    int* value_ptr;
};

TEST(event_manager_tests, test_listener_auto_deconnection)
{
    int value = 0;

    {
        evnt::event_manager event_manager;

        {
            int_event_Listener listener(value);
            event_manager.connect<int_event>(listener);

            event_manager.emit(int_event{ 5 });
            ASSERT_EQ(value, 5);
        }
        ASSERT_EQ(value, 5 * 10);

        event_manager.emit(int_event{ 200 });
        ASSERT_EQ(value, 5 * 10);
    }
    ASSERT_EQ(value, 5 * 10);
}

TEST(event_manager_tests, test_listener_deconnection)
{
    int value = 0;

    {
        evnt::event_manager event_manager;

        {
            int_event_Listener listener(value);
            event_manager.connect<int_event>(listener);

            event_manager.emit(int_event{ 5 });
            ASSERT_EQ(value, 5);

            listener.disconnect<int_event>();
            event_manager.emit(int_event{ 7 });
            ASSERT_EQ(value, 5);
        }
        ASSERT_EQ(value, 5 * 10);

        event_manager.emit(int_event{ 200 });
        ASSERT_EQ(value, 5 * 10);
    }
    ASSERT_EQ(value, 5 * 10);
}

class int_event_2
{
public:
    int value;
};

class multi_event_listener : public evnt::event_listener<int_event, int_event_2>
{
public:
    multi_event_listener(int& value, int& value_2)
    : value_ptr(&value), value_ptr_2(&value_2)
    {}

    ~multi_event_listener()
    {
        *value_ptr += 100;
        *value_ptr_2 += 100;
    }

    void receive(int_event& event)
    {
        *value_ptr = event.value + 1;
    }

    void receive(int_event_2& event)
    {
        *value_ptr_2 = event.value + 2;
    }

    int* value_ptr;
    int* value_ptr_2;
};

TEST(event_manager_tests, test_multi_listener_connection_and_auto_deconnection)
{
    int value = 0;
    int value_2 = 0;

    {
        evnt::event_manager event_manager;

        {
            multi_event_listener listener(value, value_2);
            event_manager.connect<int_event>(listener);
            event_manager.connect<int_event_2>(listener);

            event_manager.emit(int_event{ 5 });
            event_manager.emit(int_event_2{ 7 });

            ASSERT_EQ(value, 6);
            ASSERT_EQ(value_2, 9);
        }
        ASSERT_EQ(value, 6 + 100);
        ASSERT_EQ(value_2, 9 + 100);

        event_manager.emit(int_event{ 10 });
        event_manager.emit(int_event_2{ 13 });

        ASSERT_EQ(value, 6 + 100);
        ASSERT_EQ(value_2, 9 + 100);
    }
    ASSERT_EQ(value, 6 + 100);
    ASSERT_EQ(value_2, 9 + 100);
}

TEST(event_manager_tests, test_multi_listener_deconnections_and_reconnection)
{
    int value = 0;
    int value_2 = 0;

    {
        evnt::event_manager event_manager;

        {
            multi_event_listener listener(value, value_2);
            event_manager.connect<int_event>(listener);
            event_manager.connect<int_event_2>(listener);

            event_manager.emit(int_event{ 5 });
            event_manager.emit(int_event_2{ 7 });
            ASSERT_EQ(value, 6);
            ASSERT_EQ(value_2, 9);

            listener.disconnect<int_event>();
            event_manager.emit(int_event{ 10 });
            event_manager.emit(int_event_2{ 10 });
            ASSERT_EQ(value, 6);
            ASSERT_EQ(value_2, 12);

            event_manager.connect<int_event>(listener);
            event_manager.emit(int_event{ 10 });
            event_manager.emit(int_event_2{ 20 });
            ASSERT_EQ(value, 11);
            ASSERT_EQ(value_2, 22);

            listener.disconnect_all();
            event_manager.emit(int_event{ -10 });
            event_manager.emit(int_event_2{ -20 });
            ASSERT_EQ(value, 11);
            ASSERT_EQ(value_2, 22);
        }
        ASSERT_EQ(value, 111);
        ASSERT_EQ(value_2, 122);

        event_manager.emit(int_event{ 20 });
        event_manager.emit(int_event_2{ 23 });
        ASSERT_EQ(value, 111);
        ASSERT_EQ(value_2, 122);
    }
    ASSERT_EQ(value, 111);
    ASSERT_EQ(value_2, 122);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
