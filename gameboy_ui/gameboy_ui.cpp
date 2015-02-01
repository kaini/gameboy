#include "gameboy_ui.hpp"
#include <rom.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

gameboy_ui::gameboy_ui(QWidget *parent)
	: QMainWindow(parent)
{
	_ui.setupUi(this);

	connect(_ui.actionExit, SIGNAL(triggered()), this, SLOT(exit()));
	connect(_ui.loadRomPushButton, SIGNAL(clicked()), this, SLOT(open_rom()));

	update_rom_info();
}

gameboy_ui::~gameboy_ui()
{
}

void gameboy_ui::exit()
{
	close();
}

void gameboy_ui::open_rom()
{
	auto fileName = QFileDialog::getOpenFileName(this,
		tr("Select ROM File"), "",
		tr("ROM files (*.gb *.gbc *.rom *.bin);;All files (*.*)"));
	if (fileName.isEmpty())
		return;

	std::ifstream in;
	in.open(fileName.toStdString(), std::ios::binary);
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

void gameboy_ui::update_rom_info()
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
		_ui.headerOkCheckBox->setChecked(true);  // TODO
		_ui.globalOkCheckBox->setChecked(true);  // TODO
	}
}
