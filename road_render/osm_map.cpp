#include <agg_basics.h>
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_conv_stroke.h"
#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_contour.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_conv_marker.h"
#include "agg_arrowhead.h"
#include "agg_vcgen_markers_term.h"
#include "agg_scanline_p.h"
#include <agg_renderer_scanline.h>
#include <agg_pixfmt_rgb.h>
#include "osm_map.hpp"
#include "transform.hpp"

using std::vector;

static void create_road_styles(vector<road_style> & styles);
static road_style get_road_style(road_type t);


osm_map::osm_map(unsigned w, unsigned h)
	: _w{w}, _h{h}, _zoom{1.0}, _selected{road_type::count}
{
	create_road_styles(_styles);
	assert(_styles.size() == (size_t)road_type::count);
}

void osm_map::render(ren_base & dst)
{
	if (_roads.empty())
		return;

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::line_cap_e           cap = agg::butt_cap;
//	if(m_cap.cur_item() == 1) cap = agg::square_cap;
//	if(m_cap.cur_item() == 2) cap = agg::round_cap;

	agg::line_join_e           join = agg::miter_join;
//	if(m_join.cur_item() == 1) join = agg::miter_join_revert;
//	if(m_join.cur_item() == 2) join = agg::round_join;
//	if(m_join.cur_item() == 3) join = agg::bevel_join;


	to_window_transform T{_w, _h, _roads_bbox};

	footway_symbolizer footway{dst};

	for (road const & r : _roads)
	{
		// data
		agg::path_storage path;
		vec2 p0 = _zoom * (T * r.geometry[0]);
		path.move_to(p0.x, p0.y);
		for (size_t i = 1; i < r.geometry.size(); ++i)
		{
			vec2 p = _zoom * (T * r.geometry[i]);
			path.line_to(p.x, p.y);
		}

		// render
//		if (r.type == _selected)
//		{
//			road_style style{agg::rgba{1, 0, 0}, agg::rgba{0, 0, 0}, 10.0};
//			road_symbolizer symb{dst};
//			symb.width(style.width);
//			symb.color(style.color1, style.color2);
//			symb.render(path);
//		}
//		else
		{
			switch (r.type)
			{
				case road_type::footway:
				{
					footway.render(path);
					break;
				}

				default:
				{
					road_symbolizer symb{dst};
					road_style style = get_road_style(r.type);
					symb.width(style.width);
					symb.color(style.color1, style.color2);
					symb.render(path);
				}
			}
		}
	}  // for (road ...
}

void osm_map::from_file(std::string const & osm_file)
{
	read_roads(osm_file, _roads, _roads_bbox);
}

void osm_map::select(road_type r)
{
	_selected = r;
}

road_style & osm_map::get_road_style(road_type t)
{
	return _styles[(int)t];
}

road_style const & osm_map::get_road_style(road_type t) const
{
	return _styles[(int)t];
}

road_style get_road_style(road_type t)
{
	double const base_width = 10.0;

	switch (t)
	{
		case road_type::motorway:
			return road_style{agg::rgba{0.913725, 0.564706, 0.627451}, agg::rgba{0.803922, 0.325490, 0.180392}, base_width};

		case road_type::trunk:
			return road_style{agg::rgba{0.984314, 0.698039, 0.603922}, agg::rgba{0.670588, 0.482353, 0.011765}, base_width};

		case road_type::primary:
			return road_style{agg::rgba{0.992157, 0.843137, 0.631373}, agg::rgba{0.486275, 0.537255, 0.000000}, base_width};

		case road_type::secondary:
			return road_style{agg::rgba{0.968627, 0.980392, 0.749020}, agg::rgba{0.450980, 0.494118, 0.094118}, 0.4*base_width};

		case road_type::tertiary:
			return road_style{agg::rgba{1, 1, 1}, agg::rgba{0.788235, 0.772549, 0.772549}, base_width};

		case road_type::residential:
			return road_style{agg::rgba{1, 1, 1}, agg::rgba{0.788235, 0.772549, 0.772549}, 0.6*base_width};

		case road_type::service:
			return road_style{agg::rgba{1, 1, 1}, agg::rgba{0.788235, 0.772549, 0.772549}, 0.4*base_width};

		case road_type::living_street:
			return road_style{agg::rgba{0.929412, 0.929412, 0.929412}, agg::rgba{0.772549, 0.772549, 0.772549}, 0.4*base_width};

		case road_type::pedestrian:
			return road_style{agg::rgba{0.866667, 0.866667, 0.913725}, agg::rgba{0.654902, 0.647059, 0.650980}, 0.4*base_width};

//		case road_type::footway:
//			return road_style{agg::rgba{1, 0, 0}, agg::rgba{0, 0, 0}, 0.4*base_width};

		case road_type::raceway:
			return road_style{agg::rgba{1.000000, 0.752941, 0.792157}, agg::rgba{1.000000, 0.752941, 0.792157}, 0.4*base_width};

		default:
//			return road_style{agg::rgba{1, 0, 0}, agg::rgba{0, 0, 0}, 0.4*base_width};
			return road_style{agg::rgba{1, 1, 1}, agg::rgba{0.788235, 0.772549, 0.772549}, 0.6*base_width};
	}
}

void create_road_styles(vector<road_style> & styles)
{
	for (int i = (int)road_type::motorway; i < (int)road_type::count; ++i)
		styles.push_back(get_road_style((road_type)i));
}
