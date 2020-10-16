cmake_minimum_required(VERSION 3.18)

project(basic_cmake_project)

add_executable(${PROJECT_NAME} main.cpp)
# Find package evnt:
find_package(evnt 0.2 CONFIG REQUIRED)
# Use evnt release shared target:
target_link_libraries(${PROJECT_NAME} PRIVATE arba::evnt)
# Use evnt release static target:
#target_link_libraries(${PROJECT_NAME} PRIVATE arba::evnt-static)

