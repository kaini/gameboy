#pragma once
#include "gb_thread.hpp"
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/dcmemory.h>
#include <wx/bitmap.h>
#include <memory>

namespace gb
{
class video;
}

namespace ui
{

class game_frame;

class my_app : public wxApp
{
public:
	bool OnInit() override;
};

class main_frame : public wxFrame
{
public:
	main_frame();

private:
	void on_exit(wxCommandEvent &event);
	void on_start(wxCommandEvent &event);

	std::unique_ptr<gb::gb_thread> _thread;
	game_frame *_game_frame;

	wxDECLARE_EVENT_TABLE();
};

class game_frame : public wxFrame
{
public:
	game_frame(wxWindow *parent, gb::gb_thread *thread);

private:
	void on_paint(wxPaintEvent &event);
	void on_idle(wxIdleEvent &event);

	gb::gb_thread *const _thread;

	wxDECLARE_EVENT_TABLE();
};

}
