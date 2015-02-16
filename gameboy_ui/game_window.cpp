#include "game_window.hpp"
#include <QPainter>
#include <QTimer>
#include <z80opcodes.hpp>
#include <debug.hpp>

game_window::game_window(gb::rom rom, QWidget *parent) :
	QMainWindow(parent)
{
	_ui.setupUi(this);
	resize(gb::video::width * 2, gb::video::height * 2);

	auto _refresh_timer = new QTimer(this);
	_refresh_timer->setInterval(1000 / 60 / 2);  // TODO better method for this?
	_refresh_timer->setSingleShot(false);
	connect(_refresh_timer, SIGNAL(timeout()), this, SLOT(update()));

	_thread.start(std::move(rom));
	_refresh_timer->start();

	QString str;
	QString str2;
	for (int i = 0; i < 0x100; ++i) {
		if (i % 0x10 == 0 && i != 0) {
			str.append("\n");
			str2.append("\n");
		}
		str.append(QString("%1,").arg(gb::opcodes[i].cycles() / 4));
		str2.append(QString("%1,").arg((gb::opcodes[i].cycles() + gb::opcodes[i].extra_cycles()) / 4));
	}
	debug(str.toStdString());
	debug("");
	debug(str2.toStdString());
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
