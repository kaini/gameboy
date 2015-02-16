set (INCLUDES cgb_hardware.i gb_hardware.i pocket_voice.i)
set (ASMS iram_test.s)

find_program (WLAGB wla-gb)
if (NOT WLAGB)
	message (FATAL_ERROR "wla-gb executable not found")
endif ()

find_program (WLALINK wlalink)
if (NOT WLALINK)
	message (FATAL_ERROR "wlalink executable not found")
endif ()

set (INCLUDES_ABSOLUTE)
foreach (INCLUDE ${INCLUDES})
	list (APPEND INCLUDES_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE}")
endforeach ()

set (GB_FILES)
foreach (ASM ${ASMS})
	string (REGEX REPLACE ".s$" "" NAME ${ASM})
	file (WRITE "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.link"
	            "[objects]\n${CMAKE_CURRENT_BINARY_DIR}/${NAME}.o\n")
	add_custom_command (OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.gb"
	                    COMMAND "${WLAGB}" -o "${CMAKE_CURRENT_SOURCE_DIR}/${ASM}" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.o"
						COMMAND "${WLALINK}" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.link" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.gb"
	                    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${ASM}"
	                    DEPENDS ${INCLUDES_ABSOLUTE} "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.link"
	                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                    VERBATIM)
	list (APPEND GB_FILES "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.gb")
endforeach ()

add_custom_target (test_roms ALL DEPENDS ${GB_FILES} SOURCES ${INCLUDES})
