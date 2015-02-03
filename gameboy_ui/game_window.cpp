#include "game_window.hpp"
#include <QPainter>
#include <QTimer>

game_window::game_window(gb::rom rom, QWidget *parent) :
	QMainWindow(parent)
{
	_ui.setupUi(this);
	resize(gb::video::width * 2, gb::video::height * 2);

	auto _refresh_timer = new QTimer(this);
	_refresh_timer->setInterval(1000 / 60 / 2);  // TODO better method for this?
	_refresh_timer->setSingleShot(false);
	connect(_refresh_timer, SIGNAL(timeout()), this, SLOT(update_image()));

	_thread.start(std::move(rom));
	_refresh_timer->start();
}

game_window::~game_window()
{
}

void game_window::update_image()
{
	// TODO check for errors in the thread
	update();
}

void game_window::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setBrush(Qt::black);
	painter.setPen(Qt::black);
	painter.drawRect(rect());

	const auto image = _thread.fetch_image();
	if (image)
	{
		const QImage qImage(&(*image)[0][0][0], gb::video::width, gb::video::height, QImage::Format_RGB888);

		const double aspect = static_cast<double>(gb::video::height) / gb::video::width;
		const double real_aspect = static_cast<double>(height()) / width();
		QRect iRect;
		if (real_aspect < aspect)
		{
			iRect.setHeight(height());
			iRect.setWidth(static_cast<int>(std::round(1 / aspect * iRect.height())));
			iRect.moveLeft((width() - iRect.width()) / 2);
		}
		else
		{
			iRect.setWidth(width());
			iRect.setHeight(static_cast<int>(std::round(aspect * iRect.width())));
			iRect.moveTop((height() - iRect.height()) / 2);
		}
		
		painter.drawImage(iRect, qImage);
	}
}
