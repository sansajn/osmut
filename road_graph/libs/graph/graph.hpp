#pragma once
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include "range.hpp"
#include "geometry/box2.hpp"

namespace grp {  // graph (scalable graphics library), shape

using namespace geom;

unsigned const nullid = -1;

class node
{
public:
	glm::vec2 position;

	node(float x, float y);
	node(glm::vec2 const & p);
};

class vertex
{
public:
	glm::vec2 position;
	float s;  //!< pseudo curvilinear coordinate along the curve
	float l;  //!< skutocna vzdialenost od pociatku krivky
	bool control;  //!< control or regular vertex

	vertex(float x, float y, float s, bool control);
	vertex(glm::vec2 const & p, float s, bool control);
};

class curve
{
public:
	int type;
	float width;

	curve();
	void add_vertex(node * start, node * end);
	void add_vertex(float x, float y, float s, bool control);
	void add_vertex(glm::vec2 const & p, float s, bool control);
	void add_vertices(std::vector<vertex> const & verts);  // TODO: tu chcem nieco ako assign ale s move semantikou (ak to nepouzivam, tak to zmaz)
	unsigned size() const;
	glm::vec2 & xy_at(unsigned i);
	glm::vec2 const & xy_at(unsigned i) const;
	box2 const & bounds() const;
	unsigned id() const {return (unsigned)((long)(this));}
	node * first() const {return _start;}
	node * last() const {return _end;}
	float compute_curvilinear_length();  //!< spocita dlzku krivky
	// TODO: implement positions();

private:
	float _l;  //!< curve length
	node * _start;  // TODO: first,last premenuj na
	node * _end;
	std::vector<vertex> _verts;
	mutable box2 * _bounds;
};

class curve_part
{
public:
	curve_part() {}
	virtual ~curve_part() {}
	virtual glm::vec2 const & xy_at(unsigned i) const = 0;
	virtual glm::vec2 const & xy_at(glm::vec2 const & start, unsigned offset) const = 0;  //!< vrati bod offset od pociatku, alebo konca krivky
	virtual glm::vec2 const & xy_first() const = 0;
	virtual glm::vec2 const & xy_last() const = 0;
	virtual unsigned size() const = 0;
	virtual int type() const {return 0;}
	virtual float width() const {return -1;}
	virtual box2 const & bounds() const = 0;
	virtual curve_part * clip(unsigned first, unsigned last) const = 0;
	virtual void clip(box2 const & region, std::vector<curve_part *> & result) const = 0;
};

class area
{
public:
	int info;

	area() {}
	void add_curve(curve * c, int orientation = 0);
	container_range<std::vector<std::pair<curve *, int>> const> curves() const;
	box2 const & bounds() const;

	static bool clip(std::vector<curve_part *> const & cpaths, box2 const & region, std::vector<curve_part *> & result);

private:
	std::vector<std::pair<curve *, int>> _curves;  //!< (curve *, orientation)
	mutable box2 * _bounds = nullptr;
};

class basic_curve_part : public curve_part  //!< implementuje cast krivky (TODO: v podstate line-string (vyuzitie boost::geometry ?))
{
public:
	basic_curve_part(curve * c);
	basic_curve_part(curve * c, unsigned first, unsigned last);
	glm::vec2 const & xy_at(unsigned i) const override;
	glm::vec2 const & xy_at(glm::vec2 const & start, unsigned offset) const override;  //!< vrati bod offset od pociatku, alebo konca krivky
	glm::vec2 const & xy_first() const override;
	glm::vec2 const & xy_last() const override;
	unsigned size() const override;
	int type() const override {return _c->type;}
	float width() const override {return _c->width;}
	box2 const & bounds() const override;
	curve_part * clip(unsigned first, unsigned last) const override;
	void clip(box2 const & region, std::vector<curve_part *> & result) const override;  //!< vrati vysek krivky

private:
	curve * _c;
	unsigned _first, _last;  //!< curve first, last index
};

class line_curve_part : public curve_part  // TODO: v podstate segment
{
public:
	line_curve_part(glm::vec2 const & a, glm::vec2 const & b);
	glm::vec2 const & xy_at(unsigned i) const override;
	glm::vec2 const & xy_at(glm::vec2 const & start, unsigned offset) const override;
	glm::vec2 const & xy_first() const override;
	glm::vec2 const & xy_last() const override;
	unsigned size() const override;
	box2 const & bounds() const override;
	curve_part * clip(unsigned first, unsigned last) const override;
	void clip(box2 const & region, std::vector<curve_part *> & result) const override;

private:
	glm::vec2 _a, _b;
	mutable box2 * _bounds = nullptr;
};

/*! \note Implementacia sa nestara o to, ci pri vlozeni krivky v grafe existuju uzle
definujuce krivku (podobne pre oblasti). Nasledkom takejto implementacie je kumulacia
duplicit (ak ju neriesi vkladatel). */
class graph
{
public:
	graph() {}
	graph(std::vector<node *> const & nodes);
	graph(std::vector<node *> const & nodes, std::vector<curve *> const & curves);
	graph(std::vector<node *> const & nodes, std::vector<curve *> const & curves, std::vector<area *> const & areas);
	~graph();

	void add_node(node * n) {_nodes.push_back(n);}
	void add_curve(curve * c) {_curves.push_back(c);}
	void add_area(area * a) {_areas.push_back(a);}

	unsigned node_count() const {return _nodes.size();}
	unsigned curve_count() const {return _curves.size();}
	unsigned area_count() const {return _areas.size();}

	graph * clip(box2 const & clip);  // slice/clipped  ?

	container_range<std::vector<node *>> nodes() {return make_range(_nodes);}
	container_range<std::vector<node *> const> nodes() const {return make_range(_nodes);}
	node * node_at(unsigned i) const {return _nodes[i];}

	container_range<std::vector<curve *>> curves() {return make_range(_curves);}
	container_range<std::vector<curve *> const> curves() const {return make_range(_curves);}
	curve * curve_at(unsigned i) const {return _curves[i];}

	container_range<std::vector<area *>> areas() {return make_range(_areas);}
	container_range<std::vector<area *> const> areas() const {return make_range(_areas);}
	area * area_at(unsigned i) const {return _areas[i];}

	graph(graph && rhs);
	graph & operator=(graph && rhs);

	graph(graph const &) = delete;
	void operator=(graph const &) = delete;

private:
	std::vector<node *> _nodes;
	std::vector<curve *> _curves;
	std::vector<area *> _areas;
};


class reader
{
public:
	graph read(std::string const & file_name);
};

class writer
{
public:
	void write(graph const & g, std::string const & file_name);
};

}  // grp
