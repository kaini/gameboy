
set (SOURCES z80_test.cpp)
set (HEADERS)
find_package(Boost 1.57.0 REQUIRED)
include_directories (../gameboy_lib ${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})
add_executable (gameboy_test ${SOURCES} ${HEADERS})
target_link_libraries (gameboy_test gameboy_lib ${Boost_LIBRARIES})

