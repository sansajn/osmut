#include <thread>
#include <utility>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <mapnik/datasource_cache.hpp>
#include "mapnik_render_tile.hpp"
#include "mapnik_generated_tiles_mt.hpp"

using std::move;
using std::async;
using std::shared_ptr;


mapnik_tile_mt::mapnik_tile_mt(fs::path const & fname)
	: _p{fname}
{}

mapnik_tile_mt::mapnik_tile_mt(std::future<fs::path> && promised_tile)
	: _promised_tile{move(promised_tile)}
{}

fs::path mapnik_tile_mt::path()
{
	if (_p.empty())
		_p = _promised_tile.get();

	return _p;
}


mapnik_generated_tiles_mt::mapnik_generated_tiles_mt(fs::path const & cache_dir, size_t max_render_threads)
	: _cache_dir{cache_dir}
	, _max_render_threads{max_render_threads > 0 ? max_render_threads : std::thread::hardware_concurrency()}
	, _used_zoom{(size_t)-1}
{
	mapnik::datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");
}

mapnik_generated_tiles_mt::~mapnik_generated_tiles_mt()
{
	free_maps();
}

shared_ptr<tile> mapnik_generated_tiles_mt::get(size_t zoom, size_t x, size_t y)
{
	if (_used_zoom != zoom)
	{
		free_maps();

		// create maps
		for (size_t i = 0; i < _max_render_threads; ++i)
			_maps.push(new mapnik::Map{make_map(zoom)});

		fs::path tile_dir = _cache_dir / boost::str(boost::format{"%1%/%2%"} % zoom % x);
		if (!fs::exists(tile_dir))
			fs::create_directories(tile_dir);
	}

	fs::path p = _cache_dir / boost::str(boost::format{"%3%/%1%/%2%.png"} % x % y % zoom);
	if (!fs::exists(p.parent_path()))
		fs::create_directories(p.parent_path());

	if (fs::exists(p))
		return shared_ptr<tile>{new mapnik_tile_mt{p}};
	else
		return create_new_tile(zoom, x, y, p);
}

shared_ptr<tile> mapnik_generated_tiles_mt::create_new_tile(size_t zoom, size_t x, size_t y, fs::path const & fname)
{
	return shared_ptr<tile>{new mapnik_tile_mt{async(
		[this, zoom, x, y, fname]() {
			mapnik::Map * m;
			_maps.wait_and_pop(m);
			assert(m);
			render_tile_to_file(*m, x*256, y*256, 256, 256, fname);
			_maps.push(m);
			return fname;
		}
	)}};
}

void mapnik_generated_tiles_mt::free_maps()
{
	mapnik::Map * m;
	while (_maps.try_pop(m))
		delete m;
}
