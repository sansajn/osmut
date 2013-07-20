/*! Implementuje geometricke utility. */
#pragma once
#include <cstdint>

namespace traits
{
	//! Sprisstupnuje typ koordinatu bodu.
	template <typename P>
	struct coordinate_type {};

	//! spristupnuje koordinaty bodu.
	template <typename P, int D>
	struct access {};
};

template <int D, typename P>
inline typename traits::coordinate_type<P>::type & get(P & p)
{
	return traits::access<P, D>::get(p);
}

template <int D, typename P>
inline typename traits::coordinate_type<P>::type const & get(P const & p)
{
	return traits::access<P, D>::get(p);
}


template <class Point>
struct rect
{
	typedef typename traits::coordinate_type<Point>::type coordinate_type;

	Point p1, p2;  // left-bottom, right-top

	rect() {}

	rect(Point const & left_bottom, Point const & right_top)
		: p1(left_bottom), p2(right_top)
	{}

	coordinate_type width() const 
	{
		return get<0>(p2) - get<0>(p1);
	}

	coordinate_type height() const 
	{
		return get<1>(p2) - get<1>(p1);
	}

	Point center() const 
	{
		return Point{get<0>(p1)+width()/2, get<1>(p1)+height()/2};
	}

	void expand(Point const & p) 
	{
		if (get<0>(p) < get<0>(p1))
			get<0>(p1) = get<0>(p);

		if (get<1>(p) < get<1>(p1))
			get<1>(p1) = get<1>(p);

		if (get<0>(p) > get<0>(p2))
			get<0>(p2) = get<0>(p);

		if (get<1>(p) > get<1>(p2))
			get<1>(p2) = get<1>(p);
	}

	bool contains(Point const & p)
	{
		return get<0>(p) >= get<0>(p1) && get<1>(p) >= get<1>(p1) &&
			get<0>(p) <= get<0>(p2) && get<1>(p) <= get<1>(p2);
	}
};


//! Implementácia bodu.
struct signed_coordinate
{
	int32_t lat, lon;
};


namespace traits
{
	template <>
	struct coordinate_type<signed_coordinate> 
	{
		typedef int type;
	};

	template <>
	struct access<signed_coordinate, 0> : coordinate_type<signed_coordinate>
	{
		static type & get(signed_coordinate & p) {return p.lon;}
		static type const & get(signed_coordinate const & p) {return p.lon;}
	};

	template <>
	struct access<signed_coordinate, 1> : coordinate_type<signed_coordinate>
	{
		static type & get(signed_coordinate & p) {return p.lat;}
		static type const & get(signed_coordinate const & p)	{return p.lat;}
	};
};

