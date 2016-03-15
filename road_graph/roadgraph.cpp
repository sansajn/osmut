// vygeneruje proland graph s osm mapy (pozicie ciest urcuje odcitanim uhlou (co je mimo mierky))
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <boost/geometry.hpp>
#include <glm/vec2.hpp>
#include "reader_impl.h"
#include "graph/graph.hpp"
#include "geometry/box2.hpp"
#include "osm_iter.h"

using std::string;
using std::vector;
using std::map;
using std::cout;
using geom::box2;
using glm::vec2;
using glm::ivec2;
using boost::geometry::expand;
using boost::geometry::centroid;
using boost::geometry::dsv;
using boost::geometry::make_inverse;

char const * default_graph_file = "out.graph";

struct road
{
	vector<vec2> geometry;  // signed-coordinate (lat,lon)*1e7 as float
	bool oneway;
};

class osm_roads
{
public:
	osm_roads() {}
	void read(string const & osm_file);
	vector<road> & roads() {return _roads;}
	vector<road> const & roads() const {return _roads;}
	box2 const & bounds() const {return _bounds;}
	void recalculate_bounds();

private:
	void read_nodes(osmut::xml_reader & osm, map<int, ivec2> & nodes);
	void read_ways(osmut::xml_reader & osm, map<int, ivec2> const & nodes, vector<road> & roads);

	vector<road> _roads;
	box2 _bounds;
};

void osm_roads::read(string const & osm_file)
{
	osmut::xml_reader osm{osm_file.c_str()};

// nodes
	map<int, ivec2> nodes;
	read_nodes(osm, nodes);

// ways
	read_ways(osm, nodes, _roads);

	recalculate_bounds();
}

box2 make_bounds(road const & r)
{
	box2 result = make_inverse<box2>();
	for (auto const & p : r.geometry)
		expand(result, p);
	return result;
}

void osm_roads::recalculate_bounds()
{
	_bounds = make_bounds(_roads[0]);
	for (unsigned i = 1; i < _roads.size(); ++i)
		expand(_bounds, make_bounds(_roads[i]));
}

void osm_roads::read_nodes(osmut::xml_reader & osm, map<int, ivec2> & nodes)
{
	for (node_iterator it{osm}; it != node_iterator{}; ++it)  // TODO: use range
		nodes[it->id] = ivec2{it->lat, it->lon};
}


bool any_of(string const & tag, char const * profile[])
{
	char const ** type = profile;
	while (*type)
	{
		if (tag == *type)
			return true;
		++type;
	}
	return false;
}


bool is_car(way const & w)
{
	static char const * car_ways[] = {
		"motorway",
		"motorway_link",
		"trunk",
		"trunk_link",
		"primary",
		"primary_link",
		"secondary",
		"secondary_link",
		"tertiary",
		"tertiary_link",
		"residential",
		"unclassified",
//		"service",
		"track",
		"bus_guideway",
		"raceway",
		"road",
		nullptr
	};

	auto highway_tag_it = w.tags->find("highway");
	if (highway_tag_it == w.tags->end())
		return false;

	if (any_of(highway_tag_it->second, car_ways))
		return true;

	// special cases (service)
	if (highway_tag_it->second == "service")
		return w.tags->find("service") == w.tags->end();  // prechod je napr. oznaceny ako highway:service, service:parking_aisle

	return false;
}


void osm_roads::read_ways(osmut::xml_reader & osm, map<int, ivec2> const & nodes, vector<road> & roads)
{
	for (way_iterator it{osm}; it != way_iterator{}; ++it)  // TODO: use range
	{
		if (!it->tags || !is_car(*it))
			continue;

		road r;
		way const & w = *it;
		for (int nid : w.node_ids)
		{
			auto node_it = nodes.find(nid);
			assert(node_it != nodes.end() && "undefined node");
			vec2 p{node_it->second};
			r.geometry.push_back(vec2{p.y, p.x});  // TODO: reserve
		}

		auto oneway_it = w.tags->find("oneway");
		r.oneway = (oneway_it != w.tags->end()) && (oneway_it->second == "yes");

		roads.push_back(r);  // TODO: road copy
	}
}

void dump_to_graph(vector<road> const & roads, box2 const & bounds, string const & graph_file)
{
	// TODO: ries duplicity pri vytvarani grafu

	grp::graph g;

	vec2 bounds_center;
	centroid(bounds, bounds_center);

	for (road const & r : roads)
	{
		// prvy a posledny su nodes, inak krivky
		grp::node * first = new grp::node{vec2{r.geometry.front()} - bounds_center};
		grp::node * last = new grp::node{vec2{r.geometry.back()} - bounds_center};

		g.add_node(first);
		g.add_node(last);

		grp::curve * c = new grp::curve;
		c->type = 0;
		c->width = 0;
		c->add_vertex(first, last);
		for (unsigned i = 1; i < r.geometry.size()-1; ++i)
			c->add_vertex(r.geometry[i] - bounds_center, -1, false);

		g.add_curve(c);
	}

	grp::writer w;
	w.write(g, graph_file);
}

void print_usage_and_exit()
{
	exit(1);
}


int main(int argc, char * argv[])
{
	if (argc < 2)
		print_usage_and_exit();

	string osm_file = argv[1];

	string graph_file = default_graph_file;
	if (argc > 2)
		graph_file = argv[2];

	osm_roads osm;
	osm.read(osm_file);

	dump_to_graph(osm.roads(), osm.bounds(), graph_file);

	cout << "file '" << graph_file << "' created\n"
		<< "done!" << std::endl;

	return 0;
}
