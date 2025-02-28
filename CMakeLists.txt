cmake_minimum_required(VERSION 3.14)
project(CrowApp)

# Define sanitizer options
option(USE_ASAN "Enable Address Sanitizer" OFF)
option(USE_TSAN "Enable Thread Sanitizer" OFF)
option(USE_MSAN "Enable Memory Sanitizer" OFF)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# # debug mode
# set(CMAKE_BUILD_TYPE Debug)

# Configure sanitizers if enabled
if(USE_ASAN)
    message(STATUS "Enabling Address Sanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(USE_TSAN)
    message(STATUS "Enabling Thread Sanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(USE_MSAN)
    message(STATUS "Enabling Memory Sanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
    set(CMAKE_BUILD_TYPE Debug)
endif()


# Include the FetchContent module
include(FetchContent)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Fetch the Crow library
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG master
)
FetchContent_MakeAvailable(Crow)

FetchContent_Declare(
  Zoo
  GIT_REPOSITORY https://github.com/thecppzoo/zoo.git
  GIT_TAG master
)
FetchContent_MakeAvailable(Zoo)

# Create an executable
add_executable(CrowApp main.cpp)
target_include_directories(CrowApp PRIVATE ${Crow_SOURCE_DIR}/include)
target_include_directories(CrowApp PRIVATE ${zoo_SOURCE_DIR}/inc)
target_compile_definitions(CrowApp PRIVATE -DZOO_USE_LEASTNBITSMASK)
target_link_libraries(CrowApp PRIVATE Crow::Crow)


# Fetch and configure Catch2
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Tests
enable_testing()
add_executable(tests test/test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)

