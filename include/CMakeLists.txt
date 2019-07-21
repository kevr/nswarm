
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nswarm/
    DESTINATION include/nswarm
    FILES_MATCHING PATTERN "*.hpp"
)

