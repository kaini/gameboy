#include "video.hpp"
#include "debug.hpp"
#include "z80.hpp"
#include <algorithm>

gb::video::video() :
	_vram_bank(0),
	_mode_time(0),
	_hblanks(0),
	_check_ly(false)
{
	std::fill(_registers.begin(), _registers.end(), 0);
	for (size_t i = 0; i < _vram.size(); ++i)
		std::fill(_vram[i].begin(), _vram[i].end(), 0);
	std::fill(_sprite_attribs.begin(), _sprite_attribs.end(), 0);
	std::fill(_bgp.begin(), _bgp.end(), 0xff);  // all white
	std::fill(_obp.begin(), _obp.end(), 0);
	for (auto &row : _image)
		for (auto &col : row)
		{
			col[0] = 0xFF;
			col[1] = 0xFF;
			col[2] = 0xFF;
		}

	// starting mode
	access_register(r::stat) = mode::vblank;
	_mode_time = 1080000 - 1;
	access_register(r::ly) = 153;
}

bool gb::video::read8(uint16_t addr, uint8_t &value) const
{
	if (0x8000 <= addr && addr < 0xA000)
	{
		value = _vram[_vram_bank][addr - 0x8000];
		return true;
	}
	else if (0xFE00 <= addr && addr < 0xFEA0)
	{
		value = _sprite_attribs[addr - 0xFE00];
		return true;
	}
	else if (is_register(addr))
	{
		switch (addr)
		{
		case r::bgpd:
			value = _bgp[access_register(r::bgpi) & 0x3F];
			break;
		case r::obpd:
			value = _obp[access_register(r::obpi) & 0x3F];
			break;
		default:
			value = access_register(addr);
			break;
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::video::write8(uint16_t addr, uint8_t value)
{
	if (0x8000 <= addr && addr < 0xA000)
	{
		if ((access_register(r::stat) & stat_flag::mode) == 3)
		{
			debug("WARNING: write in VRAM during mode 3");
		}
		//else
		{
			_vram[_vram_bank][addr - 0x8000] = value;
		}
		return true;
	}
	else if (0xFE00 <= addr && addr < 0xFEA0)
	{
		if ((access_register(r::stat) & stat_flag::mode) >= 2)
		{
			debug("WARNING: write in OAM during mode 2 or 3");
		}
		//else
		{
			_sprite_attribs[addr - 0xFE00] = value;
		}
		return true;
	}
	else if (is_register(addr))
	{
		switch (addr)
		{
		case r::vbk:
			_vram_bank = value & 0x01;
			access_register(r::vbk) = value;
			break;
		case r::bgpd:
			if ((access_register(r::stat) & stat_flag::mode) == 3)
			{
				debug("WARNING: write to BGP in mode 3");
			}
			else
			{
				uint8_t r = access_register(r::bgpi);
				_bgp[r & 0x3F] = value;
				if ((r & 0x80) != 0)
					r = 0x80 | (((r & 0x3F) + 1) % _bgp.size());
				access_register(r::bgpi) = r;
			}
			break;
		case r::obpd:
			if ((access_register(r::stat) & stat_flag::mode) == 3)
			{
				debug("WARNING: write to OBP in mode 3");
			}
			else
			{
				uint8_t r = access_register(r::obpi);
				_obp[r & 0x3F] = value;
				if ((r & 0x80) != 0)
					r = 0x80 | (((r & 0x3F) + 1) % _obp.size());
				access_register(r::obpi) = r;
			}
			break;
		case r::stat:
			value &= ~0x07;
			value |= access_register(r::stat) & 0x07;
			access_register(r::stat) = value;
			break;
		case r::ly:
			// LY is read only
			break;
		case r::lyc:
			access_register(r::lyc) = value;
			_check_ly = true;
			break;
		case r::dma:
		case r::hdma1:
		case r::hdma2:
		case r::hdma3:
		case r::hdma4:
		case r::hdma5:
			break;  // TODO DMA
		default:
			access_register(addr) = value;
			break;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::video::is_register(uint16_t addr)
{
	return
		   0xFF40 <= addr && addr <= 0xFF4B
		|| addr == 0xFF4F
		|| 0xFF51 <= addr && addr <= 0xFF50
		|| 0xFF68 <= addr && addr <= 0xFF6B;
}

uint8_t &gb::video::access_register(uint16_t addr)
{
	return _registers[addr - 0xFF40];
}

const uint8_t &gb::video::access_register(uint16_t addr) const
{
	return _registers[addr - 0xFF40];
}

void gb::video::tick(gb::z80_cpu &cpu, int ns)
{
	if ((access_register(r::lcdc) & lcdc_flag::lcd_enable) == 0)
	{
		access_register(r::stat) &= ~(stat_flag::mode | stat_flag::coincidence);
		access_register(r::stat) |= mode::vblank;
		_mode_time = 1080000 - 1;
		_hblanks = 0;
		_vblank_ly_time = 0;
		return;
	}

	if (_check_ly)
	{
		_check_ly = false;
		set_ly(cpu, access_register(r::ly));
	}

	uint8_t current_mode = access_register(r::stat) & stat_flag::mode;
	uint8_t next_mode = current_mode;
	_mode_time += ns;
	
	switch (current_mode)
	{
	case mode::read_oam:
		if (_mode_time > 19000)
		{
			_mode_time -= 19000;
			next_mode = mode::read_vram;
		}
		break;
	case mode::read_vram:
		if (_mode_time > 41000)
		{
			_mode_time -= 41000;
			next_mode = mode::hblank;
		}
		break;
	case mode::hblank:
		if (_mode_time > 48600)
		{
			_mode_time -= 48600;
			++_hblanks;
			if (_hblanks == 144)
			{
				next_mode = mode::vblank;
			}
			else
			{
				next_mode = mode::read_oam;
			}
		}
		break;
	case mode::vblank:
		if (_mode_time > 1080000)
		{
			_mode_time -= 1080000;
			next_mode = mode::read_oam;
		}
		else
		{
			_vblank_ly_time += ns;
			if (_vblank_ly_time > 106600)
			{
				_vblank_ly_time -= 106600;
				set_ly(cpu, access_register(r::ly) + 1);
			}
		}
		break;
	}

	if (current_mode != next_mode)
	{
		switch (next_mode)
		{
		case mode::read_oam:
			if (current_mode == mode::vblank)
			{
				_hblanks = 0;
				set_ly(cpu, 0);
			}
			else
			{
				set_ly(cpu, access_register(r::ly) + 1);
			}
			break;
		case mode::read_vram:
			break;
		case mode::hblank:
			if ((access_register(r::stat) & stat_flag::hblank_int) != 0)
				cpu.post_interrupt(interrupt::lcdc);
			draw_line(access_register(r::ly));
			break;
		case mode::vblank:
			if ((access_register(r::stat) & stat_flag::vblank_int) != 0)
				cpu.post_interrupt(interrupt::vblank);
			_vblank_ly_time = 0;
			break;
		}

		access_register(r::stat) &= ~0x03;
		access_register(r::stat) |= next_mode;
	}

	// TODO oam interrupt?
}

void gb::video::draw_line(const int y)
{
	// debug("DRAWING line ", y);

	const int scy = access_register(r::scy);
	const int scx = access_register(r::scx);
	const bool bg_enabled = access_register(r::lcdc) & lcdc_flag::bg_display;
	const uint8_t *bg_tile_map;
	const uint8_t *bg_tile_map_attrs;
	if ((access_register(r::lcdc) & lcdc_flag::bg_tile_map_select) == 0)
	{
		bg_tile_map = &_vram[0][0x9800 - 0x8000];
		bg_tile_map_attrs = &_vram[1][0x9800 - 0x8000];
	}
	else
	{
		bg_tile_map = &_vram[0][0x9C00 - 0x8000];
		bg_tile_map_attrs = &_vram[1][0x9C00 - 0x8000];
	}

	for (int x = 0; x < width; ++x)
	{
		image(x, y) = {255, 255, 255};

		if (bg_enabled)
		{
			const size_t map_index = (((y + scy) / 8) % 32) * 32 + ((x + scx) / 8) % 32;
			const auto tile_idx = bg_tile_map[map_index];
			const auto tile_attrs = bg_tile_map_attrs[map_index];
			const auto bgp_idx = tile_attrs & 0x07;
			const auto tile_vram_bank = (tile_attrs & 0x08) == 0 ? 0 : 1;
			// TODO hflip
			// TODO vflip
			// TODO priority
			const uint8_t *tile = get_bg_tile(tile_vram_bank, tile_idx);
			const size_t tile_line_index = ((y + scy) % 8) * 2;
			const size_t tile_x_bit = 7 - (x + scx) % 8;
			size_t color_idx = 0;
			if ((tile[tile_line_index] & (1 << tile_x_bit)) != 0)
				color_idx |= 1;
			if ((tile[tile_line_index + 1] & (1 << tile_x_bit)) != 0)
				color_idx |= 2;
			image(x, y) = get_bg_color(bgp_idx, color_idx);
		}
	}
}

void gb::video::set_ly(z80_cpu &cpu, uint8_t value)
{
	access_register(r::ly) = value;
	if (value == access_register(r::lyc))
	{
		if ((access_register(r::stat) & stat_flag::coincidence_int) != 0)
		{
			cpu.post_interrupt(interrupt::lcdc);
		}
		access_register(r::stat) |= stat_flag::coincidence;
	}
	else
	{
		access_register(r::stat) &= ~stat_flag::coincidence;
	}
}

const uint8_t *gb::video::get_bg_tile(uint8_t bank, uint8_t idx) const
{
	if ((access_register(r::lcdc) & lcdc_flag::bg_window_data_select) == 0)
	{
		return &_vram[bank][0x1000 + static_cast<int>(static_cast<int8_t>(idx)) * 16];
	}
	else
	{
		return &_vram[bank][static_cast<size_t>(idx) * 16];
	}
}

static uint8_t map_color(double c, double n)
{
	return static_cast<uint8_t>((c + n)/ 2.0 * 255.0);
}

const std::array<uint8_t, 3> gb::video::get_bg_color(size_t bgp_idx, size_t color_idx) const
{
	const auto color_ptr = &_bgp[bgp_idx * 8 + color_idx * 2];

	double r = color_ptr[0] & 0x1F;
	r = r / 0x1F;

	double g = ((color_ptr[1] & 0x03) << 3) | ((color_ptr[0] & 0xE0) >> 5);
	g = g / 0x1F;

	double b = (color_ptr[1] & 0x7C) >> 2;
	b = b / 0x1F;

	return {map_color(r, 0.6), map_color(g, 0.6), map_color(b, 0.6)};
}
