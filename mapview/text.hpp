// text layout support
#pragma once
#include <pangomm.h>
#include <gtkmm/widget.h>

class text_layout
{
public:
	static text_layout & ref();
	static void init(Gtk::Widget * w);
	Glib::RefPtr<Pango::Layout> create_layout(std::string const & s);
};
