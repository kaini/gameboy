#include "rom.hpp"
#include <algorithm>
#include <array>

namespace
{

const std::array<uint8_t, 0x30> nintendo_logo{{
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
}};

}

gb::rom::rom(std::vector<uint8_t> data_move_from) :
	_data(std::move(data_move_from))
{
	if (_data.size() < 0x8000)  // TODO 0x4000 (?)
		throw rom_error("The ROM-file is too small. (" + std::to_string(_data.size()) + ")");

	_valid_logo = std::equal(nintendo_logo.begin(), nintendo_logo.end(), _data.begin() + 0x104);

	const int count = _data[0x14B] == 0x33 ? 11 : 15;
	_title = std::string(_data.begin() + 0x134, _data.begin() + 0x134 + count);

	if (_data[0x14B] == 0x33)
	{
		uint8_t manufacturer[] = { _data[0x13F], _data[0x140], _data[0x141], _data[0x142], '\0' };
		_manufacturer = reinterpret_cast<char *>(manufacturer);
	}

	_gbc = _data[0x0143] == 0xC0 || _data[0x0143] == 0x80;

	if (_data[0x14B] == 0x33)
	{
		uint8_t license[] = {_data[0x144], _data[0x145], '\0'};
		_license = reinterpret_cast<char *>(license);
	}
	else
	{
		char buffer[16];
		sprintf(buffer, "%02X (old)", static_cast<int>(_data[0x14B]));
		_license = buffer;
	}

	_sgb = _data[0x146] == 0x03;

	_cartridge = _data[0x147];

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

	_japanese = _data[0x14A] == 0x00;

	_rom_version = _data[0x14C];

	_header_checksum = _data[0x14D];

	_global_checksum = _data[0x14E] << 8 | _data[0x14F];
}
