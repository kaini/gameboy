#pragma once
#include <QMainWindow>
#include <memory>
#include <rom.hpp>
#include <gb_thread.hpp>
#include "ui_game_window.h"

class game_window : public QMainWindow
{
	Q_OBJECT

public:
	game_window(gb::rom rom, QWidget *parent = 0);
	~game_window();

public slots:
	void update_image();

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	Ui::gameWindowUiClass _ui;
	gb::gb_thread _thread;
};
