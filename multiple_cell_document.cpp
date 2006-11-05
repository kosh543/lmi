// xml document for multiple-cell illustrations.
//
// Copyright (C) 2002, 2004, 2005, 2006 Gregory W. Chicares.
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

// $Id: multiple_cell_document.cpp,v 1.13 2006-11-05 01:23:04 chicares Exp $

#ifdef __BORLANDC__
#   include "pchfile.hpp"
#   pragma hdrstop
#endif // __BORLANDC__

#include "multiple_cell_document.hpp"

#include "alert.hpp"
#include "inputillus.hpp"
#include "value_cast.hpp"
#include "xml_lmi.hpp"

//============================================================================
multiple_cell_document::multiple_cell_document()
    :case_parms_  (1)
    ,class_parms_ (1)
    ,cell_parms_  (1)
{
}

//============================================================================
multiple_cell_document::multiple_cell_document(std::string const& filename)
{
    xml_lmi::dom_parser parser(filename);
    parse(parser.root_node(xml_root_name()));
}

//============================================================================
multiple_cell_document::~multiple_cell_document()
{
}

//============================================================================
std::string multiple_cell_document::xml_root_name() const
{
    return "multiple_cell_document";
}

//============================================================================
void multiple_cell_document::parse(xml_lmi::Element const& root)
{
// TODO ?? It doesn't seem right to depend on node order.
// See note below--perhaps do something like this:
//    int NumberOfCases;
//    is >> NumberOfCases;
//    LMI_ASSERT(1 == NumberOfCases);

    IllusInputParms temp;

    xml_lmi::ElementContainer const elements(xml_lmi::child_elements(root));
    typedef xml_lmi::ElementContainer::const_iterator eci;
    eci i = elements.begin();

    // Case default parameters.

    case_parms_.clear();

    LMI_ASSERT(i != elements.end());
    if(std::string("cell") != (*i)->get_name())
        {
        fatal_error()
            << "XML node name is '"
            << (*i)->get_name()
            << "' but '"
            << "cell"
            << "' was expected."
            << LMI_FLUSH
            ;
        }
    (**i) >> temp;
    case_parms_.push_back(temp);

    // Number of classes.
    ++i;
    LMI_ASSERT(i != elements.end());
    if(std::string("NumberOfClasses") != (*i)->get_name())
        {
        fatal_error()
            << "XML node name is '"
            << (*i)->get_name()
            << "' but '"
            << "NumberOfClasses"
            << "' was expected."
            << LMI_FLUSH
            ;
        }
    char const* n_classes = (*i)->get_content();
    LMI_ASSERT(n_classes);
    unsigned int number_of_classes = value_cast<unsigned int>(n_classes);

    // Parameters for each class.
    class_parms_.clear();
    class_parms_.reserve(number_of_classes);

    ++i;
    for(; i != elements.end(); ++i)
        {
        (**i) >> temp;
        class_parms_.push_back(temp);
        if(class_parms_.size() == number_of_classes)
            {
            break;
            }
        }
    if(class_parms_.size() != number_of_classes)
        {
        fatal_error()
            << "Number of classes read is "
            << class_parms_.size()
            << " but should have been "
            << number_of_classes
            << "."
            << LMI_FLUSH
            ;
        }

    // Number of cells.
    LMI_ASSERT(i != elements.end());
    ++i;
    LMI_ASSERT(i != elements.end());
    if(std::string("NumberOfCells") != (*i)->get_name())
        {
        fatal_error()
            << "XML node name is '"
            << (*i)->get_name()
            << "' but '"
            << "NumberOfCells"
            << "' was expected."
            << LMI_FLUSH
            ;
        }
    char const* n_cells = (*i)->get_content();
    LMI_ASSERT(n_cells);
    unsigned int number_of_cells = value_cast<unsigned int>(n_cells);

    // Parameters for each cell.
    cell_parms_.clear();
    cell_parms_.reserve(number_of_cells);

    ++i;
    for(; i != elements.end(); ++i)
        {
        (**i) >> temp;
        cell_parms_.push_back(temp);
        status()
            << "Read "
            << cell_parms_.size()
            << " of "
            << number_of_cells
            << " lives."
            << std::flush
            ;
        if(cell_parms_.size() == number_of_cells)
            {
            break;
            }
        }
    if(cell_parms_.size() != number_of_cells)
        {
        fatal_error()
            << "Number of individuals read is "
            << cell_parms_.size()
            << " but should have been "
            << number_of_cells
            << "."
            << LMI_FLUSH
            ;
        }

    LMI_ASSERT(i != elements.end());
    ++i;
    if(i != elements.end())
        {
        fatal_error()
            << "Read all data expected in XML document, "
            << "but more data remains."
            << LMI_FLUSH
            ;
        }
}

//============================================================================
void multiple_cell_document::read(std::istream& is)
{
    xml_lmi::dom_parser parser(is);
    parse(parser.root_node(xml_root_name()));
}

//============================================================================
void multiple_cell_document::write(std::ostream& os) const
{
    xml_lmi::Element root(xml_root_name().c_str());

// TODO ?? Diagnostics will be cryptic if the xml doesn't follow
// the required layout. Perhaps they could be improved. Maybe it
// would be better to restructure the document so that each set
// of cells, with its cardinal number, is a distinct node, e.g.:
//
//    root.push_back
//        (xml_lmi::Element
//            ("NumberOfCases"
//            ,value_cast<std::string>(case_parms_.size()).c_str()
//            )
//        );
    root << case_parms_[0];

    root.push_back
        (xml_lmi::Element
            ("NumberOfClasses"
            ,value_cast<std::string>(class_parms_.size()).c_str()
            )
        );
    for(unsigned int j = 0; j < class_parms_.size(); j++)
        {
        root << class_parms_[j];
        }

    root.push_back
        (xml_lmi::Element
            ("NumberOfCells"
            ,value_cast<std::string>(cell_parms_.size()).c_str()
            )
        );
    for(unsigned int j = 0; j < cell_parms_.size(); j++)
        {
        root << cell_parms_[j];
        }

    os << root;
}

