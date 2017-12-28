#include <cmath>
#include <boost/format.hpp>
#include "local_tile.hpp"
#include "locally_stored_tiles.hpp"

using std::shared_ptr;
using glm::dvec2;

locally_stored_tiles::locally_stored_tiles(fs::path const & tile_dir)
	: _tile_dir{tile_dir}
{}

shared_ptr<tile> locally_stored_tiles::get(size_t zoom, size_t x, size_t y)
{
	return shared_ptr<tile>{new local_tile{
		(_tile_dir / boost::str(boost::format{"%3%/%1%/%2%.png"} % x % y % zoom))}};
}
