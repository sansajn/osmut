#include "graph.hpp"
#include <algorithm>
#include <set>
#include <map>
#include <limits>
#include <cmath>
#include <cassert>
#include <fstream>
#include <exception>
#include <glm/vec2.hpp>

namespace grp {

using std::string;
using std::move;
using std::vector;
using std::pair;
using std::ifstream;
using std::ofstream;
using std::numeric_limits;
using std::min;
using std::max;
using std::swap;
using std::set;
using std::map;
using std::runtime_error;
using glm::vec2;

// geometry helpers
bool contains(box2 const & b, glm::vec2 const & p);
bool segment_intersects(box2 const & region, glm::vec2 const & a, glm::vec2 const & b);
bool intersects(box2 const & a, box2 const & b);
bool intersects(box2 const & b, glm::vec2 const & p);

graph::graph(std::vector<node *> const & nodes)
	: _nodes{nodes}
{}

graph::graph(std::vector<node *> const & nodes, std::vector<curve *> const & curves)
	: _nodes{nodes}, _curves{curves}
{}

graph::graph(std::vector<node *> const & nodes, std::vector<curve *> const & curves, std::vector<area *> const & areas)
	: _nodes{nodes}, _curves{curves}, _areas{areas}
{}

graph::graph(graph && rhs)
	: _nodes{move(rhs._nodes)}, _curves{move(rhs._curves)}, _areas{move(rhs._areas)}
{}

graph & graph::operator=(graph && rhs)
{
	_nodes.swap(rhs._nodes);
	_curves.swap(rhs._curves);
	_areas.swap(rhs._areas);
	return *this;
}

graph::~graph()
{
	for (auto n : _nodes)
		delete n;
	_nodes.clear();

	for (auto c : _curves)
		delete c;
	_curves.clear();

	for (auto a : _areas)
		delete a;
	_areas.clear();
}

//! \note neriesi duplicity v grafe
void add_curve_part(curve_part const & p, area * a_dst, graph & g_dst)
{
	node * first = new node{p.xy_first()};
	node * last = new node{p.xy_last()};

	curve * c = new curve;
	c->type = p.type();
	c->width = p.width();
	c->add_vertex(first, last);
	for (int i = 1; i < p.size()-1; ++i)
	{
		glm::vec2 const & pos = p.xy_at(i);
		c->add_vertex(pos.x, pos.y, i, false);
	}

	g_dst.add_node(first);
	g_dst.add_node(last);
	g_dst.add_curve(c);

	if (a_dst)
		a_dst->add_curve(c, 0);
}

graph * graph::clip(box2 const & clip)
{
	graph * result = new graph;

	set<unsigned> visited;

	// [areas]
	for (area * a : _areas)
	{
		if (!intersects(clip, a->bounds()))
			continue;

		// TODO: cpaths mozu byt kopie
		vector<curve_part *> cpaths;  // TODO: toto je nonsens, preco area neponuka taku funkciu, respektive klip bez toho vektora curve_part ?
		for (pair<curve *, int> const & c : a->curves())
		{
			cpaths.push_back(new basic_curve_part{c.first});
			visited.insert(c.first->id());
		}

		vector<curve_part *> paths;
		area::clip(cpaths, clip, paths);
//		assert(!paths.empty() && "predpokladam, ze paths nieje prazdne");

		if (!paths.empty())
		{
			area * clipped = new area;
			clipped->info = a->info;
			for (curve_part * c : paths)
				add_curve_part(*c, clipped, *result);
			result->add_area(clipped);
		}

		for (curve_part * c : paths)
			delete c;

		for (curve_part * c : cpaths)
			delete c;
	}

	// [curves]
	for (curve * c : _curves)
	{
		if (visited.count(c->id()))
			continue;

		if (!intersects(clip, c->bounds()))
			continue;

		basic_curve_part cpart{c};
		vector<curve_part *> clipped;
		cpart.clip(clip, clipped);

		for (curve_part * part : clipped)
		{
			add_curve_part(*part, nullptr, *result);
			delete part;
		}
	}

	return result;
}


class file_reader
{
public:
	using pos_type = std::ifstream::pos_type;

	file_reader(std::string const & file_name);
	~file_reader() {close();}

	template <typename T>
	T read();

	template <typename T>
	void ignore(unsigned count);

