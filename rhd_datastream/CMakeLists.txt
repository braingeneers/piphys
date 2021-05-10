if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

cmake_minimum_required(VERSION 3.1)

project(rhd_datastream C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/lib/cmake")

file(GLOB RHD_SRC ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp .c)

set(RHD_PROJECT_NAME rhd_datastream)
add_executable(${RHD_PROJECT_NAME} ${RHD_SRC})
set_target_properties(${RHD_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_CXX_FLAGS "-fpermissive -std=c++11")

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_C_FLAGS_DEBUGOPT "")

#set warnings
if (MSVC)
    target_compile_options(${RHD_PROJECT_NAME} PRIVATE /W4 /WX /wd4068)
else ()
    target_compile_options(${RHD_PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${RHD_PROJECT_NAME} PRIVATE "-DDEBUG_BUILD")
endif ()

target_include_directories(${RHD_PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

find_package(aws-crt-cpp REQUIRED)
find_library(JSONCPP_LIBS NAMES jsoncpp PATHS ${LIBDIR})
find_library(REDISPLUSPLUS_LIBS NAMES redis++ PATHS ${LIBDIR})
find_library(HIREDIS_LIBS NAMES hiredis PATHS ${LIBDIR})
find_library(BCM2835 NAMES bcm2835 PATHS ${LIBDIR})

set(DEP_AWS_LIBS ${DEP_AWS_LIBS} AWS::aws-crt-cpp)

target_link_libraries(${RHD_PROJECT_NAME} ${DEP_AWS_LIBS}
					  ${JSONCPP_LIBS}
					  ${REDISPLUSPLUS_LIBS}
					  ${HIREDIS_LIBS}
					  ${CMAKE_THREAD_LIBS_INIT}
					  ${BCM2835})


