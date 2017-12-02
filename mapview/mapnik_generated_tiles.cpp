#include <utility>
#include <chrono>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>
#include <mapnik/datasource_cache.hpp>
#include "local_tile.hpp"
#include "mapnik_render_tile.hpp"
#include "mapnik_generated_tiles.hpp"

using std::string;
using std::move;
using std::shared_ptr;

using hres_clock = std::chrono::high_resolution_clock;


mapnik_generated_tiles::mapnik_generated_tiles(fs::path const & cache_dir)
	: _cache_dir{cache_dir}
	, _used_zoom{(size_t)-1}
{
	mapnik::datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");
}

shared_ptr<tile> mapnik_generated_tiles::get(size_t zoom, size_t x, size_t y)
{
	if (_used_zoom != zoom)
	{
		_map = make_map(zoom);
		_used_zoom = zoom;
	}

	fs::path p = _cache_dir / boost::str(boost::format{"%3%/%1%/%2%.png"} % x % y % zoom);
	if (fs::exists(p))
		return shared_ptr<tile>{new local_tile{p}};

	if (!fs::exists(p.parent_path()))
		fs::create_directories(p.parent_path());

	hres_clock::time_point t = hres_clock::now();

	render_tile_to_file(_map, x*256, y*256, 256, 256, p);

	BOOST_LOG_TRIVIAL(info) << "tile(" << zoom << ", " << x << ", " << y << ") rendered in "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(hres_clock::now() - t).count() << "ms";

	return shared_ptr<tile>{new local_tile{p}};
}
