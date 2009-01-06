// Emit a ledger in various guises.
//
// Copyright (C) 2005, 2006, 2007, 2008, 2009 Gregory W. Chicares.
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
// email: <gchicares@sbcglobal.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

// $Id: emit_ledger.hpp,v 1.7 2009-01-06 15:01:22 chicares Exp $

#ifndef emit_ledger_hpp
#define emit_ledger_hpp

#include "config.hpp"

#include "mc_enum_type_enums.hpp" // enum mcenum_emission
#include "so_attributes.hpp"

#include <boost/filesystem/path.hpp>

class Ledger;

double LMI_SO emit_ledger
    (fs::path const& filepath
    ,fs::path const& tsv_filepath
    ,Ledger const&   ledger
    ,mcenum_emission emission
    );

#endif // emit_ledger_hpp

