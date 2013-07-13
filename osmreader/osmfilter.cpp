/* na zaklade filtrou do subora dumpnespecificke property
	ukazka: ./osmfilter in.osm type:boundary admin_level:2 --poly out.dat */
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <boost/regex.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include "osm_range.h"
#include "range.h"

using std::map;
using std::set;
using std::vector;
using std::pair;
using std::make_pair;
using std::swap;
using boost::regex;
using std::string;
using std::cout;
using std::ofstream;
using osmut::xml_reader;


typedef pair<regex, regex> filter_t;
typedef vector<filter_t> filters_t;


class relation_filter
{
public:
	relation_filter(filters_t const & f)
		: _filters(f)
	{}

	bool operator()(relation const & r) const;

private:
	bool test_filter(relation const & r, filter_t const & f) const;

	filters_t const & _filters;
};


void list_used_elems(vector<relation> const & relations, vector<way> const & ways,
	set<int> & used_relations, set<int> & used_ways, set<int> & used_nodes);

void save_osm_map(string const & osm_outfname, vector<node> const & nodes,
	vector<way> const & ways, vector<relation> const & relations);

template <typename T>
void remove_unused_ids(vector<T> & from, set<int> const & used);

filter_t parse_filter(char * argv);
void unrecoverable_error(boost::format const & msg);


int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		unrecoverable_error(boost::format(
			"not enought parameters, 3 needed (<input> [<filter-pair> ...] [--<opt> ...] <output>)"));
	}

	cout << "used filters:\n";
	vector<filter_t> filters;
	for (int i = 2; i < argc-1; ++i)
	{
		if (argv[i][0] != '-' && argv[i][1] != '-')
			filters.push_back(parse_filter(argv[i]));
		else
			break;
	}

	bool poly_output = false;
	for (int i = 2; i < argc-1; ++i)
		if (strcmp(argv[i], "--poly") == 0)
			poly_output = true;

	string osm_infname = argv[1];
	xml_reader osm(osm_infname.c_str());

	cout << "open file '" << osm_infname << "'\n";

	cout << "reading nodes ..." << std::flush;
	vector<node> nodes;
	for (auto r = make_node_range(osm); r; ++r)
		nodes.push_back(*r);

	cout << "\n";
	cout << "reading ways ..." << std::flush;
	vector<way> ways;
	for (auto r = make_way_range(osm); r; ++r)
		ways.push_back(*r);

	cout << "\n";
	cout << "reading relations ..." << std::flush;
	vector<relation> relations;
	for (auto r = filtered_range(make_relation_range(osm), relation_filter(filters)); r; ++r)
		relations.push_back(*r);

	set<int> used_relations, used_ways, used_nodes;
	list_used_elems(relations, ways, used_relations, used_ways, used_nodes);

	remove_unused_ids(nodes, used_nodes);
	remove_unused_ids(ways, used_ways);

	cout << "\n";
	cout << "nodes:" << nodes.size() << ", ways:" << ways.size()
		<< ", relations: " << used_relations.size() << "\n";
	cout << "missed relations: " << used_relations.size() << "\n";

	string osm_outfname = argv[argc-1];
	save_osm_map(osm_outfname, nodes, ways, relations);

	return 0;
}


void list_used_elems(vector<relation> const & relations, vector<way> const & ways,
	set<int> & used_relations, set<int> & used_ways, set<int> & used_nodes)
{
	for (auto & r : relations)
	{
		for (auto & m : r.members)
		{
			if (m.type == member::relation_type)
				used_relations.insert(m.ref);
			else if (m.type == member::way_type)
				used_ways.insert(m.ref);  // use hint for better performance
			else if (m.type == member::node_type)
				used_nodes.insert(m.ref);
		}
	}

	for (auto & w : ways)
	{
		if (used_ways.find(w.id) != used_ways.end())
			for (auto & id : w.node_ids)
				used_nodes.insert(id);
	}
}

bool relation_filter::operator()(relation const & r) const
{
	if (!r.tags)
		return false;

	for (auto & f : _filters)
		if (!test_filter(r, f))
			return false;

	return true;
}

bool relation_filter::test_filter(relation const & r, filter_t const & f) const
{
	bool key_match = false;
	for (auto & t : *r.tags)
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
	return make_pair(regex(key),	regex(value));
}

template <typename T>
void remove_unused_ids(vector<T> & from, set<int> const & used)
{
	int end_idx = from.size();
	for (int i = 0; i < end_idx; ++i)
	{
		if (used.find(from[i].id) == used.end())  // nenasiel sa
		{
			swap(from[i], from[end_idx-1]);
			--end_idx;
			--i;  // check a new from[i] once more time
		}
	}
	from.resize(end_idx);  // removes all unused
}

void save_osm_map(string const & osm_outfname, vector<node> const & nodes,
	vector<way> const & ways, vector<relation> const & relations)
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
		o << "<node id=\"" << n.id << "\" lat=\"" << n.lat/1e7 << "\" lon=\""
			<< n.lon/1e7 << "\"/>\n";
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

	// relations
	for (auto & r : relations)
	{
		o << "<relation id=\"" << r.id << "\">\n";

		for (auto & m : r.members)
		{
			string type = "way";
			if (m.type == member::node_type)
				type = "node";
			else if (m.type == member::relation_type)
				type = "relation";

			o << "  <member type=\"" << type << "\" ref=\"" << m.ref
				<< "\" role=\"" << m.role << "\"/>\n";
		}

		if (r.tags)
		{
			for (auto & t : *r.tags)
				o << "  <tag k=\"" << t.first << "\" v=\"" << t.second << "\"/>\n";
		}

		o << "</relation>\n";
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
