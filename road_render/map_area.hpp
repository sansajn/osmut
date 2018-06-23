#pragma once
#include <gtkmm/drawingarea.h>
#include "osm_map.hpp"

class map_area : public Gtk::DrawingArea
{
public:
	map_area(osm_map & osm, unsigned w, unsigned h)
		: _osm{osm}
	{
		set_size_request(w, h);
	}

private:
	bool on_draw(Cairo::RefPtr<Cairo::Context> const & cr) override;

	osm_map & _osm;
	Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
};
