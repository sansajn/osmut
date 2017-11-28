#include <utility>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/rule.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>
#include "mapnik_generated_tiles.hpp"

using std::string;
using std::move;
using namespace mapnik;

static void render_tile_to_file(Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file);

static Map make_map(size_t zoom);


struct agg_renderer_visitor_1
{
	agg_renderer_visitor_1(Map const & m, double scale_factor, unsigned offset_x, unsigned offset_y)
		: _m{m}, _scale_factor{scale_factor}, _offset_x{offset_x}, _offset_y{offset_y}
	{}

	template <typename T>
	void operator()(T & pixmap)
	{
		throw std::runtime_error{"This image type is not currently supported for rendering."};
	}

private:
	Map const & _m;
	double _scale_factor;
	unsigned _offset_x, _offset_y;
};

template <>
void agg_renderer_visitor_1::operator()<image_rgba8>(image_rgba8 & pixmap)
{
	agg_renderer<image_rgba8> ren{_m, pixmap, _scale_factor, _offset_x, _offset_y};
	ren.apply();
}

void render_tile_to_file(Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file)
{
	image_any im{width, height};
	util::apply_visitor(agg_renderer_visitor_1{map, 1.0, offset_x, offset_y}, im);
	save_to_file(im, file.string(), "png");
}


mapnik_generated_tiles::mapnik_generated_tiles(fs::path const & cache_dir)
	: _cache_dir{cache_dir}
	, _used_zoom{(size_t)-1}
{
	datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");
}

Map make_map(size_t zoom)
{
	int size = int(pow(2.0, zoom)*256.0);

	Map m{size, size};
	m.set_background(parse_color("steelblue"));

	// styles
	feature_type_style poly_style;
	{
		rule r;
		{
			polygon_symbolizer poly_sym;
			put(poly_sym, keys::fill, parse_color("darkgreen"));
			r.append(move(poly_sym));
		}
		poly_style.add_rule(move(r));
	}
	m.insert_style("polygons", move(poly_style));

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
		p["file"] = "data/world/TM_WORLD_BORDERS-0.3";
		p["encoding"] = "utf8";

		layer lyr{"world layer"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("polygons");

		m.add_layer(lyr);
	}

	{
		parameters p;
		p["type"] = "shape";
		p["file"] = "data/world/places";
		p["encoding"] = "utf8";

		layer lyr{"places layer"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("places");

		m.add_layer(lyr);
	}

	m.zoom_all();

	return m;
}

std::string mapnik_generated_tiles::get(size_t zoom, size_t x, size_t y) const
{
	if (_used_zoom != zoom)
	{
		_map = make_map(zoom);
		_used_zoom = zoom;
	}

	fs::path p = _cache_dir / boost::str(boost::format{"%3%/%1%/%2%.png"} % x % y % zoom);
	if (fs::exists(p))
		return p.string();

	if (!fs::exists(p.parent_path()))
		fs::create_directories(p.parent_path());

	render_tile_to_file(_map, x*256, y*256, 256, 256, p);

	return p.string();
}
