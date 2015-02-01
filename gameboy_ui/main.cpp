#include "gameboy_ui.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	gameboy_ui w;
	w.show();
	return a.exec();
}
