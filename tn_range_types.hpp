// Declaration of specific tn_range types.
//
// Copyright (C) 2004, 2005 Gregory W. Chicares.
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// http://savannah.nongnu.org/projects/lmi
// email: <chicares@cox.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

// $Id: tn_range_types.hpp,v 1.2 2005-02-19 03:27:45 chicares Exp $

#ifndef tn_range_types_hpp
#define tn_range_types_hpp

#include "config.hpp"

#include "tn_range_fwd.hpp"           // Template class forward declaration.
#include "tn_range_type_trammels.hpp" // Trammels for specific tn_range types.

typedef tn_range<int,    percentage_trammel<int   > > r_int_percentage;
typedef tn_range<double, percentage_trammel<double> > r_double_percentage;

#endif // tn_range_types_hpp

