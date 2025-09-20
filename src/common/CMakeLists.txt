add_library(common STATIC)

target_sources(common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/IoFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PathUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/Backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/Filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Logging/TextFormatter.cpp
)

target_link_libraries(common PUBLIC fmt::fmt)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
