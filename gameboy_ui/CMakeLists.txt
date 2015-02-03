
set (SOURCES gameboy_ui.cpp main.cpp)
set (HEADERS gameboy_ui.hpp)
set (UIS gameboy_ui.ui)
set (RESOURCES gameboy_ui.qrc gameboy_ui.rc gameboy_ui.ico)

source_group ("UI Files" FILES ${UIS} ${RESOURCES})
source_group ("Generated Files" REGULAR_EXPRESSION
              "(.*_automoc\\.cpp)|(qrc_.*\\.cpp)")

set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)
find_package (Qt5Widgets REQUIRED)

include_directories (../gameboy_lib)

if (WIN32)
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /subsystem:windows /entry:mainCRTStartup")

endif (WIN32)



add_executable (gameboy_ui ${SOURCES} ${HEADERS} ${UIS} ${RESOURCES})

target_link_libraries(gameboy_ui gameboy_lib Qt5::Widgets)
