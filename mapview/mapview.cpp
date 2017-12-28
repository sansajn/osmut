#include <utility>
#include <vector>
#include <cmath>
#include "mapview.hpp"

using std::string;
using std::array;
using std::pair;
using std::move;
using std::vector;
using std::unique_ptr;
//using std::shared_ptr;
using glm::dvec2;
using glm::uvec2;
//using glm::min;
//using glm::ceil;
//using glm::abs;

mapview::mapview()
	: _zoom{0}
	, _origin_pos{0.0, 0.0}
	, _button_press_pos{0.0, 0.0}
{
	set_can_focus();
	add_events(Gdk::KEY_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
}

size_t mapview::zoom_level() const
{
	return _zoom;
}

void mapview::add_layer(mapview_layer * l)
{
	_layers.push_back(l);
}

bool mapview::on_draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	for (mapview_layer * lay : _layers)
		lay->draw(cr);

	return true;
}

void mapview::on_size_allocate(Gtk::Allocation & alloc)
{
	dvec2 window{alloc.get_width(), alloc.get_height()};
	double map_width = pow(2.0, _zoom)*256.0;
	_origin_pos = (window - map_width)/2.0;

	for (mapview_layer * lay : _layers)
	{
		lay->resize(window.x, window.y);
		lay->map_to_window_transform(_origin_pos);
	}

	Gtk::DrawingArea::on_size_allocate(alloc);  // default handler
}

bool mapview::on_motion_notify_event(GdkEventMotion * event)
{
	if (event->state & GDK_BUTTON3_MASK)
	{
		Gtk::Allocation alloc = get_allocation();
		dvec2 window{alloc.get_width(), alloc.get_height()};

		dvec2 world{pow(2.0, _zoom)*256.0};
		dvec2 cursor{event->x, event->y};
		dvec2 dp = cursor - _button_press_pos;

		dvec2 move_about{world.x > window.x ? dp.x : 0.0, world.y > window.y ? dp.y : 0.0};

		dvec2 lo = window - world;
		dvec2 hi{0.0, 0.0};
		_origin_pos = clamp(_origin_pos + move_about, lo, hi);

		// origin corrections
		if (window.x > world.x)
			_origin_pos.x = (window.x - world.x)/2.0;
		if (window.y > world.y)
			_origin_pos.y = (window.y - world.y)/2.0;

		_button_press_pos = cursor;

		for (mapview_layer * l : _layers)
			l->map_to_window_transform(_origin_pos);

		force_redraw();

		return true;
	}

	return false;  // allow propagation
}

bool mapview::on_button_press_event(GdkEventButton * event)
{
	if (event->button != 3)
		return false;  // allow propagation

	_button_press_pos = dvec2{event->x, event->y};

	return true;
}

bool mapview::on_key_press_event(GdkEventKey * event)
{
	Gtk::Allocation alloc = get_allocation();
	dvec2 window{alloc.get_width(), alloc.get_height()};

	bool zoom_changed = false;
	if (event->keyval == GDK_KEY_p)  // zoom in
	{
		++_zoom;
		_origin_pos = (_origin_pos - window/2.0) * 2.0 + window/2.0;
		zoom_changed = true;
	}
	else if (event->keyval == GDK_KEY_m)  // zoom out
	{
		if (_zoom > 0)
		{
			--_zoom;
			zoom_changed = true;
			_origin_pos = (_origin_pos - window/2.0) / 2.0 + window/2.0;
		}
	}

	if (zoom_changed)
	{
		// origin corrections
		double map_size = pow(2.0, _zoom)*256.0;
		if (window.x > map_size)
			_origin_pos.x = (window.x - map_size)/2.0;
		if (window.y > map_size)
			_origin_pos.y = (window.y - map_size)/2.0;

		for (mapview_layer * l : _layers)
		{
			l->map_to_window_transform(_origin_pos);
			l->zoom(_zoom);
		}

		// TODO: solve title-update
//		update_title();
		force_redraw();

		return true;
	}

	return false;  // allow propagation
}

void mapview::force_redraw()
{
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win)
	{
		Gtk::Allocation alloc = get_allocation();
		Gdk::Rectangle r{0, 0, alloc.get_width(), alloc.get_height()};
		win->invalidate_rect(r, false);
	}
}
