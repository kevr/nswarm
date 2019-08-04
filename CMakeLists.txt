# 
# Project: nswarm
#
#
# Copyright (c) 2019 Kevin Morris
# All Rights Reserved.
#
cmake_minimum_required( VERSION 2.4 )
project( nswarm )

# Deal with build types provided; default: DEBUG
if( CMAKE_BUILD_TYPE )
else( CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE "DEBUG" )
endif( CMAKE_BUILD_TYPE )

# Alter library flags dependant on build type
if( CMAKE_BUILD_TYPE EQUAL "DEBUG" )
set( PROJECT_LIBS "-ltcmalloc -lpthread -lgtest -lboost_program_options -lboost_system -lssl -lcrypto -ltsan" )
else( CMAKE_BUILD_TYPE EQUAL "DEBUG" )
set( PROJECT_LIBS "-ltcmalloc -lpthread -lgtest -lboost_program_options -lboost_system -lssl -lcrypto" )
endif( CMAKE_BUILD_TYPE EQUAL "DEBUG" )

# We require a C++17 compliant compiler

set( CMAKE_CXX_FLAGS_DEBUG "-std=c++17 -O0 -g -fsanitize=thread" )
set( CMAKE_CXX_FLAGS_RELEASE "-std=c++17 -O3" )

set( CMAKE_CXX_STANDARD_LIBRARIES ${PROJECT_LIBS} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

# Traverse through src directory
add_subdirectory( src )
add_subdirectory( include )

add_custom_target( runtests ${CMAKE_CURRENT_SOURCE_DIR}/scripts/runtest.sh )

