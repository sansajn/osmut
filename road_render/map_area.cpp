#include <gdkmm.h>
#include "map_area.hpp"

bool map_area::on_draw(Cairo::RefPtr<Cairo::Context> const & cr)
{
	int const w = get_allocation().get_width();
	int const h = get_allocation().get_height();

	// lazy pixbuf initialization
	if (!_pixbuf || w != _pixbuf->get_width() || h != _pixbuf->get_height())
	{
		_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, w, h);
		assert(_pixbuf->get_rowstride() == w*3);
	}

	agg::rendering_buffer rbuf{_pixbuf->get_pixels(), w, h, w*3};
	pixfmt pixf{rbuf};
	ren_base renb{pixf};
	renb.clear(agg::rgba8{255, 250, 230});
	_osm.render(renb);

	assert(_pixbuf);
	Gdk::Cairo::set_source_pixbuf(cr, _pixbuf, (w - _pixbuf->get_width())/2, (h - _pixbuf->get_height())/2);
	cr->paint();

	return true;
}
