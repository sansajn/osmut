// mapview program implementation
#include <string>
#include <memory>
#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <gtkmm.h>
#include "mapview.hpp"
#include "osm_layer.hpp"
#include "locally_stored_tiles.hpp"
#include "mapnik_generated_tiles_mt.hpp"
#include "center_cross.hpp"
#include "geo_point_layer.hpp"
#include "text.hpp"

using std::unique_ptr;
using std::cout;
namespace fs = boost::filesystem;

fs::path const TILE_TMP = "output/tiles";


class mapview_window : public Gtk::Window
{
public:
	mapview_window();

private:
	void update_title();

	mapview _map;
};


inline std::ostream & operator<<(std::ostream & o, glm::vec2 const & v)
{
	o << "(" << v.x << ", " << v.y << ")";
	return o;
}

mapview_window::mapview_window()
//	: _map{unique_ptr<tile_source>{new mapnik_generated_tiles{TILE_TMP}}}
//	: _map{unique_ptr<tile_source>{new mapnik_generated_tiles_mt{TILE_TMP}}}
{
	update_title();
	add(_map);

	text_layout::init(&_map);

	_map.add_layer(new osm_layer{unique_ptr<tile_source>{new locally_stored_tiles{"data/tiles"}}});

	// TODO: pouzi libosmium kniznicu k citaniu OSM suboru s atm poziciamy,
	// vytvor point layer a zobraz vsetky atm
	_map.add_layer(new geo_point_layer);

	_map.add_layer(new center_cross);
	show_all_children();
}

void mapview_window::update_title()
{
	set_title(boost::str(boost::format{"Map View (zoom:%1%)"} % _map.zoom_level()));
}


int main(int argc, char * argv[])
{
	fs::remove_all(TILE_TMP);  // render new tiles every time

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "osmut.mapview");
	mapview_window w;
	w.set_default_size(800, 600);
	return app->run(w);
}
