# note: use "cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ ." to generate debug version for coverage
#############################################################
#                                                           #
#                   GLOBAL CMAKE SETUP                      #
#                                                           #
#############################################################
option(ENABLE_CODE_ANALYSIS "Run code cppcheck" ON)
cmake_minimum_required(VERSION 3.13)
# Path to cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# disable extentions for pure C++
if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()
enable_testing()
enable_language(CXX)
if(ENABLE_CODE_ANALYSIS)
    find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
    if (CMAKE_CXX_CPPCHECK)
        list(
            APPEND CMAKE_CXX_CPPCHECK 
                "--enable=warning"
                "--inconclusive"
                "--force" 
                "--inline-suppr"
                "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt"
        )
    endif()
endif(ENABLE_CODE_ANALYSIS)

#############################################################
#                                                           #
#                         LIBRARY                           #
#                                                           #
#############################################################
project(s21_containers VERSION 1.0 LANGUAGES CXX)
include (CTest)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wenum-compare -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fprofile-arcs -ftest-coverage -Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wenum-compare -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo ${CMAKE_CXX_FLAGS_DEBUG}")
file(GLOB SRC
     "*.cc"
)
add_library(s21_containers INTERFACE ${SRC})
set( Boost_USE_STATIC_LIBS OFF )
set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_RUNTIME OFF )

find_package( Boost 1.72.0 COMPONENTS thread )

if ( Boost_FOUND )
    target_compile_definitions(s21_containers INTERFACE BOOST_FOUND)
    target_include_directories( s21_containers INTERFACE ${Boost_INCLUDE_DIRS} )
    target_link_libraries( s21_containers INTERFACE ${Boost_LIBRARIES} )
else()
    message( WARNING "Required Boost packages not found. Compiling without tree print support. Perhaps add -DBOOST_ROOT?" )
endif()
target_include_directories(s21_containers INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

#############################################################
#                                                           #
#                     GTEST COMPILE                         #
#                                                           #
#############################################################
find_package(GTest)
if (NOT GTEST_FOUND)
    message("-- Added GTest target from source")
    if (NOT TARGET gtest_main)
        unset(CMAKE_CXX_FLAGS)
        unset(CMAKE_CXX_FLAGS_DEBUG)
        set(_flag_sync_required TRUE)
        add_subdirectory(googletest)
    endif ()
    include_directories(${gtest_SOURCE_DIR}/include ${gmock_SOURCE_DIR}/include)
    set(GTEST_FOUND TRUE)
endif()

#############################################################
#                                                           #
#                         TESTING                           #
#                                                           #
#############################################################
if (GTEST_FOUND)
    set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wenum-compare -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -g ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fprofile-arcs -ftest-coverage -Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wenum-compare -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo ${CMAKE_CXX_FLAGS_DEBUG}")
    project(unit_tests VERSION 1.0 LANGUAGES CXX)
    option(CodeCoverage "CodeCoverage" ON)
    include(CTest)
    file(GLOB TEST_SRC
        "tests/*.cc"
    )
    add_executable(unit_tests ${TEST_SRC})
    set_property(TARGET unit_tests PROPERTY CXX_STANDARD 17)
    set_property(TARGET unit_tests PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)

    set_property(TARGET unit_tests PROPERTY THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(unit_tests
        gtest
        stdc++
        m
        gtest_main
        s21_containers
        Threads::Threads
    )

    file(GLOB TEST_CLEAN_FILES
        "*.dot"
    )
    set_property(
        TARGET unit_tests
        APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES Temporary Testing TAG coverage ${TEST_CLEAN_FILES}
    )
    add_test(UNIT_TESTS unit_tests)
endif()

#############################################################
#                                                           #
#                        COVERAGE                           #
#                                                           #
#############################################################

if (GTEST_FOUND)
    if (CodeCoverage MATCHES ON)
        include(CodeCoverage)
        setup_target_for_coverage(coverage unit_tests coverage)
        set_property(
            TARGET coverage
            APPEND
            PROPERTY ADDITIONAL_CLEAN_FILES DartConfiguration.tcl
        )
    endif()
    
endif()

#############################################################
#                                                           #
#                    additional checks                      #
#                                                           #
#############################################################
if (GTEST_FOUND)
    find_program(VALGRIND NAMES valgrind)
    if (VALGRIND)
        add_custom_target(memcheck
            COMMAND ${VALGRIND} -s --trace-children=yes --track-origins=yes --leak-check=yes --quiet ./unit_tests
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
        add_dependencies(memcheck unit_tests)
    endif()

    # Proper way but there are still reachable blocks
    # add_custom_target(memcheck
    #     COMMAND ${CMAKE_CTEST_COMMAND} 
    #         --force-new-ctest-process --test-action memcheck
    #     COMMAND cat "${CMAKE_BINARY_DIR}/Testing/Temporary/MemoryChecker.*.log"
    #     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    find_program(CLANG_FORMAT NAMES clang-format)
    if (CLANG_FORMAT)
        if(NOT EXISTS ".clang-format")
            MESSAGE(WARNING ".clang-format file should be placed in src folder, continuing without it, test result may be incorect.")
        else()
            file(GLOB_RECURSE STYLE_CHECK_FILES *.cc *.h *.tpp *.inc)
            list(FILTER STYLE_CHECK_FILES EXCLUDE REGEX "/googletest/")
            add_custom_target(stylecheck
            COMMAND ${CLANG_FORMAT} -n ${STYLE_CHECK_FILES}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

            add_custom_target(stylechange
            COMMAND ${CLANG_FORMAT} -i ${STYLE_CHECK_FILES}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
        endif()
        
    endif()
endif()
