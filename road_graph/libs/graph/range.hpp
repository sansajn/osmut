#pragma once
#include <type_traits>

template <typename Cond, typename T, typename F>
struct if_const
{
	using type = F;
};

template <typename Cond, typename T, typename F>
struct if_const<Cond const, T, F>
{
	using type = T;
};

template <typename Iter>
struct map_value_iterator  // TODO: nie je kompatibilny s stl
{
	using reference = typename if_const<
		typename std::remove_reference<typename Iter::reference>::type,
		typename Iter::value_type::second_type const &,  // const type
		typename Iter::value_type::second_type &>::type;  // non const type

	map_value_iterator(Iter it) : _it(it) {}
	reference operator*() {return _it->second;}
	void operator++() {++_it;}
	bool operator!=(map_value_iterator rhs) const {return _it != rhs._it;}
	Iter _it;
};

/*! Iteruje hodnoty mapy
\code
	map<int, string> data;
	data[3] = "three";
	data[1] = "one";
	data[5] = "five";

	for (auto & v : make_value_range(data))
		cout << v << ", ";
\endcode */
template <typename Map>
struct map_value_range
{
	using iterator = map_value_iterator<
		typename if_const<Map, typename Map::const_iterator, typename Map::iterator>::type>;

	map_value_range(Map & m) : _m(m) {}
	iterator begin() {return iterator{_m.begin()};}
	iterator end() {return iterator{_m.end()};}
	Map & _m;
};

template <typename Map>  // TODO: make_map_value_range
map_value_range<Map> make_value_range(Map & m) {return map_value_range<Map>{m};}


/*! Iteruje hodnoty stl kontajnera
Umoznuje skryt samotny kontajner (napr. pri navratovej hodnote funkcie).
\code
	vector<int> data{1,2,3};
	for (auto & v : make_range(data))
		cout << v << ", ";
\endcode */
template <typename Container>
struct container_range
{
	using iterator = typename if_const<
		Container, typename Container::const_iterator, typename Container::iterator>::type;

	container_range(Container & c) : _c{c} {}
	iterator begin() {return _c.begin();}
	iterator end() {return _c.end();}
	Container & _c;
};

template <typename Container>
container_range<Container> make_range(Container & c) {return container_range<Container>{c};}
