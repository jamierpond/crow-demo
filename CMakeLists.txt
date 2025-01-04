cmake_minimum_required(VERSION 3.14)
project(CrowApp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable ThreadSanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")

# Set debug build type for better ThreadSanitizer results
set(CMAKE_BUILD_TYPE Debug)

# Include the FetchContent module
include(FetchContent)

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
