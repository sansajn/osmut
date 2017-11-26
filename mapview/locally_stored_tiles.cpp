#include <boost/format.hpp>
#include "locally_stored_tiles.hpp"

using std::string;

locally_stored_tiles::locally_stored_tiles(fs::path const & tile_dir)
	: _tile_dir{tile_dir}
{}

string locally_stored_tiles::get(size_t zoom, size_t x, size_t y) const
{
	return (_tile_dir / boost::str(boost::format{"%3%/%1%/%2%.png"} % x % y % zoom)).string();
}
