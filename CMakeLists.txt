cmake_minimum_required(VERSION 3.10)

project(rgraph)

add_executable(rgraph rgraph.cpp)

find_package(fmt)
target_link_libraries(rgraph fmt::fmt)

add_custom_target(test ALL
                  COMMAND cd .. && build/rgraph test-csv/*)
