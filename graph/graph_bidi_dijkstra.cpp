/*! Vygeneruje jednoduchy graf súbor umožnujúci prehladávanie
obojsmerným dijkstrovím algoritmom.

todo:
* idx-table premenuj na nodes 

*/


#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <string>
#include <utility>
#include <climits>
#include <fstream>
#include <cmath>
#include <cassert>
#include <iostream>
#include <GeographicLib/Geodesic.hpp>
#include "geometry.h"
#include "osm_iter.h"
#include "adjacency_graph.h"

using std::sort;
using std::count_if;
using std::set;
using std::vector;
using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::numeric_limits;
using std::ofstream;
using std::cout;
using std::endl;

using namespace GeographicLib;

namespace chrono 
{
	using namespace std::chrono;
	typedef high_resolution_clock clock;
}


struct graph_header
{
	uint32_t vertices;
	uint32_t edges;
	rect<signed_coordinate> bounds;  // 16B
	uint32_t edge_table_idx;
	uint32_t vertex_table_idx;
};

struct vertex
{
	signed_coordinate coord;
};

struct edge
{
	int source;
	int target;
	int distance;  // length
	int type;
};

struct graph_edge  //!< graph edge representation
{
	uint32_t target;
	uint32_t
		direction : 2,
		type : 6,
		distance : 24;
};

struct way_stats
{
	int n_ways;
	int n_filtered_ways;
};

float const pi = 3.1415926535f;
float const r_earth = 6371000.0f;

graph_header create_header(vector<pair<vertex, int>> const & verts, uint32_t n_edges);

/*! Vytvorí graf.
\param[in] edges Zotriedený zoznam hrán.
Štruktúra grafu je nasledovná [Ukážka:

	+-------------+
	|    header   |
	+-------------+
	|    nodes    |
	+-------------+
	|    edges    |
	+-------------+
	|   vertices  |
	+-------------+

--- koniec ukážky]. Indexová tabuľka obsahuje pozície susedou w
vrchola v (ukazujúce do edges) v graf súbore. Vrcholy v sú v ňej
zoradene od 0 do N.

Detailný popis grafu [Ukážka:

	node[]
		lat:32
		lon:32

	edge[]
		target:32
		direction:2
		type:6
		distance:24

	vertex[]
		source-first-adjacet:32

Graf su v podstate iba edge[] a vertex[], node[] k výpočtu vobec nepotrebujem a
kludne môže byť uložený mimo graf.

\note neoptimalizujem obojsmerne hrany, lebo vo write-graph o nich neviem

--- koniec ukážky]. */
bool write_graph(char const * fname, vector<pair<vertex, int>> const & nodes,
	vector<edge> const & edges, adjacency_graph<edge const *> const & g);

void read_vertices(osmut::xml_reader & xml,vector<pair<vertex, int>> & verts);

void read_ways(osmut::xml_reader & xml, vector<edge> & edges, 
	way_stats & stats);

void cut_way(way const & w, vector<vertex> const & verts, 
	vector<edge> & edges);

void reindex_edges(vector<pair<vertex, int>> const & verts, 
	vector<edge> & edges);

void fill_used_vertices(vector<edge> const & edges, set<int> & used_verts);

void fill_used_mask(vector<pair<vertex, int>> const & verts, 
	set<int> const & used_verts, vector<bool> & used_mask);

void remove_unused_vertices(vector<bool> & used_mask, 
	vector<pair<vertex, int>> & verts);

void calculate_distance(vector<pair<vertex, int>> const & verts, vector<edge> & edges);

e_highway_values classify(way const & w);

bool in_profile(tagmap & tags, char const * profile_ways[]);

void sort_edges(vector<edge> & edges);

void create_adjacency_graph(vector<pair<vertex, int>> & verts,
	vector<edge> const & edges, adjacency_graph<edge const *> & g);

void test_write_graph();


char const * pedestrian_ways[] = 
{
	"living_street",
	"pedestrian",
	"path",
	"footway",
	"steps",
	NULL
};


