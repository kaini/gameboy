#include "game_window.hpp"
#include <QPainter>
#include <QTimer>
#include <QMessageBox>
#include <QKeyEvent>
#include <z80opcodes.hpp>
#include <debug.hpp>

game_window::game_window(gb::rom rom, std::shared_ptr<const key_map> map, QWidget *parent) :
	_keys(std::move(map)),
	QMainWindow(parent)
{
	_ui.setupUi(this);
	resize(gb::video::width * 2, gb::video::height * 2);
	setFocusPolicy(Qt::StrongFocus);

	auto _refresh_timer = new QTimer(this);
	_refresh_timer->setInterval(1000 / 60 / 2);  // TODO better method for this?
	_refresh_timer->setSingleShot(false);
	connect(_refresh_timer, SIGNAL(timeout()), this, SLOT(update()));

	_refresh_timer->start();
	_thread.start(std::move(rom));
}

game_window::~game_window()
{
}

void game_window::paintEvent(QPaintEvent *)
{
	auto image_future = _thread.post_get_image();

	QPainter painter(this);
	painter.setBrush(Qt::black);
	painter.setPen(Qt::black);
	painter.drawRect(rect());

	const auto image = image_future.get();
	const QImage q_image(&image[0][0][0], gb::video::width, gb::video::height, QImage::Format_RGB888);

	const double aspect = static_cast<double>(gb::video::height) / gb::video::width;
	const double real_aspect = static_cast<double>(height()) / width();
	QRect i_rect;
	if (real_aspect < aspect)
	{
		i_rect.setHeight(height());
		i_rect.setWidth(static_cast<int>(std::round(1 / aspect * i_rect.height())));
		i_rect.moveLeft((width() - i_rect.width()) / 2);
	}
	else
	{
		i_rect.setWidth(width());
		i_rect.setHeight(static_cast<int>(std::round(aspect * i_rect.width())));
		i_rect.moveTop((height() - i_rect.height()) / 2);
	}
		
	painter.drawImage(i_rect, q_image);
}

void game_window::keyPressEvent(QKeyEvent *event)
{
	auto iter = _keys->find(event->key());
	if (iter == _keys->end())
	{
		QMainWindow::keyPressEvent(event);
	}
	else
	{
		_thread.post_key_down(iter->second);
	}
}

void game_window::keyReleaseEvent(QKeyEvent *event)
{
	auto iter = _keys->find(event->key());
	if (iter == _keys->end())
	{
		QMainWindow::keyPressEvent(event);
	}
	else
	{
		_thread.post_key_up(iter->second);
	}
}
