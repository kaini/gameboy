#include "main_window.hpp"
#include <debug.hpp>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/dcbuffer.h>
#include <fstream>
#include <wx/image.h>

enum
{
	ID_start = 1,
};

bool ui::my_app::OnInit()
{
	main_frame *frame = new main_frame;
	frame->Show(true);
	return true;
}

ui::main_frame::main_frame() :
	wxFrame(nullptr, wxID_ANY, "GameBoy"),
	_game_frame(nullptr)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_start, "&Load Game...\tCtrl-O");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");

	SetMenuBar(menuBar);
}

void ui::main_frame::on_exit(wxCommandEvent &)
{
	Close(true);
}

void ui::main_frame::on_start(wxCommandEvent &)
{
	if (_game_frame)
		return;

	// Load ROM
	std::vector<uint8_t> rom_data;
	std::ifstream in;
	in.open("cpu_instrs.gb", std::ios_base::binary | std::ios_base::in);
	while (in.good())
	{
		char c;
		in.read(&c, 1);
		if (in.good())
			rom_data.emplace_back(c);
	}
	in.close();

	// Make ROM
	gb::rom rom(std::move(rom_data));
	debug("ROM: GBC=", rom.gbc(), " SGB=", rom.sgb(), " TYPE=",
		static_cast<int>(rom.cartridge()), " SIZE=", rom.rom_size(),
		" RAM_BYTES=", rom.ram_size());

	// Run it
	_thread = std::make_unique<gb::gb_thread>();
	_thread->start(std::move(rom));

	_game_frame = new game_frame(this, _thread.get());
	_game_frame->Show();
}

wxBEGIN_EVENT_TABLE(ui::main_frame, wxFrame)
	EVT_MENU(wxID_EXIT, ui::main_frame::on_exit)
	EVT_MENU(ID_start, ui::main_frame::on_start)
wxEND_EVENT_TABLE()

ui::game_frame::game_frame(wxWindow *parent, gb::gb_thread *thread) :
	wxFrame(parent, wxID_ANY, "GameBoy Game", wxDefaultPosition,
		wxSize(gb::video::width + 30, gb::video::height + 40)),
	_thread(thread)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Connect(wxEVT_IDLE, wxIdleEventHandler(ui::game_frame::on_idle));
}

void ui::game_frame::on_paint(wxPaintEvent &)
{
	auto image_bytes = _thread->fetch_image();
	wxImage image_image(gb::video::width, gb::video::height, &image_bytes[0][0][0], true);
	wxBitmap image_bitmap(image_image);
	wxMemoryDC image_dc;
	image_dc.SelectObject(image_bitmap);

	wxBufferedPaintDC dc(this);
	dc.Blit(wxPoint(0, 0), wxSize(gb::video::width, gb::video::height), &image_dc, wxPoint(0, 0));
}

void ui::game_frame::on_idle(wxIdleEvent &)
{
	Refresh(false);
}

wxBEGIN_EVENT_TABLE(ui::game_frame, wxFrame)
	EVT_PAINT(ui::game_frame::on_paint)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(ui::my_app);
