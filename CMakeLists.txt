# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

project(glassmon)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(glassmon
    main.cpp
)

target_link_libraries(glassmon PRIVATE Qt6::Widgets)