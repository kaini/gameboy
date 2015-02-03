#include "main_window.hpp"
#include "game_window.hpp"
#include <rom.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <fstream>

main_window::main_window(QWidget *parent)
	: QMainWindow(parent), _game_window(nullptr)
{
	_ui.setupUi(this);

	connect(_ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(_ui.loadRomPushButton, SIGNAL(clicked()), this, SLOT(open_rom()));
	connect(_ui.playPushButton, SIGNAL(clicked()), this, SLOT(play()));

	update_rom_info();
}

main_window::~main_window()
{
}

void main_window::closeEvent(QCloseEvent *event)
{
	if (_game_window)
		_game_window->close();
	close();
	event->accept();
}

void main_window::open_rom()
{
	auto file_name = QFileDialog::getOpenFileName(this,
		tr("Select ROM File"), _settings.value("open_rom_path", "").toString(),
		tr("ROM files (*.gb *.gbc *.rom *.bin);;All files (*.*)"));
	if (file_name.isEmpty())
		return;
	_settings.setValue("open_rom_path", file_name);

	std::ifstream in;
	in.open(file_name.toStdString(), std::ios::binary);
	std::vector<uint8_t> bytes;
	while (in.good())
	{
		uint8_t byte;
		in.read(reinterpret_cast<char *>(&byte), 1);
		if (in.good())
		{
			bytes.emplace_back(byte);
		}
	}
	in.close();

	try
	{
		_rom = std::make_unique<gb::rom>(std::move(bytes));
	}
	catch (gb::rom_error rom_error)
	{
		_rom = nullptr;
		QMessageBox::critical(this, tr("Invalid ROM"),
			tr("The ROM file is not valid.\n\n") + rom_error.what());
	}

	update_rom_info();
}

void main_window::play()
{
	if (_game_window)
	{
		_game_window->setFocus(Qt::ActiveWindowFocusReason);
	}
	else
	{
		_ui.loadRomPushButton->setEnabled(false);
		_ui.playPushButton->setEnabled(false);

		_game_window = new game_window(*_rom);
		_game_window->setAttribute(Qt::WA_DeleteOnClose);
		connect(_game_window, SIGNAL(destroyed()), this, SLOT(set_game_window_null()));
		_game_window->show();
	}
}

void main_window::set_game_window_null()
{
	_game_window = nullptr;
	_ui.loadRomPushButton->setEnabled(true);
	_ui.playPushButton->setEnabled(true);
}

void main_window::update_rom_info()
{
	if (_rom == nullptr)
	{
		_ui.playPushButton->setEnabled(false);
		_ui.titleLineEdit->setText("");
		_ui.manufacturerLineEdit->setText("");
		_ui.licenseLineEdit->setText("");
		_ui.versionLineEdit->setText("");
		_ui.cartridgeLineEdit->setText("");
		_ui.romLineEdit->setText("");
		_ui.ramLineEdit->setText("");
		_ui.gbcCheckBox->setChecked(false);
		_ui.sgbCheckBox->setChecked(false);
		_ui.jpCheckBox->setChecked(false);
		_ui.logoOkCheckBox->setChecked(false);
		_ui.headerOkCheckBox->setChecked(false);
		_ui.globalOkCheckBox->setChecked(false);
	}
	else
	{
		_ui.playPushButton->setEnabled(true);  // TODO actually check this!
		_ui.titleLineEdit->setText(QString::fromStdString(_rom->title()));
		_ui.manufacturerLineEdit->setText(QString::fromStdString(_rom->manufacturer()));
		_ui.licenseLineEdit->setText(QString::fromStdString(_rom->license()));
		_ui.versionLineEdit->setText(QString().setNum(_rom->rom_version()));
		_ui.cartridgeLineEdit->setText(QString().setNum(_rom->cartridge()));
		_ui.romLineEdit->setText(QString().setNum(_rom->rom_size()));
		_ui.ramLineEdit->setText(QString().setNum(_rom->ram_size()));
		_ui.gbcCheckBox->setChecked(_rom->gbc());
		_ui.sgbCheckBox->setChecked(_rom->sgb());
		_ui.jpCheckBox->setChecked(_rom->japanese());
		_ui.logoOkCheckBox->setChecked(_rom->valid_logo());
		_ui.headerOkCheckBox->setChecked(_rom->header_checksum_valid());
		_ui.globalOkCheckBox->setChecked(_rom->global_checksum_valid());
	}
}
