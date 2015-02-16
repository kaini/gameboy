
set (SOURCES cart_mbc1.cpp cart_rom_only.cpp debug.cpp gb_thread.cpp
             internal_ram.cpp joypad.cpp memory.cpp rom.cpp timer.cpp
             video.cpp z80.cpp z80opcodes.cpp cart_mbc5.cpp sound.cpp)
set (HEADERS cart_mbc1.hpp cart_rom_only.hpp debug.hpp gb_thread.hpp
             internal_ram.hpp joypad.hpp memory.hpp rom.hpp timer.hpp
             video.hpp z80.hpp z80opcodes.hpp bits.hpp cart_mbc5.hpp
			 sound.hpp assert.hpp time.hpp)
add_definitions (-D_CRT_SECURE_NO_WARNINGS)
add_library (gameboy_lib ${SOURCES} ${HEADERS})

