#pragma once
#include "memory.hpp"
#include <array>

namespace gb
{

class z80_cpu;

class video final : public memory_mapping
{
public:
	static const int width = 160;
	static const int height = 144;
	using raw_image = std::array<std::array<std::array<uint8_t, 3>, width>, height>;
	static_assert(sizeof(raw_image) == 3 * width * height, "raw_image has the wrong size");

	// Register memory addresses:
	//   FF40 to FF4B
	//   FF4F
	//   FF51 to FF55
	//   FF68 to FF6B
	struct r
	{
		/** LCD control */
		static const uint16_t lcdc = 0xff40;
		/** LCD status */
		static const uint16_t stat = 0xff41;
		/** Scroll Y */
		static const uint16_t scy  = 0xff42;
		/** Scroll X */
		static const uint16_t scx  = 0xff43;
		/** LCDC Y coordinate */
		static const uint16_t ly   = 0xff44;
		/** LY compare */
		static const uint16_t lyc  = 0xff45;
		/** DMA transfer start and address */
		static const uint16_t dma  = 0xff46;
		/** BG palette data */
		static const uint16_t bgp  = 0xff47;
		/** Object 0 palette data */
		static const uint16_t obp0 = 0xff48;
		/** Object 1 palette data */
		static const uint16_t obp1 = 0xff49;
		/** Window Y */
		static const uint16_t wy   = 0xff4a;
		/** Window X minus 7 */
		static const uint16_t wx   = 0xff4b;
		/** VRAM bank */
		static const uint16_t vbk  = 0xff4f;
		/** New DMA source high */
		static const uint16_t hdma1 = 0xff51;
		/** New DMA source low */
		static const uint16_t hdma2 = 0xff52;
		/** New DMA dest high */
		static const uint16_t hdma3 = 0xff53;
		/** New DMA dest low */
		static const uint16_t hdma4 = 0xff54;
		/** New DMA length/mode/start */
		static const uint16_t hdma5 = 0xff55;
		/** Background palette index */
		static const uint16_t bgpi = 0xff68;  // bcps
		/** Background palette data */
		static const uint16_t bgpd = 0xff69;  // bcpd
		/** Sprite palette index */
		static const uint16_t obpi = 0xff6a;  // ocps
		/** Sprite palette data */
		static const uint16_t obpd = 0xff6b;  // ocpd
	};

	struct lcdc_flag
	{
		static const uint8_t lcd_enable = 1 << 7;
		static const uint8_t window_tile_map_display_select = 1 << 6;
		static const uint8_t window_display_enable = 1 << 5;
		static const uint8_t bg_window_data_select = 1 << 4;
		static const uint8_t bg_tile_map_select = 1 << 3;
		static const uint8_t obj_size = 1 << 2;
		static const uint8_t obj_display_enable = 1 << 1;
		static const uint8_t bg_display = 1 << 0;
	};

	struct stat_flag
	{
		static const uint8_t coincidence_int = 1 << 6;
		static const uint8_t oam_int = 1 << 5;
		static const uint8_t vblank_int = 1 << 4;
		static const uint8_t hblank_int = 1 << 3;
		static const uint8_t coincidence = 1 << 2;
		static const uint8_t mode = (1 << 1) | (1 << 0);
	};

	struct mode
	{
		static const uint8_t hblank = 0;
		static const uint8_t vblank = 1;
		static const uint8_t read_oam = 2;
		static const uint8_t read_vram = 3;
	};

	video();

	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

	void tick(z80_cpu &cpu, double ns);

	bool is_enabled() const { return (access_register(r::lcdc) & lcdc_flag::lcd_enable) != 0; }
	const raw_image &image() const { return _image; }

private:
	static bool is_register(uint16_t addr);
	uint8_t &access_register(uint16_t addr);
	const uint8_t &access_register(uint16_t addr) const;
	void draw_line(const int line);
	void set_ly(z80_cpu &cpu, uint8_t value);
	std::array<uint8_t, 3> &image(int x, int y) { return _image[y][x]; }
	const uint8_t *get_bg_tile(uint8_t bank, uint8_t idx) const;
	const std::array<uint8_t, 3> get_bg_color(size_t bgp_idx, size_t color_idx) const;

	std::array<uint8_t, 0x30> _registers;
	std::array<std::array<uint8_t, 0x2000>, 2> _vram;
	std::array<uint8_t, 0xA0> _sprite_attribs;
	bool _check_ly;

	std::array<uint8_t, 0x40> _bgp;  // background palette (8 times 4 colors times 2 byte)
	std::array<uint8_t, 0x40> _obp;  // object/sprite palette (8 times 4 colors times 2 byte)

	int _vram_bank;

	raw_image _image;
	double _mode_time;
	double _vblank_ly_time;
	int _hblanks;
};

}

