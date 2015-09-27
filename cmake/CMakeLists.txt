cmake_minimum_required (VERSION 2.8)

set (PROJECT_NAME executor)

project(${PROJECT_NAME})

set(ROOT_DIR "../src")

include_directories("${ROOT_DIR}/include")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	add_definitions("/Zi /W4")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall -Wextra")
endif ()

set(
	SOURCES
	"${ROOT_DIR}/test/main.cpp"
)

set(
	HEADERS
	"${ROOT_DIR}/include/concurrent_queue.hpp"
	"${ROOT_DIR}/include/utils.hpp"
	"${ROOT_DIR}/include/executor.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

enable_testing()

add_test(executor ${PROJECT_NAME})
