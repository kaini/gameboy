#include "main_window.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("pushrax");
	QCoreApplication::setOrganizationDomain("pushrax.com");
	QCoreApplication::setApplicationName("Gameboy");
	QApplication a(argc, argv);
	main_window w;
	w.show();
	return a.exec();
}
