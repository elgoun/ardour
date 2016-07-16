/*
    Copyright (C) 2016 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __ardour_push2_mix_layout_h__
#define __ardour_push2_mix_layout_h__

#include "layout.h"
#include "push2.h"

namespace ARDOUR {
	class Stripable;
}

namespace ArdourSurface {

class Push2Knob;

class MixLayout : public Push2Layout
{
   public:
	MixLayout (Push2& p, ARDOUR::Session&, Cairo::RefPtr<Cairo::Context>);
	~MixLayout ();

	bool redraw (Cairo::RefPtr<Cairo::Context>, bool force) const;
	void on_show ();

	void button_upper (uint32_t n);
	void button_lower (uint32_t n);
	void button_left ();
	void button_right ();
	void button_select_press ();
	void button_select_release ();
	void button_solo ();
	void button_mute ();

	void strip_vpot (int, int);
	void strip_vpot_touch (int, bool);

  private:
	mutable bool _dirty;
	Glib::RefPtr<Pango::Layout> upper_layout[8];
	Glib::RefPtr<Pango::Layout> lower_layout[8];
	Push2Knob* knobs[8];

	/* stripables */

	int32_t bank_start;
	PBD::ScopedConnectionList stripable_connections;
	boost::shared_ptr<ARDOUR::Stripable> stripable[8];

	PBD::ScopedConnectionList session_connections;
	void stripables_added ();

	void stripable_property_change (PBD::PropertyChange const& what_changed, int which);

	void switch_bank (uint32_t base);

	enum VPotMode {
		Volume,
		PanAzimuth,
		PanWidth,
		Send1, Send2, Send3, Send4, Send5
	};

	Push2::Button* mode_button;
	VPotMode vpot_mode;
	void show_vpot_mode ();
};

} /* namespace */

#endif /* __ardour_push2_layout_h__ */