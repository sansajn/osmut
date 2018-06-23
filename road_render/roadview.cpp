// zobrazi cesty s mapy a umozni ich stylovanie
#include <vector>
#include <iostream>
#include <cassert>
#include <gtkmm.h>
#include "symbolizer.hpp"
#include "road.hpp"
#include "transform.hpp"
#include "osm_map.hpp"
#include "map_area.hpp"

unsigned const WIDTH = 800, HEIGHT = 600;

using std::vector;
using std::cout;
using std::cerr;


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


class roadview_window : public Gtk::Window
{
public:
	roadview_window();

private:
	Gtk::TreeModel::Row add_road_type(Glib::ustring const & s, unsigned rid);
	void on_combo_changed();
	void on_width_changed();
	void on_inner_color_changed();
	void on_outer_color_changed();
	road_type active_road_type() const;

	road_model_columns _road_combo_columns;
	Glib::RefPtr<Gtk::ListStore> _road_model;

	// ui
	Gtk::Fixed _fbox;
	Gtk::ComboBox _road_combo;
	Gtk::VBox _edit_box;
	Gtk::HBox _width_box;
	Gtk::HScale _width_scale;
	Glib::RefPtr<Gtk::Adjustment> _width_adj;
	Gtk::HBox _color_box;
	Gtk::HBox _color1_box, _color2_box;
	Gtk::ColorButton _c1_btn, _c2_btn;

	// map
	osm_map _osm;
	map_area _map;
};

roadview_window::roadview_window()
	: _width_adj{Gtk::Adjustment::create(10, 1, 20)}
	, _c1_btn{Gdk::Color{"ff0000"}}
	, _c2_btn{Gdk::Color{"00ff00"}}
	, _osm{WIDTH, HEIGHT}
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

	// toolbox
	_fbox.put(_road_combo, 10, 10);

	_width_box.pack_start(*Gtk::manage(new Gtk::Label{"Width:"}), Gtk::PACK_SHRINK);
	_width_scale.set_adjustment(_width_adj);
	_width_scale.signal_value_changed().connect(sigc::mem_fun(*this, &roadview_window::on_width_changed));

	_width_box.pack_start(_width_scale);
	_edit_box.add(_width_box);

	_c1_btn.signal_color_set().connect(sigc::mem_fun(*this, &roadview_window::on_inner_color_changed));

	_color1_box.pack_start(*Gtk::manage(new Gtk::Label{"Inner color:"}), Gtk::PACK_SHRINK);
	_color1_box.pack_start(_c1_btn);
	_color_box.add(_color1_box);

	_c2_btn.signal_color_set().connect(sigc::mem_fun(*this, &roadview_window::on_outer_color_changed));

	_color2_box.pack_start(*Gtk::manage(new Gtk::Label{"Outer color:"}), Gtk::PACK_SHRINK);
	_color2_box.pack_start(_c2_btn);
	_color_box.add(_color2_box);

	_color_box.set_spacing(20);
	_edit_box.add(_color_box);

	_fbox.put(_edit_box, 150, 10);

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
	if (!it)
		return;

	Gtk::TreeModel::Row row = *it;
	if (!row)
		return;

	int rid = row[_road_combo_columns.rid];
	assert(rid < (int)road_type::count);
	_osm.select((road_type)rid);
	Glib::ustring name = row[_road_combo_columns.name];
	cout << "  RID=" << rid << ", name=" << name << std::endl;

	// update tool-bar
	road_style style = _osm.get_road_style((road_type)rid);

	_width_adj->set_value(style.width);

	Gdk::Color c;
	c.set_rgb_p(style.color1.r, style.color1.g, style.color1.b);
	_c1_btn.set_color(c);

	c.set_rgb_p(style.color2.r, style.color2.g, style.color2.b);
	_c2_btn.set_color(c);

	queue_draw();  // redraw
}

void roadview_window::on_width_changed()
{
	road_type rtype = active_road_type();
	_osm.get_road_style(rtype).width = _width_adj->get_value();
	queue_draw();  // redraw
}

void roadview_window::on_inner_color_changed()
{
	road_type rtype = active_road_type();

	Gdk::RGBA c = _c1_btn.get_rgba();
	_osm.get_road_style(rtype).color1 = agg::rgba{c.get_red(), c.get_green(), c.get_blue()};

	queue_draw();  // redraw
}

void roadview_window::on_outer_color_changed()
{
	road_type rtype = active_road_type();

	Gdk::RGBA c = _c2_btn.get_rgba();
	_osm.get_road_style(rtype).color2 = agg::rgba{c.get_red(), c.get_green(), c.get_blue()};

	queue_draw();  // redraw
}

road_type roadview_window::active_road_type() const
{
	Gtk::TreeModel::iterator it = _road_combo.get_active();
	if (!it)
		throw std::logic_error{"unable to get road_type"};

	Gtk::TreeModel::Row row = *it;
	if (!row)
		throw std::logic_error{"unable to get road_type"};

	int rid = row[_road_combo_columns.rid];
	assert(rid < (int)road_type::count);

	return (road_type)rid;
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