int main(int argc, char * argv[])
{
	string xml_fname("test.osm");
	if (argc > 1)
		xml_fname = argv[1];

	osmut::xml_reader xml(xml_fname.c_str());

// vrcholy	
	chrono::clock::time_point start_tm = chrono::clock::now();
	
	vector<pair<vertex, int>> verts;
	read_vertices(xml, verts);

	assert(verts.size() && "not a valid vertex");

	chrono::clock::duration dt = chrono::clock::now() - start_tm;

	cout << "vertices:" << verts.size() << ", size:" 
		<< sizeof(vertex)*verts.size()/1024.0 << "kB in ~" 
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" 
		<< endl;

// hrany
	start_tm = chrono::clock::now();

	vector<edge> edges;
	way_stats stats{0, 0};
	read_ways(xml, edges, stats);

	dt = chrono::clock::now() - start_tm;

	cout << "ways    :" << stats.n_ways << ", filtered: " 
		<< stats.n_filtered_ways << " in ~"
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" 
		<< endl;

	cout << "edges   :" << edges.size() << endl;

	start_tm = chrono::clock::now();

	set<int> used_verts;
	fill_used_vertices(edges, used_verts);

	cout << "used vetrices: " << used_verts.size()
		<< " (" << (used_verts.size()/float(verts.size()))*100 << "%)" << endl;

	vector<bool> used_mask;
	fill_used_mask(verts, used_verts, used_mask);
	used_verts.clear();  // we not need them anymore

	remove_unused_vertices(used_mask, verts);
	used_mask.clear();  // not valid anymore

	dt = chrono::clock::now() - start_tm;

	cout << "remove unused vertices in ~" 
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" 
		<< endl;

	cout << "vertices after removed unused: " << verts.size() << endl;

	assert(edges.size() >= verts.size()-1 
		&& "logic error: too much vertices.");

	start_tm = chrono::clock::now();

	reindex_edges(verts, edges);

	sort_edges(edges);  // toto uz dalej nie je potrebne

	dt = chrono::clock::now() - start_tm;
	cout << "reindex and sort vertices in ~"
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" << endl;

	start_tm = chrono::clock::now();

	calculate_distance(verts, edges);

	dt = chrono::clock::now() - start_tm;
	cout << "calculate edges distance ~"
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" << endl;

	adjacency_graph<edge const *> g;
	create_adjacency_graph(verts, edges, g);

	start_tm = chrono::clock::now();

	write_graph("/data/temp/graph.bgrp", verts, edges, g);

	dt = chrono::clock::now() - start_tm;

	cout << "write graph in ~"
		<< chrono::duration_cast<chrono::milliseconds>(dt).count() << " ms" 
		<< endl;

	return 0;
}

void sort_edges(vector<edge> & edges)
{
	sort(begin(edges), end(edges), 
		[](edge const & a, edge const & b){return a.source < b.source;});
}

void create_adjacency_graph(vector<pair<vertex, int>> & verts,
	vector<edge> const & edges, adjacency_graph<edge const *> & g)
{
	g.resize(verts.size());
	for (auto & e : edges)
	{
		g.append_edge(e.source, &e);
		g.append_edge(e.target, &e);
	}
}

void fill_used_vertices(vector<edge> const & edges, set<int> & used_verts)
{
	for (auto & e : edges)
	{
		used_verts.insert(e.source);
		used_verts.insert(e.target);
	}
}

void reindex_edges(vector<pair<vertex, int>> const & verts, 
	vector<edge> & edges)
{
	map<int, int> idxtable;
	for (int i = 0; i < verts.size(); ++i)
	{
		assert(idxtable.find(verts[i].second) == idxtable.end() 
			&& "logic error: dva vrcholy s rovnakym indexom");

		idxtable[verts[i].second] = i;
	}

	for (auto & e : edges)
	{
		assert(idxtable.find(e.source) != idxtable.end() 
			&& "logic error: source vrchol nenajdeny");

		assert(idxtable.find(e.target) != idxtable.end()
			&& "logic error: target vrchol nenajdeny");

		e.source = idxtable[e.source];
		e.target = idxtable[e.target];
	}
}

void fill_used_mask(vector<pair<vertex, int>> const & verts, 
	set<int> const & used_verts, vector<bool> & used_mask)
{
	used_mask.resize(verts.size(), false);

	for (int i = 0; i < verts.size(); ++i)
	{
		if (used_verts.find(verts[i].second) != used_verts.end())
			used_mask[i] = true;
	}
}

