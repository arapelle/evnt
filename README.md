# Concept

The purpose is to provide C++ event managing tools.

- event_listener
- event_manager
- event_box

See [task board](https://app.gitkraken.com/glo/board/X2dgij2bBQARwA8W) for future updates and features.

# Install

## Requirements

Binaries:

- A C++20 compiler (ex: g++-10)
- CMake 3.16 or later

Libraries:

- [Google Test](https://github.com/google/googletest) 1.10 or later (only for testing)

## Clone

```
git clone https://github.com/arapelle/evnt --recurse-submodules
```

## Quick Install

There is a cmake script at the root of the project which builds the library in *Release* mode and install it (default options are used).

```
cd /path/to/evnt
cmake -P cmake_quick_install.cmake
```

Use the following to quickly install a different mode.

```
cmake -DCMAKE_BUILD_TYPE=Debug -P cmake_quick_install.cmake
```

## Uninstall

There is a uninstall cmake script created during installation. You can use it to uninstall properly this library.

```
cd /path/to/installed-evnt/
cmake -P cmake_uninstall.cmake
```

# How to use

## Example - Connect a function to an *event_manager*

```c++
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
```

## Example - Using *evnt* in a CMake project

See the [basic cmake project](https://github.com/arapelle/evnt/tree/master/example/basic_cmake_project) example, and more specifically the [CMakeLists.txt](https://github.com/arapelle/evnt/tree/master/example/basic_cmake_project/CMakeLists.txt) to see how to use *evnt* in your CMake projects.

# License

[MIT License](https://github.com/arapelle/evnt/blob/master/LICENSE.md) © evnt