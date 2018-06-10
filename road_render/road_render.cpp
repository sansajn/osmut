#include <algorithm>
#include <string>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include "agg_basics.h"
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
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgb.h"
#include "platform/agg_platform_support.h"
#include "ctrl/agg_slider_ctrl.h"
#include "ctrl/agg_rbox_ctrl.h"
#include <Magick++.h>
#include "road.hpp"
#include "pattern_renderer.hpp"

using std::min;
using std::max;
using std::string;
using std::vector;
using std::cout;
using boost::geometry::get;
namespace bg = boost::geometry;

using pixfmt = agg::pixfmt_rgb24;

enum flip_y_e { flip_y = true };



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

class road_symbolizer
{
public:
	using ren_base = agg::renderer_base<pixfmt>;

	road_symbolizer(ren_base & renb);
	void width(double w);
	void color(agg::rgba const & inner, agg::rgba const & outer);
	void render(agg::path_storage & path);

private:
	double _w;
	agg::rgba _inner, _outer;
	ren_base & _renb;  // rendering_buffer
};

class pattern_line_symbolizer;


class footway_symbolizer
{
public:
	using ren_base = agg::renderer_base<pixfmt>;

	footway_symbolizer(ren_base & renb);
	void render(agg::path_storage & path);

private:
	ren_base & _dst;  // rendering_buffer
	pattern_renderer<pixfmt> _ren;
};

footway_symbolizer::footway_symbolizer(ren_base & renb)
	: _dst{renb}
{
	// load pattern
	string const file_name = "3.ppm";

	Magick::Blob pixels;
	Magick::Image im{file_name};
	im.write(&pixels, "RGB");
	assert(pixels.length() == im.columns()*im.rows()*3);

	agg::rendering_buffer patt_buf;
	patt_buf.attach((uint8_t *)pixels.data(), im.columns(), im.rows(), im.columns()*3);
	pixfmt patt{patt_buf};

	_ren.pattern(patt);
}

void footway_symbolizer::render(agg::path_storage & path)
{
	_ren.render(_dst, path);
}


class the_application : public agg::platform_support
{
public:
	the_application(agg::pix_format_e format, bool flip_y, string const & osmfile);
	void on_init() override {}
	void on_draw() override;
	void on_mouse_button_down(int x, int y, unsigned flags) override;
	void on_mouse_move(int x, int y, unsigned flags) override;
	void on_key(int x, int y, unsigned key, unsigned flags) override;

private:
	road_style get_road_style(road_type t) const;

	agg::rbox_ctrl<agg::rgba8> m_join;
	agg::rbox_ctrl<agg::rgba8> m_cap;
	agg::slider_ctrl<agg::rgba8> m_width;
	agg::slider_ctrl<agg::rgba8> m_miter_limit;
	box _roads_bbox;
	vector<road> _roads;
	vec2 _origin;
	vec2 _pan_begin;
	int _zoom;  //!< from 1 to N
};


void the_application::on_draw()
{
	typedef agg::renderer_base<pixfmt> ren_base;

	pixfmt pixf(rbuf_window());
	ren_base renb(pixf);
	renb.clear(agg::rgba(1, 1, 1));

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::line_cap_e           cap = agg::butt_cap;
	if(m_cap.cur_item() == 1) cap = agg::square_cap;
	if(m_cap.cur_item() == 2) cap = agg::round_cap;

	agg::line_join_e           join = agg::miter_join;
	if(m_join.cur_item() == 1) join = agg::miter_join_revert;
	if(m_join.cur_item() == 2) join = agg::round_join;
	if(m_join.cur_item() == 3) join = agg::bevel_join;


	to_window_transform T{width(), height(), _roads_bbox};
	double zoom = (4.0 + _zoom) / 5.0;

	footway_symbolizer footway{renb};

	for (road const & r : _roads)
	{
		// data
		agg::path_storage path;
		vec2 p0 = _origin + zoom * (T * r.geometry[0]);
		path.move_to(p0.x, p0.y);
		for (size_t i = 1; i < r.geometry.size(); ++i)
		{
			vec2 p = _origin + zoom * (T * r.geometry[i]);
			path.line_to(p.x, p.y);
		}

		// render

		switch (r.type)
		{
			case road_type::footway:
			{
				footway.render(path);
				break;
			}

			default:
			{
				road_symbolizer symb{renb};
				road_style style = get_road_style(r.type);
				symb.width(style.width);
				symb.color(style.color1, style.color2);
				symb.render(path);
			}
		}
	}

	agg::render_ctrl(ras, sl, renb, m_join);
	agg::render_ctrl(ras, sl, renb, m_cap);
	agg::render_ctrl(ras, sl, renb, m_width);
	agg::render_ctrl(ras, sl, renb, m_miter_limit);
}

