// Instantiation of specific tn_range types.
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

// $Id: tn_range_types.cpp,v 1.3 2005-03-10 04:44:06 chicares Exp $

#ifdef __BORLANDC__
#   include "pchfile.hpp"
#   pragma hdrstop
#endif // __BORLANDC__

#include "tn_range.tpp"               // Template class implementation.
#include "tn_range_type_trammels.hpp" // Trammels for specific tn_range types.

// This header need not be included:
//   #include "tn_range_types.hpp"
// because it merely declares typedefs that aren't needed here.

// TODO ?? Move these two to a 'test' file.

template class tn_range<int   , percentage_trammel<int   > >;
template class tn_range<double, percentage_trammel<double> >;

template class tn_range<double, proportion_trammel     <double> >;
template class tn_range<double, nonnegative_trammel    <double> >;
template class tn_range<int   , nonnegative_trammel    <int   > >;
template class tn_range<int   , issue_age_trammel      <int   > >;
template class tn_range<int   , attained_age_trammel   <int   > >;
template class tn_range<int   , duration_trammel       <int   > >;
template class tn_range<int   , month_trammel          <int   > >;
template class tn_range<double, corridor_factor_trammel<double> >;
template class tn_range<int   , date_trammel           <int   > >;

