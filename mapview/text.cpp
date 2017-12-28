#include "text.hpp"

Gtk::Widget * __mapview_widget = nullptr;


text_layout & text_layout::ref()
{
	if (!__mapview_widget)
		throw std::logic_error{"unable to create text_layout, please call text_layout::init() first"};

	static text_layout lay;
	return lay;
}

void text_layout::init(Gtk::Widget * w)
{
	if (__mapview_widget)
		throw std::logic_error{"text_layout already initialized"};
	__mapview_widget = w;
}

Glib::RefPtr<Pango::Layout> text_layout::create_layout(std::string const & s)
{
	return __mapview_widget->create_pango_layout(s.c_str());
}
