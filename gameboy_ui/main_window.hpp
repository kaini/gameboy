#pragma once
#include <QMainWindow>
#include <QSettings>
#include <memory>
#include <rom.hpp>
#include "ui_main_window.h"

class game_window;
class QCloseEvent;

class main_window : public QMainWindow
{
	Q_OBJECT

public:
	main_window(QWidget *parent = 0);
	~main_window();

protected:
	void closeEvent(QCloseEvent *event) override;

private slots:
	void open_rom();
	void play();
	void set_game_window_null();

private:
	void update_rom_info();

	QSettings _settings;
	Ui::mainWindowUiClass _ui;
	std::unique_ptr<gb::rom> _rom;
	game_window *_game_window;
};