	pos_type tell() {return _fin.tellg();}
	void seek(pos_type pos) {_fin.seekg(pos);}
	void close() {_fin.close();}
	bool binary() const {return _binary;}
	bool indexed() const {return _indexed;}

private:
	std::ifstream _fin;
	bool _binary, _indexed;
};

file_reader::file_reader(std::string const & file_name)
{
	_fin.open(file_name);
	if (!_fin.is_open())
		throw runtime_error{string{"unable to open '"} + file_name + string{"' file"}};

	int format;
	_fin.read((char *)&format, sizeof(int));

	_binary = (format == 0 || format == 1);
	_indexed = (format == 1 || format == '1');

	if (!_binary)
		seek(2);  // TODO: preco nie 1 ?
}

template <typename T>
T file_reader::read()
{
	T result;
	if (_binary)
		_fin.read((char *)&result, sizeof(T));
	else
		_fin >> result;
	return result;
}

template <typename T>
void file_reader::ignore(unsigned count)
{
	if (_binary)
		_fin.ignore(sizeof(T)*count);
	else
	{
		T dummy;
		for (int i = 0; i < count; ++i)
			_fin >> dummy;
	}
}


static graph read(file_reader & fin)
{
	// TODO: subgraph support

	assert(!fin.indexed() && "expect un-indexed graph file");

	// [header]
	int node_param_count = fin.read<int>();
	int curve_param_count = fin.read<int>();
	int area_param_count = fin.read<int>();
	int curve_extremities_param_count = fin.read<int>();
	int curve_point_param_count = fin.read<int>();
	int area_curve_param_count = fin.read<int>();
	int subgraph_param_count = fin.read<int>();

	// [nodes]
	int node_count = fin.read<int>();

	vector<node *> nodes(node_count);
	for (int i = 0; i < node_count; ++i)
	{
		float x = fin.read<float>();
		float y = fin.read<float>();
		fin.ignore<float>(node_param_count - 2);
		int size = fin.read<int>();
		fin.ignore<int>(size);
		nodes[i] = new node{x, y};
	}

	// [curves]
	int curve_count = fin.read<int>();

	vector<curve *> curves(curve_count);
	for (int i = 0; i < curve_count; ++i)
	{
		int vertex_count = fin.read<int>();
		float width = fin.read<float>();
		int type = fin.read<int>();
		fin.ignore<float>(curve_param_count - 3);

		//  [vertices]
		int start_id = fin.read<int>();
		assert(start_id < nodes.size() && "start-id out of range");
		fin.ignore<float>(curve_extremities_param_count - 1);

		vector<vertex> verts;
		verts.reserve(vertex_count);
		for (int j = 1; j < vertex_count - 1; ++j)
		{
			float x = fin.read<float>();
			float y = fin.read<float>();
			int control = fin.read<int>();
			verts.emplace_back(vertex{x, y, -1, control == 1});
			fin.ignore<float>(curve_point_param_count - 3);
		}

		int end_id = fin.read<int>();
		assert(end_id < nodes.size() && "end-id out of range");
		fin.ignore<float>(curve_extremities_param_count - 1);
		fin.ignore<int>(2);

		int parent_id = fin.read<int>();
		assert(parent_id == nullid && "ignorujem parent-id");

		curve * c = new curve;
		c->width = width;
		c->type = type;
		c->add_vertex(nodes[start_id], nodes[end_id]);
		c->add_vertices(verts);

		curves[i] = c;
	}

	// [areas]
	int area_count = fin.read<int>();

	vector<area *> areas(area_count);
	for (int i = 0; i < area_count; ++i)
	{
		int size = fin.read<int>();
		int info = fin.read<int>();
		int subgraph = fin.read<int>();
		fin.ignore<float>(area_param_count - 3);

		vector<pair<int, int>> curve_ids;  // curve-id, orientation
		curve_ids.reserve(size);
		for (int j = 0; j < size; ++j)
		{
			int index = fin.read<int>();
			int orientation = fin.read<int>();
			assert(index < curves.size() && "curve index out of range");
			fin.ignore<float>(area_curve_param_count - 2);
			curve_ids.emplace_back(index, orientation);
		}

		fin.ignore<float>(subgraph_param_count);

		int parent_id = fin.read<int>();
		assert(parent_id == nullid && "ignorujem parent-id");

		area * a = new area;
		a->info = info;
		for (auto & id : curve_ids)
			a->add_curve(curves[id.first], id.second);

		areas[i] = a;
	}

	return graph{nodes, curves, areas};
}

static graph read_indexed(file_reader & fin)
{
	// TODO: subgraph support

	assert(fin.indexed() && "expect indexed graph file");

	// [header]
	int node_param_count = fin.read<int>();
	int curve_param_count = fin.read<int>();
	int area_param_count = fin.read<int>();
	int curve_extrem_param_count = fin.read<int>();
	int curve_verts_param_count = fin.read<int>();
	int area_curves_param_count = fin.read<int>();
	int area_subgraphs_param_count = fin.read<int>();

	uint32_t offset = fin.read<uint32_t>();  // index map offset
	long begin = fin.tell();

	fin.seek(offset);
	int node_count = fin.read<int>();
	int curve_count = fin.read<int>();
	int area_count = fin.read<int>();
	int subgraph_count = fin.read<int>();
	assert(subgraph_count == 0 && "subgraphs not implemented");

	// [nodes]
	fin.seek(begin);
	vector<grp::node *> nodes(node_count);
	for (int i = 0; i < node_count; ++i)
	{
		float x = fin.read<float>();
		float y = fin.read<float>();
		nodes[i] = new node{vec2{x,y}};
		fin.ignore<float>(node_param_count - 2);
		int size = fin.read<int>();
		fin.ignore<int>(size);
	}

	// [curves]
	vector<curve *> curves(curve_count);
	for (int i = 0; i < curve_count; ++i)
	{
		int size = fin.read<int>();
		float width = fin.read<float>();
		int type = fin.read<int>();
		fin.ignore<float>(curve_param_count-3);

		int start = fin.read<int>();
		fin.ignore<float>(curve_extrem_param_count-1);

		vector<vertex> verts;
		for (int j = 1; j < size-1; ++j)
		{
			float x = fin.read<float>();
			float y = fin.read<float>();
			int control = fin.read<int>();
			fin.ignore<float>(curve_verts_param_count-3);
			verts.push_back(vertex{x, y, -1.0f, control == 1});
		}

		int end = fin.read<int>();
		fin.ignore<float>(curve_extrem_param_count-1);

		fin.ignore<int>(2);

		unsigned parent_id =	fin.read<unsigned>();
		assert(parent_id == nullid && "ignorujem parent-id");

		curve * c = new curve;
		c->width = width;
		c->type = type;
		c->add_vertex(nodes[start], nodes[end]);
		c->add_vertices(verts);

		curves[i] = c;
	}

	// [areas]
	vector<area *> areas(area_count);
	for (int i = 0; i < area_count; ++i)
	{
		int size = fin.read<int>();
		int info = fin.read<int>();
		int subgraph = fin.read<int>();
		fin.ignore<float>(area_param_count-3);
		assert(subgraph == 0 && "subgraphs not supported");

		vector<pair<int, int>> curve_ids;  // curve-id, orientation
		curve_ids.reserve(size);
		for (int j = 0; j < size; ++j)
		{
			int index = fin.read<int>();
			int orientation = fin.read<int>();
			fin.ignore<float>(area_curves_param_count-2);
			curve_ids.emplace_back(index, orientation);
		}

		fin.ignore<float>(area_subgraphs_param_count);

		unsigned parent_id = fin.read<unsigned>();
		assert(parent_id == nullid && "ignorujem parent-id");

		area * a = new area;
		a->info = info;
		for (auto & id : curve_ids)
			a->add_curve(curves[id.first], id.second);

		areas[i] = a;
	}

	return graph{nodes, curves, areas};
}

graph reader::read(std::string const & file_name)
{
	file_reader fin{file_name};

	if (fin.indexed())
		return grp::read_indexed(fin);
	else
		return grp::read(fin);
}


template <typename T>
void write(ofstream & fout, T const & val)
{
	fout.write((char *)&val, sizeof(T));
}

void writer::write(graph const & g, std::string const & file_name)
{
	// TODO: moznost nastavit mod, default je binary-indexed

	ofstream fout{file_name};
	if (!fout.is_open())
		throw runtime_error{"unable to open a file for writing"};

	// [header]
	grp::write<uint32_t>(fout, 1);  // format: binary-indexed
	grp::write<uint32_t>(fout, 2);  // node_param_count
	grp::write<uint32_t>(fout, 3);  // curve_param_count
	grp::write<uint32_t>(fout, 3);  // area_param_count
	grp::write<uint32_t>(fout, 1);  // curve_extrem_param_count
	grp::write<uint32_t>(fout, 3);  // curve_verts_param_count
	grp::write<uint32_t>(fout, 2);  // area_curve_param_count
	grp::write<uint32_t>(fout, 0);  // area_subgraphs_param_count

	// index offset
	uint32_t idx_offset = fout.tellp();
	grp::write<uint32_t>(fout, -1);  // index-offset (dummy value)

	// [nodes]
	map<node *, unsigned> node_indices;
	unsigned count = 0;
	for (node * n : g.nodes())
	{
		grp::write(fout, n->position.x);
		grp::write(fout, n->position.y);
		grp::write<uint32_t>(fout, 0);  // size
		node_indices[n] = count++;
	}

	// [curves]
	for (curve * c : g.curves())
	{
		grp::write(fout, c->size());
		grp::write(fout, c->width);
		grp::write(fout, c->type);

		auto first_it = node_indices.find(c->first());
		assert(first_it != node_indices.end() && "undefinned node");
		grp::write(fout, first_it->second);

		for (unsigned i = 1; i < c->size()-1; ++i)
		{
			vec2 const & p = c->xy_at(i);
			grp::write(fout, p.x);
			grp::write(fout, p.y);
			grp::write<int>(fout, 0);  // all vertices are regular points, TODO: control support
		}

		auto last_it = node_indices.find(c->last());
		assert(last_it != node_indices.end() && "undefinned node");
		grp::write(fout, last_it->second);

		grp::write<int32_t>(fout, 0);  // write 2 ints
		grp::write<int32_t>(fout, 0);

		grp::write<uint32_t>(fout, -1);  // parent-id, TODO: parrent support
	}

	// TODO: support areas

	// index map
	uint32_t idx_offset_value = fout.tellp();
	grp::write<uint32_t>(fout, g.node_count());
	grp::write<uint32_t>(fout, g.curve_count());
	grp::write<uint32_t>(fout, g.area_count());
	grp::write<uint32_t>(fout, 0);  // subgraph count

	fout.seekp(idx_offset);
	grp::write(fout, idx_offset_value);

	fout.close();
}


node::node(float x, float y)
	: position{x,y}
{}

node::node(glm::vec2 const & p) : position{p}
{}

vertex::vertex(float x, float y, float s, bool control)
	: position{x, y}, s{s}, l{-1}, control{control}
{}

vertex::vertex(glm::vec2 const & p, float s, bool control)
	: position{p}, s{s}, l{-1}, control{control}
{}

curve::curve()
	: type{0}, width{0}, _start{nullptr}, _end{nullptr}, _bounds{nullptr}
{}

void curve::add_vertex(node * start, node * end)
{
	_start = start;
	_end = end;
}

void curve::add_vertex(float x, float y, float s, bool control)
{
	_verts.emplace_back(x, y, s, control);
}

void curve::add_vertex(glm::vec2 const & p, float s, bool control)
{
	_verts.emplace_back(p, s, control);
}

void curve::add_vertices(std::vector<vertex> const & verts)
{
	_verts.reserve(_verts.size() + verts.size());
	_verts.insert(_verts.end(), verts.begin(), verts.end());
}

unsigned curve::size() const
{
	return _verts.size()+2;
}

glm::vec2 & curve::xy_at(unsigned i)
{
	assert(i < size() && "index mimo rozsahu");

	if (i == 0)
		return _start->position;
	else if (i <= _verts.size())
		return _verts[i-1].position;
	else
		return _end->position;
}

glm::vec2 const & curve::xy_at(unsigned i) const
{
	assert(i < size() && "index mimo rozsahu");

	if (i == 0)
		return _start->position;
	else if (i <= _verts.size())
		return _verts[i-1].position;
	else
		return _end->position;
}

float curve::compute_curvilinear_length()
{
	float len = 0.0f;
	glm::vec2 * a = &_start->position;
	for (auto & v : _verts)
	{
		glm::vec2 * b = &v.position;
		len += glm::distance(*a, *b);
		v.l = len;
		a = b;
	}
	len += glm::distance(*a, _end->position);

	_l = len;
	return len;
}

box2 const & curve::bounds() const
{
	if (!_bounds)
	{
		float xmin = numeric_limits<float>::max();
		float xmax = numeric_limits<float>::lowest();
		float ymin = numeric_limits<float>::max();
		float ymax = numeric_limits<float>::lowest();

		for (int i = 0; i < size(); ++i)
		{
			glm::vec2 const & p = xy_at(i);
			xmin = min(xmin, p.x);
			xmax = max(xmax, p.x);
			ymin = min(ymin, p.y);
			ymax = max(ymax, p.y);
		}

		_bounds = new box2{glm::vec2{xmin, ymin}, glm::vec2{xmax, ymax}};
	}

	return *_bounds;
}

basic_curve_part::basic_curve_part(curve * c)
	: _c{c}, _first{0}, _last{c->size()-1}
{}

basic_curve_part::basic_curve_part(curve * c, unsigned first, unsigned last)
	: _c{c}, _first{first}, _last{last}
{}

bool intersects(box2 const & a, box2 const & b)
{
//	return !(a.max_corner().x < b.min_corner().x || a.max_corner().y < b.min_corner().y);  // TODO: toto je podozrivo jednoduche

	return a.max_corner().x >= b.min_corner().x && a.min_corner().x <= b.max_corner().x &&
		a.max_corner().y >= b.min_corner().y && a.min_corner().y <= b.max_corner().y;
}

bool intersects(box2 const & b, glm::vec2 const & p)
{
	return p.x >= b.min_corner().x && p.x <= b.max_corner().x &&
		p.y >= b.min_corner().y && p.y <= b.max_corner().y;
}

bool contains(box2 const & b, glm::vec2 const & p)
{
	return intersects(b, p);
}

bool segment_intersects(box2 const & region, glm::vec2 const & a, glm::vec2 const & b)  // segment intersects
{
	if (contains(region, a) || contains(region, b))
		return true;

	box2 ab{a,b};
	if (ab.max_corner().x < region.min_corner().x || ab.min_corner().x > region.max_corner().x
		 || ab.max_corner().y < region.min_corner().y || ab.min_corner().y > region.max_corner().y)
	{
		return false;
	}

	return true;  // conservative result (TODO: ? je mozna chyba)
}

//! uhol medzi dvoma vektormi (v intervale 0, 2pi)
float angle(glm::vec2 const & u, glm::vec2 const & v)
{
	float a = atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
	return a > 0 ? a : a + 2*M_PI;
}

void basic_curve_part::clip(box2 const & region, std::vector<curve_part *> & result) const
{
	unsigned first = -1;

	for (int i = 0; i < size()-1; ++i)
	{
		glm::vec2 const & a = xy_at(i);
		glm::vec2 const & b = xy_at(i+1);  // TODO: segment a intersects

		if (segment_intersects(region, a, b))
		{
			if (first == -1)
				first = i;
		}
		else
		{
			if (first != -1)
			{
				result.push_back(clip(first, i));
				first = -1;
			}
		}
	}

	if (first != -1)
		result.push_back(clip(first, size()-1));
}

curve_part * basic_curve_part::clip(unsigned first, unsigned last) const
{
	return new basic_curve_part{_c, _first + first, _first + last};
}

box2 const & basic_curve_part::bounds() const
{
	return _c->bounds();
}

glm::vec2 const & basic_curve_part::xy_at(unsigned i) const
{
	return _c->xy_at(_first + i);
}

glm::vec2 const & basic_curve_part::xy_at(glm::vec2 const & start, unsigned offset) const
{
	return start == xy_first() ? _c->xy_at(_first + offset) : _c->xy_at(_last - offset);
}

glm::vec2 const & basic_curve_part::xy_first() const
{
	return _c->xy_at(_first);
}

glm::vec2 const & basic_curve_part::xy_last() const
{
	return _c->xy_at(_last);
}

unsigned basic_curve_part::size() const
{
	return _last - _first + 1;
}

line_curve_part::line_curve_part(glm::vec2 const & a, glm::vec2 const & b)
	: _a{a}, _b{b}
{}

glm::vec2 const & line_curve_part::xy_at(unsigned i) const
{
	return i == 0 ? _a : _b;
}

glm::vec2 const & line_curve_part::xy_at(glm::vec2 const & start, unsigned offset) const
{
	assert(offset <= 1 && "line-curve-part ma iba dva body (pociatok a koniec)");
	return start == xy_first() ? xy_last() : xy_first();
}

glm::vec2 const & line_curve_part::xy_first() const
{
	return _a;
}

glm::vec2 const & line_curve_part::xy_last() const
{
	return _b;
}

unsigned line_curve_part::size() const
{
	return 2;
}

box2 const & line_curve_part::bounds() const
{
	if (!_bounds)
		_bounds = new box2{_a, _b};
	return *_bounds;
}

curve_part * line_curve_part::clip(unsigned first, unsigned last) const
{
	assert(first == 0 && last == 1 && "line-curve-part ma iba 2 body");
	return new line_curve_part{_a, _b};
}

void line_curve_part::clip(box2 const & region, std::vector<curve_part *> & result) const
{
	if (intersects(region, _a) || intersects(region, _b))
		result.push_back(clip(0, 1));
}

void area::add_curve(curve * c, int orientation)
{
	_curves.emplace_back(c, orientation);
}

container_range<std::vector<std::pair<curve *, int>> const> area::curves() const
{
	return make_range(_curves);
}

box2 const & area::bounds() const
{
	if (!_bounds)
	{
		float xmin = numeric_limits<float>::max();
		float xmax = numeric_limits<float>::lowest();
		float ymin = numeric_limits<float>::max();
		float ymax = numeric_limits<float>::lowest();

		for (int i = 0; i < _curves.size(); ++i)
		{
			box2 const & b = _curves[i].first->bounds();
			xmin = min(xmin, b.min_corner().x);
			xmax = max(xmax, b.max_corner().x);
			ymin = min(ymin, b.min_corner().y);
			ymax = max(ymax, b.max_corner().y);
		}

		_bounds = new box2{glm::vec2{xmin, ymin}, glm::vec2{xmax, ymax}};
	}
	return *_bounds;
}

bool area::clip(vector<curve_part *> const & cpaths, box2 const & region, vector<curve_part *> & result)
{
	// [najdi krivky v oblasti]
	for (curve_part * path : cpaths)
		if (intersects(region, path->bounds()))
			path->clip(region, result);

	if (result.empty())
		return true;

	// [spoj volne konce rozseknutych kriviek] TODO: preco?
	vector<glm::vec2> exterior_points;
	for (curve_part * c : result)
	{
		if (!intersects(region, c->xy_first()))
			exterior_points.push_back(c->xy_first());

		if (!intersects(region, c->xy_last()))
			exterior_points.push_back(c->xy_last());
	}

//	assert(exterior_points.size() % 2 == 0);  // TODO: co v pripadoch ak je pocet bodov mimo neparny

	bool first_point_inside = intersects(region, result[0]->xy_first());
	for (int i = first_point_inside ? 0 : 1; i < exterior_points.size(); i += 2)
	{
		glm::vec2 p0 = exterior_points[i];
		glm::vec2 p1 = exterior_points[(i+1) % exterior_points.size()];
		if (p0.x != p1.x || p0.y != p1.y)  // test p0 != p1 inak
			result.push_back(new line_curve_part(p0, p1));
	}

	// [poprehadzuj krivky, tak aby nasledovali za sebou (v zmysle pozicii)] - spatial_reorder
	glm::vec2 cur = result[0]->xy_last();
	for (int i = 1; i < result.size(); ++i)
	{
		curve_part * ic = result[i];

		bool connected_found = false;
		for (int j = i; j < result.size(); ++j)
		{
			curve_part * jc = result[j];
			if (jc->xy_first() == cur || jc->xy_last() == cur)
			{
				if (connected_found)  // already one connected curve found, decide based segments angles
				{
					glm::vec2 prev = result[i-1]->xy_at(cur, 1);
					glm::vec2 pi = ic->xy_at(cur, 1);
					glm::vec2 pj = jc->xy_at(cur, 1);
					float ai = angle(prev - cur, pi - cur);
					float aj = angle(prev - cur, pj - cur);
					if (ai < aj)
						swap(result[i], result[j]);
				}
				else
				{
					swap(result[i], result[j]);
					connected_found = true;
				}
			}

		}  // for j

		assert(connected_found && "predpokladam existenciu spojenia");

		if (!connected_found)
			return false;

		cur = cur == ic->xy_first() ? ic->xy_last() : ic->xy_first();
	}  // for i

	return true;
}

}  // grp
