cmake_minimum_required (VERSION 2.8)

project(executor)

set(ROOT_DIR "../src")

include_directories("${ROOT_DIR}/include")

set(
	SOURCES
	"${ROOT_DIR}/test/main.cpp"
)

set(
	HEADERS
	"${ROOT_DIR}/include/ConcurrentQueue.hpp"
)

add_executable(executor ${SOURCES} ${HEADERS})
