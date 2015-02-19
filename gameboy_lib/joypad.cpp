#include "joypad.hpp"
#include "bits.hpp"

gb::joypad::joypad() :
	_arrows_select(false),
	_buttons_select(false),
	_arrows(0xF),
	_buttons(0xF)
{
}

bool gb::joypad::read8(uint16_t addr, uint8_t &value) const
{
	if (addr == 0xFF00)
	{
		uint8_t v = 0x0F;
		if (_arrows_select) v &= _arrows;
		if (_buttons_select) v &= _buttons;
		if (!_arrows_select) bit::set(v, 1 << 4);
		if (!_buttons_select) bit::set(v, 1 << 5);

		value = v;
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::joypad::write8(uint16_t addr, uint8_t value)
{
	if (addr == 0xFF00)
	{
		_arrows_select = !bit::test(value, 1 << 4);
		_buttons_select = !bit::test(value, 1 << 5);
		return true;
	}
	else
	{
		return false;
	}
}

void gb::joypad::down(key key)
{
	const auto k = static_cast<int>(key);
	bit::clear(k / 4 == 0 ? _arrows : _buttons, 1 << (k % 4));
	// TODO DMG interrupt
}

void gb::joypad::up(key key)
{
	const auto k = static_cast<int>(key);
	bit::set(k / 4 == 0 ? _arrows : _buttons, 1 << (k % 4));
}
