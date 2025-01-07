cmake_minimum_required(VERSION 3.10)
project(MyCppApp)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable multi-threading
find_package(Threads REQUIRED)

# Find Boost libraries
find_package(Boost 1.70 REQUIRED COMPONENTS system thread)

# Include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
    include
)

# Include directories for third-party libraries
# Adjust the paths if necessary
include_directories(
    path/to/cpp-httplib
    path/to/inja/include
    path/to/nlohmann/json/include
)

# Collect all source files
file(GLOB SOURCES
    main.cpp
    src/*.cpp
)

# Add executable
add_executable(main ${SOURCES})

# Link libraries
target_link_libraries(main
    ${Boost_LIBRARIES}
    Threads::Threads
    # Add other libraries if needed, e.g., for cpp-httplib, Inja, nlohmann/json
)

# Optionally, define preprocessor macros or compile options
# target_compile_definitions(main PRIVATE SOME_MACRO=1)