the_application::the_application(agg::pix_format_e format, bool flip_y, string const & osmfile)
	: agg::platform_support(format, flip_y)
	, m_join(10.0, 10.0, 133.0, 80.0, !flip_y)
	, m_cap(10.0, 80.0 + 10.0, 133.0, 80.0 + 80.0, !flip_y)
	, m_width(130 + 10.0, 10.0 + 4.0, 500.0 - 10.0, 10.0 + 8.0 + 4.0, !flip_y)
	, m_miter_limit(130 + 10.0, 20.0 + 10.0 + 4.0, 500.0 - 10.0, 20.0 + 10.0 + 8.0 + 4.0, !flip_y)
	, _origin{0, 0}
	, _zoom{1}
{
	add_ctrl(m_join);
	m_join.text_size(7.5);
	m_join.text_thickness(1.0);
	m_join.add_item("Miter Join");
	m_join.add_item("Miter Join Revert");
	m_join.add_item("Round Join");
	m_join.add_item("Bevel Join");
	m_join.cur_item(2);

	add_ctrl(m_cap);
	m_cap.add_item("Butt Cap");
	m_cap.add_item("Square Cap");
	m_cap.add_item("Round Cap");
	m_cap.cur_item(2);

	add_ctrl(m_width);
	m_width.range(3.0, 40.0);
	m_width.value(10.0);
	m_width.label("Width=%1.2f");

	add_ctrl(m_miter_limit);
	m_miter_limit.range(1.0, 10.0);
	m_miter_limit.value(4.0);
	m_miter_limit.label("Miter Limit=%1.2f");

	m_join.no_transform();
	m_cap.no_transform();
	m_width.no_transform();
	m_miter_limit.no_transform();

	read_roads(osmfile, _roads, _roads_bbox);

	// stats
	cout << "roads:" << _roads.size() << "\n";
}

road_style the_application::get_road_style(road_type t) const
{
	double base_width = m_width.value();

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

void the_application::on_mouse_button_down(int x, int y, unsigned flags)
{
	if (flags & agg::mouse_right)
		_pan_begin = vec2{x, y};
}

void the_application::on_mouse_move(int x, int y, unsigned flags)
{
	if (flags & agg::mouse_right)
	{
		vec2 p{x, y};
		vec2 diff = p - _pan_begin;
		_origin += diff;
		_pan_begin = p;
		force_redraw();
	}
}

void the_application::on_key(int x, int y, unsigned key, unsigned flags)
{
	unsigned prev_zoom = _zoom;

	switch (key)
	{
		case agg::key_kp_plus: _zoom += 1; break;
		case agg::key_kp_minus: _zoom = max(1, _zoom - 1); break;
	}

	if (_zoom != prev_zoom)
		force_redraw();
}

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

void road_symbolizer::render(agg::path_storage & path)
{
	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	// outer line
	agg::conv_stroke<agg::path_storage> outer{path};
	outer.width(_w);
	ras.add_path(outer);
	agg::render_scanlines_aa_solid(ras, sl, _renb, _outer);

	// inner line
	agg::conv_stroke<agg::path_storage> inner{path};
	inner.width(_w - 2.0);
	ras.add_path(inner);
	agg::render_scanlines_aa_solid(ras, sl, _renb, _inner);
}

road_symbolizer::road_symbolizer(ren_base & renb)
	: _renb{renb}
{}

void road_symbolizer::width(double w)
{
	_w = w;
}

void road_symbolizer::color(agg::rgba const & inner, agg::rgba const & outer)
{
	_inner = inner;
	_outer = outer;
}


int agg_main(int argc, char* argv[])
{
	the_application app(agg::pix_format_rgb24, flip_y, argc > 1 ? argv[1] : "../assets/maps/roads.osm");
	app.caption("OSM road render sample");

	if(app.init(1024, 768, 0))
		return app.run();

	return 1;
}
