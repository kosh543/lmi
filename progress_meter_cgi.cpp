// Show progress on lengthy operations--common gateway interface.
//
// Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022 Gregory W. Chicares.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
//
// https://savannah.nongnu.org/projects/lmi
// email: <gchicares@sbcglobal.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

#include "pchfile.hpp"

#include "progress_meter.hpp"

namespace
{
// Implicitly-declared special member functions do the right thing.

// Virtuals are private because no one has any business accessing
// them--not even derived classes, because deriving from this concrete
// class is not contemplated.

class concrete_progress_meter
    :public progress_meter
{
  public:
    concrete_progress_meter
        (int                max_count
        ,std::string const& title
        ,enum_display_mode
        );

    ~concrete_progress_meter() override = default;

  private:
    // progress_meter required implementation.
    std::string progress_message() const override;
    bool show_progress_message() override;
    void culminate_ui() override;
};

concrete_progress_meter::concrete_progress_meter
    (int                               max_count
    ,std::string const&                title
    ,progress_meter::enum_display_mode display_mode
    )
    :progress_meter(max_count, title, display_mode)
{
}

std::string concrete_progress_meter::progress_message() const
{
    return "";
}

bool concrete_progress_meter::show_progress_message()
{
    return true;
}

void concrete_progress_meter::culminate_ui()
{
}

std::unique_ptr<progress_meter> concrete_progress_meter_creator
    (int                               max_count
    ,std::string const&                title
    ,progress_meter::enum_display_mode mode
    )
{
    return std::make_unique<concrete_progress_meter>(max_count, title, mode);
}

bool volatile ensure_setup = set_progress_meter_creator
    (concrete_progress_meter_creator
    );
} // Unnamed namespace.