//! \note pomale, presuva prvky pri kazdom mazani, co je neziaduce
void remove_unused_vertices(vector<bool> & used_mask, 
	vector<pair<vertex, int>> & verts)
{
	int end_idx = used_mask.size();
	for (int i = 0; i < end_idx; ++i)
	{
		if (!used_mask[i])
		{
			verts[i] = verts[end_idx-1];  // swap
			used_mask[i] = used_mask[end_idx-1];
			end_idx -= 1;
			i -= 1;  // check same index one more time
		}
	}

	verts.resize(end_idx);
}

void cut_way(way const & w, vector<edge> & edges)
{
	for (int i = 0; i < w.node_ids.size()-1; ++i)
	{
		int node_idx = w.node_ids[i];
		int next_node_idx = w.node_ids[i+1];

		e_highway_values highway_class = classify(w);
		if (highway_class == e_highway_values::e_unknown)
			continue;

		// distance will be calculated later
		edges.push_back(edge{node_idx, next_node_idx, 0, int(highway_class)});

		auto oneway_it = w.tags->find("oneway");
		if (oneway_it == w.tags->end() || oneway_it->second != "yes")
			edges.push_back(
				edge{next_node_idx, node_idx, 0, int(highway_class)});
	}
}

void calculate_distance(vector<pair<vertex, int>> const & verts, vector<edge> & edges)
{
	for (auto & e : edges)
	{
		double distance = 0.0;
		signed_coordinate const & s = verts[e.source].first.coord;
		signed_coordinate const & t = verts[e.target].first.coord;
		Geodesic const & geod = Geodesic::WGS84;
		geod.Inverse(s.lat/1e7, s.lon/1e7, t.lat/1e7, t.lon/1e7, distance);
		e.distance = distance;
	}
}

inline float torad(float deg) {return deg*pi/180.0f;}

e_highway_values classify(way const & w)
{
	static char const * highway_values[] = {
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
		"living_street",
		"pedestrian",
		"residential",
		"unclassified",
		"service",
		"track",
		"bus_guideway",
		"raceway",
		"road",
		"path",
		"footway",
		"cycleway",
		"bridleway",
		"steps",
		"proposed",
		"construction"
	};

	static map<string, e_highway_values> highway_map;

	if (highway_map.empty())
	{
		int n_values = sizeof(highway_values)/sizeof(highway_values[0]);
		for (int i = 0; i < n_values; ++i)
			highway_map.insert(
				make_pair(string(highway_values[i]), e_highway_values(i)));
	}

	auto highway_it = w.tags->find("highway");
	if (highway_it == end(*w.tags))
		return e_highway_values::e_unknown;
	else
		return highway_map[highway_it->second];

//	assert(highway_it != end(*w.tags) && "way has no 'highway' tag");
}

void read_ways(osmut::xml_reader & xml, vector<edge> & edges, way_stats & stats)
{
	for (way_iterator way_it(xml); way_it != way_iterator(); ++way_it)
	{
		stats.n_ways += 1;
		if (way_it->tags /*&& in_profile(*way_it->tags, pedestrian_ways)*/)
			cut_way(*way_it, edges);  // rozdeli cestu na hrany (zmen nazov)
		else
			stats.n_filtered_ways += 1;
	}
}

inline bool valid_node_position(signed_coordinate const & coord)
{
	return coord.lat <= 180*1e7 && coord.lat >= -180*1e7 && coord.lon <= 180*1e7
		&& coord.lon >= -180*1e7;
}

void read_vertices(osmut::xml_reader & xml,vector<pair<vertex, int>> & verts)
{
	for (node_iterator node_it(xml); node_it != node_iterator(); ++node_it)
	{
		vertex v;
		v.coord.lat = node_it->lat;
		v.coord.lon = node_it->lon;
		if (valid_node_position(v.coord))
			verts.push_back(make_pair(v, node_it->id));
		else
			cout << "invalid coordinate " << node_it->id << "\n";
	}
}

