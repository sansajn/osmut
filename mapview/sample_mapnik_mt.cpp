// pouzitie mapniku vo viecerich vlaknach
#include <string>
#include <future>
#include <utility>
#include <vector>
#include <chrono>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <mapnik/map.hpp>
#include <mapnik/rule.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>
#include "sync_queue.hpp"

using std::string;
using std::move;
using std::async;
using std::future;
using std::vector;
using std::cout;
using namespace mapnik;
namespace fs = boost::filesystem;


class tile_source
{
public:
	tile_source(size_t max_render_threads = 0);
	~tile_source();
	future<string> get(size_t zoom, size_t x, size_t y) const;

private:
	size_t _max_render_threads;
	mutable size_t _used_zoom;
	mutable sync_queue<Map *> _maps;
};

Map make_map(size_t zoom);

void render_tile_to_file(Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file);


tile_source::tile_source(size_t max_render_threads)
	: _used_zoom{(size_t)-1}
{
	_max_render_threads = (max_render_threads != 0)
		? max_render_threads : std::thread::hardware_concurrency();

	datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");
}

tile_source::~tile_source()
{
	// free maps
	Map * m;
	while (_maps.try_pop(m))
		delete m;
}

future<string> tile_source::get(size_t zoom, size_t x, size_t y) const
{
	if (_used_zoom != zoom)
	{
		// free maps
		Map * m;
		while (_maps.try_pop(m))
			delete m;

		// create maps
		for (size_t i = 0; i < _max_render_threads; ++i)
			_maps.push(new Map{make_map(zoom)});

		_used_zoom = zoom;
	}

	return async(
		[this, zoom, x, y]() {
			Map * m;
			_maps.wait_and_pop(m);
			assert(m);

			fs::path p = boost::str(boost::format{"output/%3%/%1%/%2%.png"} % x % y % zoom);
			if (!fs::exists(p.parent_path()))
				fs::create_directories(p.parent_path());

			render_tile_to_file(*m, x*256, y*256, 256, 256, p);

			_maps.push(m);  // return map, so it can be used by other worker

			return p.string();
		}
	);
}


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

void render_tile_to_file(Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file)
{
	image_any im{width, height};
	util::apply_visitor(agg_renderer_visitor_1{map, offset_x, offset_y}, im);
	save_to_file(im, file.string(), "png");
}


int main(int argc, char * argv[])
{
	cout << std::thread::hardware_concurrency() << " concurrent thread available\n";

	// getni dlazdice pre 2 level (4*4)
	size_t const zoom = 2;

	tile_source tiles;

	using hres_clock = std::chrono::high_resolution_clock;
	hres_clock::time_point t = hres_clock::now();

	size_t size = size_t(pow(2.0, zoom));
	vector<future<string>> result;
	for (size_t x = 0; x < size; ++x)
		for (size_t y = 0; y < size; ++y)
			result.push_back(tiles.get(zoom, x, y));

	for (auto & r : result)  // wait for result
		r.get();

	size_t d = std::chrono::duration_cast<std::chrono::milliseconds>(hres_clock::now() - t).count();
	cout << "tiles generated in "	<< d << "ms (" << d/double(size*size) << "ms per tile), done." << std::endl;

	return 0;
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

	// layers
	{
		parameters p;
		p["type"] = "shape";
		p["file"] = "data/world/world_boundaries_m2";
		p["encoding"] = "utf8";

		layer lyr{"world-layer"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("polygons");

		m.add_layer(move(lyr));
	}

	m.zoom_all();

	return m;
}
