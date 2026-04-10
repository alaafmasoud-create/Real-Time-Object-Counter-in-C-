cmake_minimum_required(VERSION 3.16)

project(real_time_object_counter_cpp VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_executable(real_time_object_counter
    src/main.cpp
    src/counter.cpp
    src/tracker.cpp
    src/csv_logger.cpp
    src/utils.cpp
)

target_include_directories(real_time_object_counter PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(real_time_object_counter PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(real_time_object_counter PRIVATE /W4 /permissive-)
else()
    target_compile_options(real_time_object_counter PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS real_time_object_counter RUNTIME DESTINATION bin)