bool write_graph(char const * fname, vector<pair<vertex, int>> const & nodes,
	vector<edge> const & edges, adjacency_graph<edge const *> const & g)
{
	ofstream fgraph(fname);
	if (!fgraph.is_open())
		return false;

// header
	graph_header head = create_header(nodes, edges.size());
	fgraph.write((char const *)(&head), sizeof(graph_header));

// nodes
	for (auto & n : nodes)
	{
		fgraph.write((char const *)(&n.first.coord.lat), 4);
		fgraph.write((char const *)(&n.first.coord.lon), 4);
	}

// edges
	assert(fgraph.tellp() == head.edge_table_idx
		&& "edges are written to wrong file position");

	assert(nodes.size() == g.vertices()
		&& "number of nodes must be equall to number of vertices");

	vector<uint32_t> verts(g.vertices(), numeric_limits<uint32_t>::max());

	// pre kazdy vrchol
	for (size_t v = 0; v < g.vertices(); ++v)
	{
		verts[v] = fgraph.tellp();

		// for edge direction 0:forward, 1:backward, 2:bidirectional

		for (auto & e : g[v])
		{
			graph_edge ge;
			ge.type = e->type;
			ge.distance = e->distance;
			ge.direction = (e->source == v) ? 0 : 1;
			ge.target = (ge.direction == 0) ? e->target : e->source;

			fgraph.write((char const *)(&ge), sizeof(graph_edge));  // 8 bytes
		}
	}

/*
	// postupne pre kazdy vrchol
	for (int v = 0; v < nodes.size(); ++v)
	{
		vector<graph_edge> incidents;

		// najdi jeho incidencne hrany (in aj out)
		for (auto & e : edges)
		{
			assert(e.distance < (1<<24)
				&& "data-error: prekrocena maximalna dlzka hrany" );

			// for edge direction 0:forward, 1:backward, 2:bidirectional

			if (e.source == v)  // forward
				incidents.push_back(	graph_edge{e.target, 0, e.type, e.distance});

			if (e.target == v)  // backward
				incidents.push_back(graph_edge{e.source, 1, e.type, e.distance});
		}

		verts[v] = fgraph.tellp();

		// zapis ich do grafu
		for (auto & e : incidents)
			fgraph.write((char const *)(&e), sizeof(graph_edge));  // 8 bytes
	}
*/

	// stats, how many zero degree vertices are there ?
	int zero_degree_count = int(
		count_if(begin(verts), end(verts),
			[](uint32_t x){return x == numeric_limits<uint32_t>::max();}));
	cout << "zero degree vertices: " << zero_degree_count << "\n";

// vertices
	assert(fgraph.tellp() == head.vertex_table_idx
		&& "vertex table is written to wrong file position");

	for (auto & v : verts)
		fgraph.write((char const *)(&v), 4);

	fgraph.close();

	return true;
}

graph_header create_header(vector<pair<vertex, int>> const & verts, uint32_t n_edges)
{
	uint32_t n_verts = verts.size();

	assert(n_verts > 1 && "there must be at least two vertices");

	rect<signed_coordinate> bounds{verts[0].first.coord, verts[1].first.coord};
	for (auto & v : verts)
		bounds.expand(v.first.coord);

#ifdef DEBUG
	// rectangle check
	for (auto & v : verts)
		assert(bounds.contains(v.first.coord) 
			&& "logic error: vertex outside boundary");
#endif

	cout << "map bounds: " << bounds.p1.lat/1e7f << ":" << bounds.p1.lon/1e7f 
		<< ", " << bounds.p2.lat/1e7f << ":" << bounds.p2.lon/1e7f << "\n"
		<< "  (" << bounds.p1.lat << ":" << bounds.p1.lon << ", " 
		<< bounds.p2.lat << ":" << bounds.p2.lon << ")" << "\n";

	graph_header h;
	h.vertices = n_verts;
	h.edges = n_edges;
	h.bounds = bounds;
	h.edge_table_idx = sizeof(graph_header) + n_verts*sizeof(vertex);
	h.vertex_table_idx = h.edge_table_idx + n_edges*2*sizeof(graph_edge);
	return h;
}

bool in_profile(tagmap & tags, char const * profile_ways[])
{
	char const ** type = profile_ways;
	while (*type)
	{
		if (tags["highway"] == *type)
			return true;
		type += 1;
	}
	return false;
}

void test_write_graph()
{
/*	
	vector<vertex> verts{vertex{-36946600, 404150810}, 
		vertex{-36934300, 404123190}};
	vector<edge> edges{edge{0, 1, 10, 0}};
	if (!write_graph("graph.grp", verts, edges))
		cout << "graph creation failed!\n";
	else
		cout << "graph created\n";
*/		
}
