// zobrazi cesty s mapy a umozni ich stylovanie
#include <vector>
#include <iostream>
#include <cassert>
#include <gtkmm.h>
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
#include "symbolizer.hpp"
#include "road.hpp"
#include "transform.hpp"

unsigned const WIDTH = 800, HEIGHT = 600;

using std::vector;
using std::cout;
using std::cerr;

using ren_base = agg::renderer_base<pixfmt>;

bool write_ppm(const unsigned char* buf, unsigned width, unsigned height,
	const char* file_name);

struct road_model_columns : public Gtk::TreeModel::ColumnRecord
{
	Gtk::TreeModelColumn<unsigned> rid;  //!< road id
	Gtk::TreeModelColumn<Glib::ustring> name;

	road_model_columns()
	{
		add(rid);
		add(name);
	}
};

class osm_map
{
public:
	osm_map(unsigned w, unsigned h)
		: _w{w}, _h{h}, _zoom{1.0}, _selected{road_type::count}
	{}

	void render(ren_base & dst);
	void from_file(std::string const & osm_file);
	void zoom(double v) {_zoom = v;}
	void select(road_type r);

private:
	road_style get_road_style(road_type t) const;

	unsigned _w, _h;
	box _roads_bbox;
	vector<road> _roads;
	double _zoom;
	road_type _selected;
};

class map_area : public Gtk::DrawingArea
{
public:
	map_area(osm_map & osm, unsigned w, unsigned h)
		: _osm{osm}
	{
		set_size_request(w, h);
	}

private:
	bool on_draw(Cairo::RefPtr<Cairo::Context> const & cr) override;

	osm_map & _osm;
	Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
};

bool map_area::on_draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	int const w = get_allocation().get_width();
	int const h = get_allocation().get_height();

	// lazy pixbuf initialization
	if (!_pixbuf || w != _pixbuf->get_width() || h != _pixbuf->get_height())
	{
		_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, w, h);
		assert(_pixbuf->get_rowstride() == w*3);
	}

	agg::rendering_buffer rbuf{_pixbuf->get_pixels(), w, h, w*3};
	pixfmt pixf{rbuf};
	ren_base renb{pixf};
	renb.clear(agg::rgba8{255, 250, 230});
	_osm.render(renb);

	assert(_pixbuf);
	Gdk::Cairo::set_source_pixbuf(cr, _pixbuf, (w - _pixbuf->get_width())/2, (h - _pixbuf->get_height())/2);
	cr->paint();

	return true;
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
		if (r.type == _selected)
		{
			road_style style{agg::rgba{1, 0, 0}, agg::rgba{0, 0, 0}, 10.0};
			road_symbolizer symb{dst};
			symb.width(style.width);
			symb.color(style.color1, style.color2);
			symb.render(path);
		}
		else
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


class roadview_window : public Gtk::Window
{
public:
	roadview_window();

private:
	void redraw();
	Gtk::TreeModel::Row add_road_type(Glib::ustring const & s, unsigned rid);
	void on_combo_changed();

	road_model_columns _road_combo_columns;
	Glib::RefPtr<Gtk::ListStore> _road_model;

	// ui
	Gtk::ComboBox _road_combo;
	Gtk::Fixed _fbox;

	// map
	osm_map _osm;
	map_area _map;
};

roadview_window::roadview_window()
	: _osm{WIDTH, HEIGHT}
	, _map{_osm, WIDTH, HEIGHT}
{
	set_title("roadview - road style editor");
	set_default_size(WIDTH, HEIGHT);
	add(_fbox);

	// map
	_osm.from_file("../assets/maps/holesovice.osm");

	_fbox.put(_map, 0, 0);

	// road types
	_road_model = Gtk::ListStore::create(_road_combo_columns);
	_road_combo.signal_changed().connect(sigc::mem_fun(*this, &roadview_window::on_combo_changed));
	_road_combo.set_model(_road_model);
	_road_combo.pack_start(_road_combo_columns.name);

	Gtk::TreeModel::Row row = add_road_type("motorway", 0);
	_road_combo.set_active(row);
	add_road_type("trunk", 1);
	add_road_type("primary", 2);
	add_road_type("secondary", 3);
	add_road_type("tertiary", 4);
	add_road_type("residential", 5);
	add_road_type("service", 6);
	add_road_type("living street", 7);
	add_road_type("pedestrian", 8);
	add_road_type("footway", 9);
	add_road_type("raceway", 10);

	_fbox.put(_road_combo, 10, 10);

//	add(_fbox);  // it creates warning
	show_all_children();
}

Gtk::TreeModel::Row roadview_window::add_road_type(Glib::ustring const & name, unsigned rid)
{
	Gtk::TreeModel::Row row = *(_road_model->append());
	row[_road_combo_columns.rid] = rid;
	row[_road_combo_columns.name] = name;
	return row;
}

void roadview_window::on_combo_changed()
{
	Gtk::TreeModel::iterator it = _road_combo.get_active();
	if (it)
	{
		Gtk::TreeModel::Row row = *it;
		if (row)
		{
			int rid = row[_road_combo_columns.rid];
			assert(rid < (int)road_type::count);
			_osm.select((road_type)rid);
			Glib::ustring name = row[_road_combo_columns.name];
			cout << "  RID=" << rid << ", name=" << name << std::endl;
			queue_draw();  // redraw
		}
	}
	else
		cerr << "invalid road type selected" << std::endl;
}


road_style osm_map::get_road_style(road_type t) const
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

bool write_ppm(const unsigned char* buf,
					unsigned width,
					unsigned height,
					const char* file_name)
{
	FILE* fd = fopen(file_name, "wb");
	if(fd)
	{
		fprintf(fd, "P6 %d %d 255 ", width, height);
		fwrite(buf, 1, width * height * 3, fd);
		fclose(fd);
		return true;
	}
	return false;
}


int main(int argc, char * argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "roadview");
	roadview_window w;
	return app->run(w);
}
