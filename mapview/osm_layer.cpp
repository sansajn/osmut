#include <utility>
#include <vector>
#include <boost/format.hpp>
#include <gtkmm.h>
#include <gdkmm.h>
#include "osm_layer.hpp"
#include "text.hpp"

using std::move;
using std::array;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using glm::dvec2;
using glm::uvec2;

osm_layer::osm_layer(unique_ptr<tile_source> tiles)
	: _tiles{move(tiles)}
	, _window{0.0, 0.0}
	, _zoom{0}
	, _map_to_widow{0.0, 0.0}
{}

void osm_layer::draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	array<uvec2, 2> bounds = visible_tiles();

	// request tiles
	vector<shared_ptr<tile>> tiles_to_render;
	for (size_t y = bounds[1].x; y < bounds[1].y; ++y)
		for (size_t x = bounds[0].x; x < bounds[0].y; ++x)
			tiles_to_render.emplace_back(_tiles->get(_zoom, x, y));

	dvec2 origin = _map_to_widow;

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
			Gdk::Cairo::set_source_pixbuf(cr, tile, round(origin.x) + x*256, round(origin.y) + y*256);
			cr->paint();
		}
	}
	cr->restore();

	// draw stitches
	cr->save();
	for (size_t n = 1; n < size_t(pow(2.0, _zoom)); ++n)
	{
		cr->move_to(origin.x + n*256.0, 0.0);
		cr->line_to(origin.x + n*256.0, _window.y);

		cr->move_to(0.0, origin.y + n*256.0);
		cr->line_to(_window.x, origin.y + n*256.0);
	}
	cr->stroke();
	cr->restore();

	// draw tile coordinates
	cr->save();
	for (size_t j = bounds[1].x; j < bounds[1].y; ++j)
	{
		for (size_t i = bounds[0].x; i < bounds[0].y; ++i)
		{
			Glib::RefPtr<Pango::Layout> layout = text_layout::ref().create_layout(
				boost::str(boost::format{"(%1%,%2%)"} % i % j).c_str());
			layout->set_font_description(_font);
			int tw, th;
			layout->get_pixel_size(tw, th);
			cr->move_to(origin.x + i*256.0 + 256.0-tw-5.0, origin.y + j*256.0 + 256.0-th-5.0);
			layout->show_in_cairo_context(cr);
		}
	}
	cr->restore();
}

void osm_layer::resize(int w, int h)
{
	_window = dvec2{w, h};
}

void osm_layer::zoom(size_t z)
{
	_zoom = z;
}

void osm_layer::map_to_window_transform(glm::dvec2 const & T)
{
	_map_to_widow = T;
}

array<uvec2, 2> osm_layer::visible_tiles() const
{
	dvec2 world{pow(2.0, _zoom)};
	dvec2 origin = _map_to_widow;
	dvec2 p0{min(abs(origin)/256.0, world)};
	dvec2 size{min(ceil(_window/256.0)+1.0, world - p0)};

	array<uvec2, 2> result{uvec2{p0.x, p0.x + size.x}, uvec2{p0.y, p0.y + size.y}};  // zip, combine

	// corrections
	double map_size = pow(2.0, _zoom)*256.0;
	if (_window.x > map_size)
		result[0] = uvec2{0, size_t(pow(2.0, _zoom))};
	if (_window.y > map_size)
		result[1] = uvec2{0, size_t(pow(2.0, _zoom))};

	return result;
}
