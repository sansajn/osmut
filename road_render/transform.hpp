#pragma once
#include "road.hpp"  // vec2

class to_window_transform
{
public:
	to_window_transform(int w, int h, box const & data_bounds);
	vec2 operator*(vec2 const & p) const;

private:
	double _w, _h;
	vec2 _origin;
	double _wd, _hd;  //!< data width/height
	double _k;
};
