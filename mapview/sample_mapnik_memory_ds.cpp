// ukazka pouzitia memory datasource-u
#include <string>
#include <memory>
#include <utility>
#include <exception>
#include <iostream>
#include <glm/glm.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/rule.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/wkt/wkt_factory.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::move;
using std::logic_error;
using std::cout;
using glm::dvec2;
using namespace mapnik;

unsigned const WIDTH = 1920;
unsigned const HEIGHT = WIDTH;
string const wkt_albert_pt = "POINT(14.45019899999999957 50.10563479999999714)";
dvec2 const albert_loc = {50.10563479999999714, 14.45019899999999957};

int main(int argc, char * argv[])
{
	// vytvor a napln datasource
	parameters params;
	shared_ptr<memory_datasource> ds = make_shared<memory_datasource>(params);

	context_ptr ctx = make_shared<context_type>();
	feature_ptr f = feature_factory::create(ctx, 1);
	geometry::geometry<double> geom;
	if (from_wkt(wkt_albert_pt, geom))
		f->set_geometry(move(geom));
	else
		throw logic_error{"unable to parse wkt geometry"};

	ds->push(f);

	// vytvor a inizializuj mapu s vrstvou pre bod
	datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");

	Map m{WIDTH, HEIGHT};
	m.set_background(parse_color("white"));
	m.set_srs(MAPNIK_GMERC_PROJ);  // web/google mercator projection aka. EPSG 3857

	// styles
	feature_type_style poly_style;
	{
		rule r;
		{
			polygon_symbolizer poly_sym;
			put(poly_sym, keys::fill, parse_color("darkgreen"));
			r.append(move(poly_sym));
		}
		poly_style.add_rule(move(r));
	}
	m.insert_style("polygons", move(poly_style));

	feature_type_style poi_style;
	{
		rule r;
		{
			point_symbolizer poi_sym;
			r.append(move(poi_sym));
		}
		poi_style.add_rule(move(r));
	}
	m.insert_style("pois", move(poi_style));


	{  // polygon layer
		parameters p;
		p["type"] = "sqlite";
		p["file"] = "data/holesovice.db";
		p["table"] = "multipolygons";
		p["encoding"] = "utf8";

		layer lyr{"layer_0"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("polygons");
//		lyr.set_srs(srs_lcc);

		m.add_layer(lyr);
	}

	{  // poi layer
		layer lyr{"poi_layer"};
		lyr.set_datasource(ds);
		lyr.add_style("pois");
		m.add_layer(lyr);
	}

	m.zoom_all();

	// render
	image_rgba8 im{WIDTH, HEIGHT};
	agg_renderer<image_rgba8> ren{m, im};
	ren.apply();
	save_to_file(im, "output/holecovice_with_poi.png");

	dvec2 albert_loc_gmerc = albert_loc;
	lonlat2merc(&albert_loc_gmerc.y, &albert_loc_gmerc.x, 1);
	cout << "GPS(" << albert_loc.x << ", " << albert_loc.y << ") -> "
		<< "GMERC(" << albert_loc_gmerc.x << ", " << albert_loc_gmerc.y << ")\n";

	dvec2 albert_loc_rel = dvec2{albert_loc_gmerc.x/MAXEXTENT, albert_loc_gmerc.y/MAXEXTENT};
	cout << "REL(" << albert_loc_rel.x << ", " << albert_loc_rel.y << ")\n";

	// toto je zle, musim brat do uvahu len vykreslovanu oblast
	dvec2 albert_loc_img = (albert_loc_rel + 0.5) * (double)WIDTH;
	cout << "IMG(" << albert_loc_img.x << ", " << albert_loc_img.y << ")\n";

	return 0;
}
