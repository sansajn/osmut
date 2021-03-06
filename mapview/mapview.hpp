#pragma once
#include <array>
#include <memory>
#include <vector>
#include <gtkmm.h>
#include <gdkmm.h>
#include <cairomm/cairomm.h>
#include <sigc++/sigc++.h>
#include <gdk/gdkkeysyms.h>
#include <glm/glm.hpp>
#include "tile_source.hpp"
#include "mapview_layer.hpp"


//! Tile based map view widget.
class mapview : public Gtk::DrawingArea
{
public:
	mapview();
	size_t zoom_level() const;
	std::array<glm::uvec2, 2> visible_tiles() const;
	void add_layer(mapview_layer * l);

	// TODO:
	// chcem aby mapview poskytoval funkcie umoznujuce jednoduchu implementaciu motion a button_press event
	// zoom_in|_out signals

private:
	bool on_draw(Cairo::RefPtr<Cairo::Context> const & cr) override;
	void on_size_allocate(Gtk::Allocation & alloc) override;
	bool on_motion_notify_event(GdkEventMotion * event) override;
	bool on_button_press_event(GdkEventButton * event) override;
	bool on_key_press_event(GdkEventKey * event) override;

	void force_redraw();

	std::vector<mapview_layer *> _layers;

	size_t _zoom;  //!< map zoom level
	glm::dvec2 _origin_pos;
	glm::dvec2 _button_press_pos;
};
