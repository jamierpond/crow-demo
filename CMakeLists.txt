cmake_minimum_required(VERSION 3.14)
project(CrowApp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the FetchContent module
include(FetchContent)

find_package( asio REQUIRED )


# Fetch the Crow library
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG master
)
FetchContent_MakeAvailable(Crow)
FetchContent_Populate(Crow)

# Create an executable
add_executable(CrowApp main.cpp)

target_include_directories(CrowApp PRIVATE ${Crow_SOURCE_DIR}/include)

target_link_libraries(CrowApp PRIVATE Crow::Crow)

