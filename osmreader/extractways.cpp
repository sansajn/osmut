/* vylistuje všetky way-properties */
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include "osm_range.h"

using std::map;
using std::set;
using std::cout;
using std::string;
using std::ofstream;
using osmut::xml_reader;


void unrecoverable_error(boost::format const & msg);


struct gps_coordinate
{
	float lat;
	float lon;

	gps_coordinate() {}

	gps_coordinate(float latitude, float longitude)
		: lat(latitude), lon(longitude)
	{}
};

typename map<int, gps_coordinate> nodes_type;

class kml_dumper
{
public:
	kml_dumper(string const & fname, nodes_type const & nodes); 
	void dump(way const & w);
	bool ok() const {return _out.is_open();}

private:
	void line_string(vector<gps_coordinate> const & coords);
	void header();
	void footer();

	ofstream _out;
	nodes_type const & _nodes;
};


int main(int argc, char * argv[])
{
	if (argc < 4)
		unrecoverable_error(
			boost::format("not enought parameters, 4 needed"));

	string osm_fname = argv[1];
	xml_reader osm(osm_fname.c_str());

	cout << "open file '" << osm_fname << "'\n";

	cout << "reading nodes ..." << std::flush;

	nodes_type nodes;

	// save nodes for further use
	for (auto r = make_node_range(osm); r; ++r)
		nodes[r->id] = gps_coordinate(r->lat, r->lon);

	cout << "\nnodes:" << nodes.size() << "\n";

	cout << "reading ways ..." << std::flush;

	string key = argv[2];
	string value = argv[3];

	kml_dumper out("extract.kml", nodes);

	int nways = 0;
	for (auto r = make_way_range(osm); r; ++r)
	{
		way w = *r;
		if (w.tags)
			for (auto & t : *w.tags)
				if (t.first == key && t.second == value)
					out.dump(w);
		nways += 1;
	}

	cout << "\n\n{stats}\n";
	cout << "ways:" << nways << "\n";

	cout << "\n";

	return 0;
}


kml_dumper::kml_dumper(string const & fname, nodes_type const & nodes)
	: _out(ofstream(fname.c_str())), _nodes(nodes)
{
	if (ok())
		header();
}

kml_dumper::~kml_dumper()
{
	if (ok())
		footer();

	_out.flush();
	_out.close();
}

void kml_dumper::dump(way const & w, string const & name, string const & color)
{
	_out << "<Placemark>\n"
		<< "<name>" << name << "</name>\n"
		<< "<description></description>\n"
		<< "<Style id=\"way\">\n"
		<< "<LineStyle>\n"
		<< "\t<color>7f0000ff</color>\n"
		<< "\t<width>6</width>\n"
		<< "</LineStyle>\n"
		<< "</Style>\n";

	vector<gps_coordinate> coords;
	for (auto & id : w.node_ids)
		coords.push_back(_nodes[id]);

	line_string(coords);

	_out << "</Placemark>\n";
}

void kml_dumper::line_string(vector<gps_coordinate> const & coords)
{
	_out << "<LineString>\n"
		<< "\t<extrude>1</extrude>\n"
		<< "\t<tessellate>1</tessellate>\n"
		<< "\t<coordinates>\n";

	for (auto & c : coords)
		_out << "\t\t" << get<0>(c) << "," << get<1>(c) << ",0\n";

	_out << "\t</coordinates>\n"
		<< "</LineString>\n";
}

void kml_dumper::header()
{
	_out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
		<< "<Document>\n";
}

void kml_dumper::footer()
{
	_out << "<\Document>\n";
		<< "<\kml>\n";
}


void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}

