#include <utility>
#include <vector>
#include <cmath>
#include <boost/format.hpp>
#include <gdk/gdkkeysyms.h>
#include "tile.hpp"
#include "mapview.hpp"

using std::string;
using std::array;
using std::pair;
using std::move;
using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using glm::dvec2;
using glm::uvec2;
using glm::min;
using glm::ceil;
using glm::abs;

mapview::mapview(unique_ptr<tile_source> tiles)
	: _tiles{move(tiles)}
	, _zoom{0}
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

array<uvec2, 2> mapview::visible_tiles() const
{
	Gtk::Allocation alloc = get_allocation();
	dvec2 window{alloc.get_width(), alloc.get_height()};

	dvec2 world{pow(2.0, _zoom)};
	dvec2 p0{min(abs(_origin_pos)/256.0, world)};
	dvec2 size{min(ceil(window/256.0)+1.0, world - p0)};

	array<uvec2, 2> result{uvec2{p0.x, p0.x + size.x}, uvec2{p0.y, p0.y + size.y}};  // zip, combine

	// corrections
	double map_size = pow(2.0, _zoom)*256.0;
	if (window.x > map_size)
		result[0] = uvec2{0, size_t(pow(2.0, _zoom))};
	if (window.y > map_size)
		result[1] = uvec2{0, size_t(pow(2.0, _zoom))};

	return result;
}

bool mapview::on_draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	// draw tiles
	array<uvec2, 2> bounds = visible_tiles();

	// request tiles
	vector<shared_ptr<tile>> tiles_to_render;
	for (size_t y = bounds[1].x; y < bounds[1].y; ++y)
		for (size_t x = bounds[0].x; x < bounds[0].y; ++x)
			tiles_to_render.emplace_back(_tiles->get(_zoom, x, y));

	// draw tiles
	cr->save();
	for (size_t y = bounds[1].x; y < bounds[1].y; ++y)
	{
		for (size_t x = bounds[0].x; x < bounds[0].y; ++x)
		{
			size_t idx = (y - bounds[1].x) * (bounds[0].y - bounds[0].x) + (x - bounds[0].x);
			tile & t = *tiles_to_render[idx];
			fs::path tile_file = t.path();
			Glib::RefPtr<Gdk::Pixbuf> tile = Gdk::Pixbuf::create_from_file(tile_file.string());
			Gdk::Cairo::set_source_pixbuf(cr, tile, round(_origin_pos.x) + x*256, round(_origin_pos.y) + y*256);
			cr->paint();
		}
	}
	cr->restore();

	// draw stitches
	Gtk::Allocation alloc = get_allocation();
	uvec2 window{alloc.get_width(), alloc.get_height()};

	cr->save();
	for (size_t n = 1; n < size_t(pow(2.0, _zoom)); ++n)
	{
		cr->move_to(_origin_pos.x + n*256.0, 0.0);
		cr->line_to(_origin_pos.x + n*256.0, window.y);

		cr->move_to(0.0, _origin_pos.y + n*256.0);
		cr->line_to(window.x, _origin_pos.y + n*256.0);
	}
	cr->stroke();
	cr->restore();

	// draw tile coordinates
	cr->save();
	for (size_t j = bounds[1].x; j < bounds[1].y; ++j)
	{
		for (size_t i = bounds[0].x; i < bounds[0].y; ++i)
		{
			Glib::RefPtr<Pango::Layout> layout = create_pango_layout(boost::str(
				boost::format{"(%1%,%2%)"} % i % j).c_str());
			layout->set_font_description(_font);
			int tw, th;
			layout->get_pixel_size(tw, th);
			cr->move_to(_origin_pos.x + i*256.0 + 256.0-tw-5.0, _origin_pos.y + j*256.0 + 256.0-th-5.0);
			layout->show_in_cairo_context(cr);
		}
	}
	cr->restore();

	// visualize center
	cr->save();
	cr->begin_new_sub_path();
	cr->arc(window.x/2.0, window.y/2.0, 5.0, 0, 2*M_PI);
	cr->stroke();
	cr->restore();

	return true;
}

void mapview::on_size_allocate(Gtk::Allocation & alloc)
{
	dvec2 window{alloc.get_width(), alloc.get_height()};
	double map_width = pow(2.0, _zoom)*256.0;
	_origin_pos = (window - map_width)/2.0;

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
