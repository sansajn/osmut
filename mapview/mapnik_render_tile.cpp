#include <utility>
#include <mapnik/rule.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>
#include "mapnik_render_tile.hpp"

using std::move;
using namespace mapnik;

struct agg_renderer_visitor_1
{
	agg_renderer_visitor_1(Map const & m, unsigned offset_x, unsigned offset_y)
		: _m{m}, _offset_x{offset_x}, _offset_y{offset_y}
	{}

	template <typename T>
	void operator()(T & pixmap)
	{
		throw std::runtime_error{"This image type is not currently supported for rendering."};
	}

private:
	Map const & _m;
	unsigned _offset_x, _offset_y;
};

template <>
void agg_renderer_visitor_1::operator()<image_rgba8>(image_rgba8 & pixmap)
{
	agg_renderer<image_rgba8> ren{_m, pixmap, 1.0, _offset_x, _offset_y};
	ren.apply();
}

Map make_map(size_t zoom)
{
	int size = int(pow(2.0, zoom)*256.0);

	Map m{size, size};
	m.set_background(parse_color("steelblue"));

	// styles
	feature_type_style world_style;
	{
		rule r;
		{
			polygon_symbolizer poly_sym;
			put(poly_sym, keys::fill, parse_color("darkgreen"));
			r.append(move(poly_sym));
		}
		world_style.add_rule(move(r));
	}
	m.insert_style("polygons", move(world_style));

	// builtup-area
	feature_type_style builtup_style;
	{
		rule r;
		{
			polygon_symbolizer poly_sym;
			put(poly_sym, keys::fill, parse_color("lightsteelblue"));
			r.append(move(poly_sym));
		}
		builtup_style.add_rule(move(r));
	}
	m.insert_style("builtup", move(builtup_style));

	// places
	feature_type_style places_style;
	{
		rule r;
		{
			point_symbolizer pt_sym;
			r.append(move(pt_sym));
		}
		places_style.add_rule(move(r));
	}
	m.insert_style("places", move(places_style));

	// layers
	{
		parameters p;
		p["type"] = "shape";
//		p["file"] = "data/world/TM_WORLD_BORDERS-0.3";
		p["file"] = "data/world/world_boundaries_m2";
		p["encoding"] = "utf8";

		layer lyr{"world-layer"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("polygons");

		m.add_layer(move(lyr));
	}

	{
		parameters p;
		p["type"] = "shape";
		p["file"] = "data/world/builtup_area2";
		p["encoding"] = "utf8";

		layer lyr{"builtup-area"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("builtup");

		m.add_layer(move(lyr));
	}

	{
		parameters p;
		p["type"] = "shape";
		p["file"] = "data/world/places2";
		p["encoding"] = "utf8";

		layer lyr{"places-layer"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("places");

		m.add_layer(move(lyr));
	}

	m.zoom_all();

	return m;
}

void render_tile_to_file(Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file)
{
	image_any im{width, height};
	util::apply_visitor(agg_renderer_visitor_1{map, offset_x, offset_y}, im);
	save_to_file(im, file.string(), "png");
}
