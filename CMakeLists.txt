# 
# Project: nswarm
#
#
# Copyright (c) 2019 Kevin Morris
# All Rights Reserved.
#

project( nswarm )
cmake_minimum_required( VERSION 2.4 )

# Add target linker flags here
set(PROJECT_LIBS "-pthread -lgtest -lboost_program_options")

# We require a C++17 compliant compiler
set(CMAKE_CXX_FLAGS "-std=c++17 -O0 -g")
set(CMAKE_CXX_STANDARD_LIBRARIES ${PROJECT_LIBS})

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

# Traverse through src directory
add_subdirectory( src )
add_subdirectory( include )

add_custom_target(runtests ${CMAKE_CURRENT_SOURCE_DIR}/scripts/runtest.sh)

