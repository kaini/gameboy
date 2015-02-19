#pragma once
#include <QMainWindow>
#include <memory>
#include <rom.hpp>
#include <gb_thread.hpp>
#include <unordered_map>
#include "ui_game_window.h"

class game_window : public QMainWindow
{
	Q_OBJECT

public:
	using key_map = std::unordered_map<int, gb::key>;

	/**
	 * The key_map has to life as long as this!
	 * It will only be accessed from the Qt event loop.
	 */
	game_window(gb::rom rom, std::shared_ptr<const key_map> map, QWidget *parent = 0);
	~game_window();

protected:
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;

private:
	Ui::gameWindowUiClass _ui;
	gb::gb_thread _thread;
	std::shared_ptr<const key_map> _keys;
};
