// mapview program implementation
#include <string>
#include <memory>
#include <iostream>
#include <boost/format.hpp>
#include <gtkmm.h>
#include "mapview.hpp"

using std::unique_ptr;
using std::cout;


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
//	: _map{unique_ptr<tile_source>{new locally_storred_tiles{"data/tiles"}}}
	: _map{unique_ptr<tile_source>{new mapnik_generated_tiles{"output/tiles"}}}
{
	update_title();
	add(_map);
	show_all_children();
}

void mapview_window::update_title()
{
	set_title(boost::str(boost::format{"Map View (zoom:%1%)"} % _map.zoom_level()));
}


int main(int argc, char * argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "osmut.mapview");
	mapview_window w;
	w.set_default_size(800, 600);
	return app->run(w);
}
