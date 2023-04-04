cmake_minimum_required(VERSION 3.1.2)
project(Mini-SQLite VERSION 0.1.0)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.h" "src/*.cpp")

#[[
include(CTest)
enable_testing()
]]

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

#[[
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug ... FORCE)
endif()
]]

add_executable(${PROJECT_NAME}.out ${SOURCE_FILES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)