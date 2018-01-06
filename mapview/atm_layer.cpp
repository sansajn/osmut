#include <iostream>
#include <boost/format.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include "atm_layer.hpp"

using std::vector;
using std::string;
using std::cout;
using glm::dvec2;

struct node_visitor : public osmium::handler::Handler
{
	node_visitor(geo_point_layer & points, vector<atm_layer::atm_desc> & atms)
		: _points{points}, _atms{atms}
	{}

	void node(osmium::Node const & n);

private:
	geo_point_layer & _points;
	vector<atm_layer::atm_desc> & _atms;
};

void node_visitor::node(osmium::Node const & n)
{
	osmium::TagList const & tags = n.tags();
	char const * name = tags["name"];
	_atms.push_back(atm_layer::atm_desc{_atms.size(), string{name ? name : "unknown"}});

	osmium::Location const & loc = n.location();
	_points.add_point(dvec2{loc.lat(), loc.lon()});
}

atm_layer::atm_layer(fs::path const & atm_osm)
{
	if (!fs::exists(atm_osm))
		throw std::runtime_error{boost::str(boost::format{"file {} doesn't exists"} % atm_osm)};

	osmium::io::File fin{atm_osm.string()};
	osmium::io::Reader reader{fin};

	node_visitor vis{*this, _atms};
	osmium::apply(reader, vis);

	reader.close();

	cout << "atm count = " << _atms.size() << std::endl;
}
