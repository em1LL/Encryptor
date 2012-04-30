#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
cmake_minimum_required(VERSION 2.6)

# Enable non unique target names in different subdirs
cmake_policy(SET CMP0002 OLD)

project(EncryptorTutorial CXX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION_STR ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------
find_package(
    Boost 1.47 REQUIRED COMPONENTS
    # Lets keep libs list sorted... :)
    filesystem
    program_options
    unit_test_framework
  )


#---------------------------------------------------------
# Section: Configure builing process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})
# Tell to boost::result_of to use decltype to decay return type of callable.
# NOTE This would enabel to use C++11 labmda expressions w/ boost::rage adaptors
add_definitions(-DBOOST_RESULT_OF_USE_DECLTYPE)

if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    message(STATUS "Turn C++11 mode for ${CMAKE_CXX_COMPILER_ID} compiler")
    set(CMAKE_CXX_FLAGS "-pipe -std=c++0x ${CMAKE_CXX_FLAGS}")
endif()


#---------------------------------------------------------
# Section: Define targets
#---------------------------------------------------------

add_executable(
    ${PROJECT_NAME}
    main.cpp
  )
target_link_libraries(
    ${PROJECT_NAME}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )
