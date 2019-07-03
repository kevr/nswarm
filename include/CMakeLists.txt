
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/nswarm
    FILES_MATCHING PATTERN "*.hpp"
)

