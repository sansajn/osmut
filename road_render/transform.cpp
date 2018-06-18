#include <algorithm>
#include "transform.hpp"

using std::min;
using boost::geometry::get;
namespace bg = boost::geometry;

to_window_transform::to_window_transform(int w, int h, box const & data_bounds)
	: _w(w), _h(h), _origin{data_bounds.min_corner()}
{
	_wd = get<bg::max_corner, 0>(data_bounds) - get<bg::min_corner, 0>(data_bounds);
	_hd = get<bg::max_corner, 1>(data_bounds) - get<bg::min_corner, 1>(data_bounds);
	_k = min(_w, _h);
}

vec2 to_window_transform::operator*(vec2 const & p) const
{
	return vec2{
		(_w - _k)/2.0 + (_k * (get<0>(p) - get<0>(_origin)) / _wd),
		_k * (get<1>(p) - get<1>(_origin)) / _hd};
}
