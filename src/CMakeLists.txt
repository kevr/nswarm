
include_directories(".")

# Compile tests first
add_subdirectory(tests)

# Produce targets: nswarm-host nswarm-node
add_executable(
    nswarm-host main.cpp
)

add_executable(
    nswarm-node node/main.cpp
)

