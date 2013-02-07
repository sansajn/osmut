#pragma once
#include <cstring>
#include <cassert>
#include "osmff.h"
#include "osm_parser.h"

template <class ReaderPolicy>
class tag_iterator
{
public:
	typedef typename ReaderPolicy::value_type value_type;
	typedef value_type * pointer;
	typedef value_type & reference;
	typedef value_type const & const_reference;

	tag_iterator() : _osm(nullptr), _buf(nullptr) {}
	tag_iterator(osmut::parser & xml, value_type & buf);
	tag_iterator & operator++() {next(); return *this;}
	reference operator*() {return *_buf;}
	const_reference & operator*() const {return *_buf;}
	pointer operator->() const {return _buf;}
	bool operator==(tag_iterator const & rhs) const;

	void next();

private:
	bool process_node();

private:
	osmut::parser * _osm;
	value_type * _buf;
};

template <class ReaderPolicy>
bool operator!=(tag_iterator<ReaderPolicy> const & lhs, 
	tag_iterator<ReaderPolicy> const & rhs)
{
	return !(lhs == rhs);
}

template <class ReaderPolicy>
tag_iterator<ReaderPolicy>::tag_iterator(osmut::parser & osm, 
	value_type & buf)	: _osm(&osm), _buf(&buf)
{
	osmut::parser::e_node node_type = _osm->node_type();

	if (node_type == osmut::parser::start_node
		&& ReaderPolicy::tag(_osm->node_name()))
	{
		ReaderPolicy::read_tag(*_osm, *_buf);
	}
	else
		next();
}

template <class ReaderPolicy>
void tag_iterator<ReaderPolicy>::next()
{
	while (_buf && _osm && _osm->read() && !process_node())
		continue;
}

template <class ReaderPolicy>
bool tag_iterator<ReaderPolicy>::process_node()
{
	std::string const & node_name = _osm->node_name();

	switch (_osm->node_type())
	{
		case osmut::parser::start_node:
		case osmut::parser::empty_node:
		{
			if (ReaderPolicy::tag(node_name))
				ReaderPolicy::read_tag(*_osm, *_buf);
			else if (ReaderPolicy::stop_tag(node_name))  
				_buf = nullptr;  // end
			else
				return false;

			return true;
		}

		case osmut::parser::end_node:
		{
			if (node_name == OSM_TAG)
				_buf = nullptr;
			else
				return false;

			return true;
		}

		default:
			assert(false && "unexpected element type inside 'osm'");
	}
	return false;
}

template <class ReaderPolicy>
bool tag_iterator<ReaderPolicy>::operator==(tag_iterator const & rhs) const
{
	return _buf == rhs._buf;
}

