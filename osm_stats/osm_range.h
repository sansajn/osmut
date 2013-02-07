#pragma once
#include "osm_iter.h"

template <class Iterator>
class osm_range
{
public:
	typedef typename Iterator::pointer pointer;
	typedef typename Iterator::reference reference;
	typedef typename Iterator::const_reference const_reference;

	osm_range(Iterator first, Iterator last)
		: _curr(first), _end(last)
	{}

	reference operator*() {return *_curr;}
	const_reference operator*() const {return *_curr;}
	pointer operator->() const {return _curr.operator->();}

	void operator++() 
	{
		assert(_curr != _end && "increments not valid range"); 
		++_curr;	
	}

	operator bool() const {return _curr != _end;}

	bool operator==(osm_range const & rhs) const 
	{
		return _curr == rhs._curr && _end == rhs._end;
	}

	bool operator!=(osm_range const & rhs) const {return !(*this == rhs);}

private:
	Iterator _curr, _end;
};


typedef osm_range<node_iterator> node_range;
typedef osm_range<way_iterator> way_range;


inline node_range make_node_r(osmut::parser & osm, node & buf) 
{
	return node_range(node_iterator(osm, buf), node_iterator());
}

inline way_range make_way_r(osmut::parser & osm, way & buf)
{
	return way_range(way_iterator(osm, buf), way_iterator());
}

