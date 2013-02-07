#pragma once
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include <boost/utility.hpp>
#include <boost/format.hpp>


namespace osmut 
{

	namespace parser_detail
	{

template <int BufferSize = 4096>
class file_content : boost::noncopyable
{
public:
	file_content(char const * file_name);
	~file_content();
	bool is_open() const {return _fin.is_open();}
	char get();
	void getline(std::string & s);
	bool eof() const {return !_fin && (_bufidx == size());}

private:
	void alloc() {_buf = new char[BufferSize];}
	void free() {delete [] _buf; _buf = nullptr;}	
	void fill() {_fin.read(_buf, BufferSize);}
	int size() const {return _fin.gcount();}

private:
	std::ifstream _fin;
	char * _buf;
	int _bufidx;
};

template <int BufferSize>
file_content<BufferSize>::file_content(char const * file_name) 
	: _fin(file_name, std::ios::in|std::ios::binary), _bufidx(0) 
{
	alloc();
	fill();
}

template <int BufferSize>
file_content<BufferSize>::~file_content() 
{
	free();
	_fin.close();
}

template <int BufferSize>
char file_content<BufferSize>::get() 
{
	if (!eof() && _bufidx == size())
	{
		fill();
		_bufidx = 0;
	}
	return _buf[_bufidx++];
}

template <int BufferSize>
void file_content<BufferSize>::getline(std::string & s) 
{
	char ch = get();
	while (!eof() && ch != '\n')
	{
		s += ch;
		ch = get();
	}
}

	};  // parser_detail

//! osm-xml parser implementation (based on r5)
class parser
{
public:
	typedef std::pair<std::string, std::string> keyval_t;

	enum e_node {undefined_node, start_node, end_node, empty_node};

	parser(char const * file_name);
	~parser() {}
	bool read();

	std::string const & node_name() const {return _curnode.name;}
	e_node node_type() const {return _curnode.type;}
	int attribute_count() const {return _curnode.attribs.size();}

	std::string const & attribute_name(int idx) const {
		return _curnode.attribs[idx].first;
	}
	
	std::string const & attribute_value(int idx) const {
		return _curnode.attribs[idx].second;
	}

	keyval_t const & attribute(int idx) const {
		return _curnode.attribs[idx];
	}

	// error handling
	bool good() const {return !_error;}
	boost::format const & error_message() const {return _errmsg;}

private:
	void node();
	void start_tag();
	void end_tag();

	void eat_pi_section();

	void get_sym();
	void get_complex_sym();
	void read_name();
	void read_text();
	void get_nospace_char();
	void get_char();

	void error(char const * msg);
	void error(boost::format const & msg);
	void unrecoverable_error(boost::format const & msg);
	std::string offset();

private:
	enum e_symbol
	{
		tag_opener = 1,
		tag_closer,
		end_tag_opener,    // </
		empty_tag_closer,  // />
		pi_opener, 
		pi_closer, 
		equall, 
		text, 
		name,
		eof
	};

	struct node_data
	{
		std::string name;
		e_node type;
		std::vector<keyval_t> attribs;  // spôsobuje 4-nasobné spomalenie parsingu
	};

	node_data _curnode;

	// low-level-parser
	e_symbol sym;
	char ch;
	char next_ch;
	std::string name_buf;
	std::string text_buf;

	parser_detail::file_content<4096> _osm;

	// error handling
	bool _error;
	boost::format _errmsg;

	// debug
	int tabs;
};  // parser


};  // osmut

