set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON SYSTEM ON)

# Set CMP0069 policy to "NEW" for building external targets with LTO enabled
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# GoogleTest
add_subdirectory(googletest)
enable_testing()
