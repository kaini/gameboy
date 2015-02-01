#ifndef GAMEBOY_UI_HPP
#define GAMEBOY_UI_HPP

#include <QtWidgets/QMainWindow>
#include <memory>
#include <rom.hpp>
#include "ui_gameboy_ui.h"

class gameboy_ui : public QMainWindow
{
	Q_OBJECT

public:
	gameboy_ui(QWidget *parent = 0);
	~gameboy_ui();

private slots:
	void exit();
	void open_rom();

private:
	void update_rom_info();

	Ui::gameboy_uiClass _ui;
	std::unique_ptr<gb::rom> _rom;
};

#endif // GAMEBOY_UI_HPP
