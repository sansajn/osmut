// mapview program implementation
#include <string>
#include <array>
#include <vector>
#include <utility>
#include <iostream>
#include <cmath>
#include <boost/format.hpp>
#include <gtkmm.h>
#include <gdkmm.h>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <sigc++/sigc++.h>
#include <gdk/gdkkeysyms.h>
#include <glm/glm.hpp>

using std::string;
using std::array;
using std::vector;
using std::pair;
using std::min;
using std::max;
using std::cout;
using glm::dvec2;
using glm::uvec2;
using glm::min;
using glm::ceil;
using glm::abs;


class mapview_window : public Gtk::Window
{
public:
	mapview_window();

private:
	bool on_key_press_event(GdkEventKey * event) override;
	void on_size_allocate(Gtk::Allocation & alloc) override;
	bool canvas_draw(Cairo::RefPtr<Cairo::Context> const & cr);
	bool canvas_motion_notify_event(GdkEventMotion * event);
	bool canvas_button_press_event(GdkEventButton * event);
	array<uvec2, 2> visible_tiles() const;
	void force_redraw();
	void update_title();

	Gtk::DrawingArea _canvas;
	size_t _zoom;  //!< map zoom level
	dvec2 _button_press_pos;
	dvec2 _origin_pos;
	Pango::FontDescription _font;
};

inline std::ostream & operator<<(std::ostream & o, glm::vec2 const & v)
{
	o << "(" << v.x << ", " << v.y << ")";
	return o;
}

mapview_window::mapview_window()
	: _zoom{0}
	, _button_press_pos{0.0, 0.0}
	, _origin_pos{0.0, 0.0}
{
	update_title();
	_canvas.add_events(Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
	_canvas.signal_draw().connect(sigc::mem_fun(*this, &mapview_window::canvas_draw));
	_canvas.signal_motion_notify_event().connect(sigc::mem_fun(*this, &mapview_window::canvas_motion_notify_event));
	_canvas.signal_button_press_event().connect(sigc::mem_fun(*this, &mapview_window::canvas_button_press_event));
	add(_canvas);
	show_all_children();
}

array<uvec2, 2> mapview_window::visible_tiles() const
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

bool mapview_window::on_key_press_event(GdkEventKey * event)
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

		update_title();
		force_redraw();

		return true;
	}

	return false;  // allow propagation
}

void mapview_window::on_size_allocate(Gtk::Allocation & alloc)
{
	dvec2 window{alloc.get_width(), alloc.get_height()};
	double map_width = pow(2.0, _zoom)*256.0;
	_origin_pos = (window - map_width)/2.0;

	Gtk::Window::on_size_allocate(alloc);  // default handler
}

bool mapview_window::canvas_draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	// draw tiles
	array<uvec2, 2> bounds = visible_tiles();

	cr->save();
	for (size_t y = bounds[1].x; y < bounds[1].y; ++y)
	{
		for (size_t x = bounds[0].x; x < bounds[0].y; ++x)
		{
			string tile_file = boost::str(
				boost::format{"tiles/%3%/%1%/%2%.png"} % x % y % _zoom);
			Glib::RefPtr<Gdk::Pixbuf> tile = Gdk::Pixbuf::create_from_file(tile_file);
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

bool mapview_window::canvas_motion_notify_event(GdkEventMotion * event)
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

bool mapview_window::canvas_button_press_event(GdkEventButton * event)
{
	if (event->button != 3)
		return false;  // allow propagation

	_button_press_pos = dvec2{event->x, event->y};

	return true;
}

void mapview_window::force_redraw()
{
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win)
	{
		Gtk::Allocation alloc = get_allocation();
		Gdk::Rectangle r{0, 0, alloc.get_width(), alloc.get_height()};
		win->invalidate_rect(r, false);
	}
}

void mapview_window::update_title()
{
	set_title(boost::str(boost::format{"Map View (zoom:%1%)"} % _zoom));
}

int main(int argc, char * argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "osmut.mapview");
	mapview_window w;
	w.set_default_size(800, 600);
	return app->run(w);
}
