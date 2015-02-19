cmake_minimum_required (VERSION 3.1.1)

project (GAMEBOY)

if (CMAKE_COMPILER_IS_GNUCXX)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif ()

add_subdirectory (gameboy_lib)
add_subdirectory (gameboy_test)
add_subdirectory (gameboy_ui)
add_subdirectory (test_roms)
