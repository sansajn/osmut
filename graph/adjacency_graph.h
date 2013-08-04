#include <list>
#include <vector>

template <typename Edge>
class adjacency_graph
{
public:
	typedef unsigned int vertex_descriptor;
	typedef Edge edge_descriptor;

	void append_edge(vertex_descriptor s, edge_descriptor e) {	_data[s].push_back(e);}

	std::list<edge_descriptor> & operator[](vertex_descriptor v) {return _data[v];}
	std::list<edge_descriptor> const & operator[](vertex_descriptor v) const {return _data[v];}

	size_t vertices() const {return _data.size();}

	void resize(size_t n) {_data.resize(n);}

private:
	std::vector<std::list<edge_descriptor>> _data;
};

