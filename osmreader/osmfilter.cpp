/* vylistuje špecificku (na zaklade 'key' a 'value') way-properties
	$ listways <osm-input> <key> <value> */
#include <map>
#include <vector>
#include <utility>
//#include <regex>
#include <boost/regex.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include "osm_range.h"
#include "range.h"

using std::map;
using std::vector;
using std::pair;
//using std::regex;
using boost::regex;
using std::make_pair;
using std::string;
using std::cout;
using std::ofstream;
using osmut::xml_reader;


typedef pair<regex, regex> filter_t;
typedef vector<filter_t> filters_t;

class way_filter
{
public:
	way_filter(filters_t const & f)
		: _filters(f)
	{}

	bool operator()(way const & w) const;

private:
	bool test_filter(way const & w, filter_t const & f) const;

	filters_t const & _filters;
};


void save_osm_map(string const & osm_outfname, map<int, node> const & nodes,
	vector<way> const & ways);

filter_t parse_filter(char * argv);
void remove_unused_nodes(map<int, node> & nodes, vector<way> const & ways);
void unrecoverable_error(boost::format const & msg);


int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		unrecoverable_error(boost::format(
			"not enought parameters, 3 needed (<input>, [<filter-pair> ...], <output>)"));
	}

	cout << "used filters:\n";
	vector<filter_t> filters;
	for (int i = 2; i < argc-1; ++i)
		filters.push_back(parse_filter(argv[i]));

/*
	cout << "used filters:\n";
	for (auto & f : filters)
		cout << f.first << ":" << f.second << ", ";
*/

	string osm_infname = argv[1];
	xml_reader osm(osm_infname.c_str());

	cout << "open file '" << osm_infname << "'\n";

	cout << "reading nodes ..." << std::flush;

	// save nodes for future use
	map<int, node> nodes;
	for (auto r = make_node_range(osm); r; ++r)
		nodes[r->id] = *r;

	cout << "\n";
	cout << "reading ways ..." << std::flush;

	vector<way> ways;
	for (auto r = filtered_range(make_way_range(osm), way_filter(filters)); r; ++r)
		ways.push_back(*r);

	remove_unused_nodes(nodes, ways);

	cout << "\n";
	cout << "nodes:" << nodes.size() << ", ways:" << ways.size() << "\n";

	string osm_outfname = argv[argc-1];
	save_osm_map(osm_outfname, nodes, ways);

	return 0;
}


bool way_filter::operator()(way const & w) const
{
	if (!w.tags)
		return false;

	for (auto & f : _filters)
		if (!test_filter(w, f))
			return false;

	return true;
}

bool way_filter::test_filter(way const & w, filter_t const & f) const
{
	bool key_match = false;
	for (auto & t : *w.tags)
	{
		key_match = regex_match(t.first, f.first);
		if (key_match && regex_match(t.second, f.second))
			return true;
	}
	return false;
}

filter_t parse_filter(char * argv)
{
	string key, value;
	string argument(argv);
	string::size_type sep_pos = argument.find(':');
	if (sep_pos != string::npos)
	{
		key = string(argument.begin(), argument.begin()+sep_pos);
		value = string(argument.begin()+sep_pos+1, argument.end());
		cout << key << ":" << value << "\n";
	}
	return make_pair(regex(key/*, std::regex_constants::basic*/),
		regex(value/*, std::regex_constants::basic*/));
}

void remove_unused_nodes(map<int, node> & nodes, vector<way> const & ways)
{
	map<int, node> used;

	for (auto & w : ways)
	{
		for (auto & id : w.node_ids)
			used[id] = nodes[id];
	}

	nodes.swap(used);
}

void save_osm_map(string const & osm_outfname, map<int, node> const & nodes,
	vector<way> const & ways)
{
	ofstream o(osm_outfname.c_str());
	if (!o.is_open())
		return;

	o.precision(10);

	// header
	o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<osm version=\"0.6\" generator=\"osmfilter\">\n";

	// bounds

	// nodes
	for (auto & n : nodes)
	{
		o << "<node id=\"" << n.second.id << "\" lat=\"" << n.second.lat/1e7
			<< "\" lon=\"" << n.second.lon/1e7 << "\"/>\n";
//			<< "\" lon=\"" << n.second.lon/1e7 << "\" visible=\"true\"/>\n";
	}

	// ways
	for (auto & w : ways)
	{
		o << "<way id=\"" << w.id << "\">\n";

		for (auto & n : w.node_ids)
			o << "  <nd ref=\"" << n << "\"/>\n";

		if (w.tags)
		{
			for (auto & t : *w.tags)
				o << "  <tag k=\"" << t.first << "\" v=\"" << t.second << "\"/>\n";
		}

		o << "</way>\n";
	}

	// footer
	o << "</osm>\n";

	o.close();
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}

