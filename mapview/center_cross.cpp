#include "center_cross.hpp"

void center_cross::draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	cr->save();
	cr->begin_new_sub_path();
	cr->arc(_w/2.0, _h/2.0, 5.0, 0, 2*M_PI);
	cr->stroke();
	cr->restore();
}
