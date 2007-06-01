// Sole top-level facility for illustration generation.
//
// Copyright (C) 2007 Gregory W. Chicares.
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
// http://savannah.nongnu.org/projects/lmi
// email: <chicares@cox.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

// $Id: illustrator.hpp,v 1.5 2007-06-01 01:24:18 chicares Exp $

#ifndef illustrator_hpp
#define illustrator_hpp

#include "config.hpp"

#include "mc_enum_type_enums.hpp" // enum mcenum_emission
#include "obstruct_slicing.hpp"
#include "so_attributes.hpp"

#include <boost/filesystem/path.hpp>

#include <functional>

/// Sole top-level facility for illustration generation.
/// TODO ?? Not yet the sole one, though.
///
/// Implicitly-declared special member functions do the right thing.

class LMI_SO illustrator
    :public std::unary_function<fs::path const&, bool>
    ,virtual private obstruct_slicing<illustrator>
{
  public:
    explicit illustrator(mcenum_emission);
    ~illustrator();

    bool operator()(fs::path const&);

  private:
    mcenum_emission emission_;
    double usec_for_input_;
    double usec_for_calculations_;
    double usec_for_output_;
};

#endif // illustrator_hpp

