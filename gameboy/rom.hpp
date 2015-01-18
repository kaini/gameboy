#pragma once
#include <vector>
#include <string>

namespace gb
{

class rom_error : public std::runtime_error
{
public:
	rom_error(const std::string &msg) : std::runtime_error(msg) {}
};

class rom
{
public:
	/** Parses a rom and throws an error when the rom is invalid. */
	rom(std::vector<uint8_t> data);

	const std::vector<uint8_t> &data() const { return _data; }
	bool gbc() const { return _gbc; }
	bool sgb() const { return _sgb; }
	uint8_t cartridge() const { return _cartridge; }
	size_t rom_size() const { return _rom_size; }
	size_t ram_size() const { return _ram_size; }

private:
	std::vector<uint8_t> _data;
	bool _gbc, _sgb;
	uint8_t _cartridge;
	size_t _rom_size;
	size_t _ram_size;
};

}
