/* s mapy do kml extrahuje specifikovanu way
	$ extractway <osm-input> <way-id> [<kml-output>] */
#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "osm_range.h"

using std::map;
using std::set;
using std::vector;
using std::unique_ptr;
using std::cout;
using std::string;
using std::ofstream;
using osmut::xml_reader;


void unrecoverable_error(boost::format const & msg);


struct gps_coordinate
{
	int lat;
	int lon;

	gps_coordinate() {}

	gps_coordinate(int latitude, int longitude)
		: lat(latitude), lon(longitude)
	{}
};

typedef map<int, gps_coordinate> nodes_type;

class kml_dumper
{
public:
	kml_dumper(string const & fname, nodes_type const & nodes); 
	~kml_dumper();
	void dump(way const & w, string const & name, string const & color);
	bool ok() const {return _out->is_open();}

private:
	void line_string(vector<gps_coordinate> const & coords);
	void write_header();
	void write_footer();
	bool create_dumpfile();

	nodes_type const & _nodes;
	string _outfname;
	unique_ptr<ofstream> _out;
};


int main(int argc, char * argv[])
{
	if (argc < 3)
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

	cout << "reading ways ..." << std::flush;

	string out_fname("extract.kml");
	if (argc > 3)
		out_fname = argv[3];

	kml_dumper out(out_fname.c_str(), nodes);

	int id = boost::lexical_cast<int>(argv[2]);

	int nways = 0;
	for (auto r = make_way_range(osm); r; ++r)
	{
		way w = *r;
		if (w.id == id)
			out.dump(w, (boost::format("way:{1}") % id).str(), "7f0000ff");
		nways += 1;
	}

	cout << "\n\n";
	cout << "nodes:" << nodes.size() << ", ways:" << nways << "\n";

	cout << "\n";

	return 0;
}


kml_dumper::kml_dumper(string const & fname, nodes_type const & nodes)
	: _nodes(nodes), _outfname(fname)
{}

bool kml_dumper::create_dumpfile()
{
	_out.reset(new ofstream(_outfname.c_str()));
	if (_out->is_open())
		write_header();
	return _out->is_open();
}

kml_dumper::~kml_dumper()
{
	if (ok())
		write_footer();

	_out->flush();
	_out->close();
}

void kml_dumper::dump(way const & w, string const & name, 
	string const & color)
{
	if (!_out)
		create_dumpfile();

	*_out << "<Placemark>\n"
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
	{
		nodes_type::const_iterator it = _nodes.find(id);
		if (it != _nodes.end())
			coords.push_back(it->second);
	}

	line_string(coords);

	*_out << "</Placemark>\n";
}

void kml_dumper::line_string(vector<gps_coordinate> const & coords)
{
	*_out << "<LineString>\n"
		<< "\t<extrude>1</extrude>\n"
		<< "\t<tessellate>1</tessellate>\n"
		<< "\t<coordinates>\n";

	for (auto & c : coords)
		*_out << "\t\t" << c.lat/1e7 << "," << c.lon/1e7 << ",0\n";

	*_out << "\t</coordinates>\n"
		<< "</LineString>\n";
}

void kml_dumper::write_header()
{
	*_out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
		<< "<Document>\n";
}

void kml_dumper::write_footer()
{
	*_out << "</Document>\n"
		<< "</kml>\n";
}


void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}

