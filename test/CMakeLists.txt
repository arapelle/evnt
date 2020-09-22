add_cpp_library_tests(SHARED ${PROJECT_NAME}
                      STATIC ${PROJECT_NAME}-static
                      SOURCES
                        event_manager_tests.cpp
                        event_box_tests.cpp
                      )
