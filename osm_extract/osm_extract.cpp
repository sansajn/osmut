/* $ ./osm_extract ../data/petrzalka.osm 48.1166 17.0998 48.1231 17.1096 petrzalka-extract.osm */
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "geometry.h"
#include "osm_range.h"
using std::set;
using std::vector;
using std::string;
using std::ofstream;
using std::cout;
using namespace irr::io;

#define VERSION 20121025

void write_node(ofstream & fout, node const & n);
void write_way(ofstream & fout, way const & w, vector<long> const & nodes);
void write_osm_header(ofstream & fout);
void write_osm_footer(ofstream & fout);
void unrecoverable_error(boost::format const & msg);
void check_arguments(int argc, char * argv[]);

inline node_range make_node_r(IrrXMLReader & xml, node & buf) 
{
	return node_range(node_iterator(&xml, &buf), node_iterator());
}

inline way_range make_way_r(IrrXMLReader & xml, way & buf)
{
	return way_range(way_iterator(&xml, &buf), way_iterator());
}

struct stats
{
	int nodes;
	int ways;
	int written_nodes;
	int written_ways;
};

int main(int argc, char * argv[])
{
	check_arguments(argc, argv);

	if (argc < 6)
		unrecoverable_error(
			boost::format("not enought parameters, 6 needed get %1%") % argc);

	string osm_fname = argv[1];

	signed_coordinate sw{int(boost::lexical_cast<double>(argv[2])*1e7),
		int(boost::lexical_cast<double>(argv[3])*1e7)};

	signed_coordinate ne{int(boost::lexical_cast<double>(argv[4])*1e7),
		int(boost::lexical_cast<double>(argv[5])*1e7)};

	rect<signed_coordinate> bounding_box(sw, ne);

	string osm_extract("extract.osm");
	if (argc > 6)
		osm_extract = argv[6];

	ofstream fextract(osm_extract.c_str());
	if (!fextract.is_open())
		unrecoverable_error(
			boost::format("can't create output file '%1%'") % osm_extract);

	write_osm_header(fextract);

	IrrXMLReader * xml = createIrrXMLReader(osm_fname.c_str());

	stats st = {0};

	set<long> inbbox_nodes;

	// nodes
	cout << "reading nodes ";

	node nodebuf;
	for (auto r = make_node_r(*xml, nodebuf); r; ++r)
	{
		if (bounding_box.contains(
			signed_coordinate{int32_t(r->lat*1e7),	int32_t(r->lon*1e7)}))
		{
			inbbox_nodes.insert(r->id);
			write_node(fextract, *r);
			st.written_nodes += 1;
		}

		st.nodes += 1;
		if (!(st.nodes % 10000000))
		{
			cout << "*";
			cout.flush();
		}
	}

	cout << " done\n";
	cout << boost::format("witten nodes: %1%/%2%\n") % st.written_nodes 
		% st.nodes;

	// ways
	cout << "reading ways ";

	way waybuf;
	for (auto r = make_way_r(*xml, waybuf); r; ++r)
	{
		vector<long> inbbox_waynodes;
		for (auto id : r->node_ids)
			if (inbbox_nodes.find(id) != end(inbbox_nodes))
				inbbox_waynodes.push_back(id);

		if (inbbox_waynodes.size())
		{
			write_way(fextract, *r, inbbox_waynodes);
			st.written_ways += 1;
		}

		st.ways += 1;
		if (!(st.ways % 10000000))
		{
			cout << "*";
			cout.flush();
		}
	}

	cout << " done\n";
	cout << boost::format("witten ways: %1%/%2%\n") % st.written_ways 
		% st.ways;

	write_osm_footer(fextract);

	fextract.close();


	return 0;
}


void write_node(ofstream & fout, node const & n)
{
	fout << "<node id=\"" << n.id << "\" lat=\"" << n.lat << "\" lon=\""
		<< n.lon << "\"";

	if (n.tags)
	{
		fout << ">\n";
		for (auto tag : *n.tags)
		{
			fout << "\t<tag k=\"" << tag.first << "\" v=\"" << tag.second 
				<< "\"/>\n";
		}
		fout << "</node>\n";
	}
	else
		fout << "/>\n";
}

void write_way(ofstream & fout, way const & w, vector<long> const & nodes)
{
	fout << "<way id=\"" << w.id << "\">\n";

	for (auto n : nodes)
		fout << "\t<nd ref=\"" << n << "\"/>\n";

	if (w.tags)
	{
		for (auto tag : *w.tags)
			fout << "\t<tag k=\"" << tag.first << "\" v=\"" << tag.second << "\"/>\n";
	}

	fout << "</way>\n";
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}

void write_osm_header(ofstream & fout)
{
	fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<osm version=\"0.6\" generator=\"osm_extract\">\n";
}

void write_osm_footer(ofstream & fout)
{
	fout << "</osm>\n";
}

void check_arguments(int argc, char * argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (string(argv[i]) == string("-v"))
		{
			cout << VERSION << "\n";
			exit(0);
		}
	}
}

