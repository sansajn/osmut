#include "geo_point_layer.hpp"
#include <iostream>
#include <cmath>

using std::pair;
using std::cout;
using glm::dvec2;

dvec2 const PRAGUE_POS{50.088182, 14.420210};  // lat,lon

static dvec2 geo_to_map(dvec2 const & p, size_t zoom);  //!< tuto funkciu musy poskytovat map layer

geo_point_layer::geo_point_layer()
	: _map_to_window{0.0, 0.0}
	, _zoom{0}
{
	_points.push_back(PRAGUE_POS);
}

dvec2 geo_to_map(dvec2 const & p, size_t zoom)  //!< tuto funkciu musy poskytovat map layer
{
	double lat_rad = p.x * M_PI/180.0;
	double n = pow(2.0, zoom);
	size_t x = size_t((p.y + 180.0) / 360.0 * n * 256.0);
	size_t y = size_t((1.0 - log(tan(lat_rad) + (1.0 / cos(lat_rad))) / M_PI) / 2.0 * n * 256.0);
	return dvec2{x, y};
}

void geo_point_layer::draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	cr->save();

	// draw points
	cr->begin_new_sub_path();

	for (dvec2 const & p : _points)
	{
		dvec2 xy = geo_to_map(p, _zoom) + _map_to_window;
		cr->arc(xy.x, xy.y, 3.0, 0, 2*M_PI);
	}

	cr->stroke();

	cr->restore();
}

void geo_point_layer::map_to_window_transform(glm::dvec2 const & T)
{
	_map_to_window = T;
}

void geo_point_layer::zoom(size_t z)
{
	_zoom = z;
}

void geo_point_layer::add_point(glm::dvec2 const & p)
{
	_points.push_back(p);
}
