#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

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

	bool valid_logo() const { return _valid_logo; }
	const std::string &title() const { return _title; }
	const std::string &manufacturer() const { return _manufacturer; }
	bool gbc() const { return _gbc; }
	const std::string &license() const { return _license; }
	bool sgb() const { return _sgb; }
	uint8_t cartridge() const { return _cartridge; }
	size_t rom_size() const { return _rom_size; }
	size_t ram_size() const { return _ram_size; }
	bool japanese() const { return _japanese; }
	int rom_version() const { return _rom_version; }
	uint8_t header_checksum() const { return _header_checksum; }
	uint16_t global_checksum() const { return _global_checksum; }
	bool header_checksum_valid() const;
	bool global_checksum_valid() const;

private:
	std::vector<uint8_t> _data;

	bool _valid_logo;
	std::string _title;
	std::string _manufacturer;
	bool _gbc;
	std::string _license;
	bool _sgb;
	uint8_t _cartridge;
	size_t _rom_size;
	size_t _ram_size;
	bool _japanese;
	int _rom_version;
	uint8_t _header_checksum;
	uint16_t _global_checksum;
};

}
