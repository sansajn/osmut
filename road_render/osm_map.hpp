#pragma once
#include <vector>
#include <string>
#include <agg_renderer_base.h>
#include "road.hpp"
#include "symbolizer.hpp"

using ren_base = agg::renderer_base<pixfmt>;

class osm_map
{
public:
	osm_map(unsigned w, unsigned h);
	void render(ren_base & dst);
	void from_file(std::string const & osm_file);
	void zoom(double v) {_zoom = v;}
	void select(road_type r);
	void unselect();
	road_style & get_road_style(road_type t);
	road_style const & get_road_style(road_type t) const;

private:
	unsigned _w, _h;
	box _roads_bbox;
	std::vector<road> _roads;
	double _zoom;
	road_type _selected;
	std::vector<road_style> _styles;
};
