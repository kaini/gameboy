#include "rom.hpp"

gb::rom::rom(std::vector<uint8_t> data_move_from) :
	_data(std::move(data_move_from))
{
	if (_data.size() < 0x8000)  // TODO 0x4000 (?)
		throw rom_error("The ROM-file is too small. (" + std::to_string(_data.size()) + ")");

	_gbc = _data[0x0143] >= 0x80;
	_sgb = _data[0x0146] == 0x03;
	_cartridge = _data[0x0147];

	uint8_t raw_rom_size = _data[0x0148];
	if (raw_rom_size <= 7)
		_rom_size = static_cast<size_t>(32 * 1024) << raw_rom_size;
	else if (raw_rom_size == 0x52)
		_rom_size = 16 * 1024 * 72;
	else if (raw_rom_size == 0x53)
		_rom_size = 16 * 1024 * 80;
	else if (raw_rom_size == 0x54)
		_rom_size = 16 * 1024 * 96;
	else
		throw rom_error("The ROM has an invalid ROM size field. (" + std::to_string(raw_rom_size) + ")");

	uint8_t raw_ram_size = _data[0x0149];
	switch (raw_ram_size)
	{
	case 0:
		_ram_size = 0;
		break;
	case 1:
		_ram_size = 2 * 1024;
		break;
	case 2:
		_ram_size = 8 * 1024;
		break;
	case 3:
		_ram_size = 32 * 1024;
		break;
	default:
		throw rom_error("The ROM has an invalid ram size field. (" + std::to_string(raw_ram_size) + ")");
	}
}
