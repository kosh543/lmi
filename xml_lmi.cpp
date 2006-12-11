// Interface to xmlwrapp.
//
// Copyright (C) 2006 Gregory W. Chicares.
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

// $Id: xml_lmi.cpp,v 1.14 2006-12-11 18:21:29 chicares Exp $

#ifdef __BORLANDC__
#   include "pchfile.hpp"
#   pragma hdrstop
#endif // __BORLANDC__

#include "xml_lmi.hpp"

#include "alert.hpp"
#include "istream_to_string.hpp"

#include <xmlwrapp/attributes.h>
#include <xmlwrapp/document.h>
#include <xmlwrapp/init.h>
#include <xmlwrapp/tree_parser.h>

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace xml_lmi
{
/// Parse an xml file.
///
/// Precondition: argument names an accessible xml file.
///
/// Postconditions: member parser_ is a non-null pointer; the object
/// it points to is valid in that its operator!() returns false.
///
/// Throws: std::runtime_error, via fatal_error(), if a precondition
/// is violated, or if xml-library calls throw an exception derived
/// from std::exception.

xml_lmi::dom_parser::dom_parser(std::string const& filename)
    :initializer_(new Initializer)
{
    try
        {
        error_context_ = "Unable to parse xml file '" + filename + "': ";
        if(filename.empty())
            {
            throw std::runtime_error("File name is empty.");
            }
        parser_.reset(new DomParser(filename.c_str()));
        if(0 == parser_.get())
            {
            throw std::runtime_error("Parser not initialized.");
            }
        if(true == parser_->operator!())
            {
            throw std::runtime_error
                ("Parser failed: " + parser_->get_error_message()
                );
            }
        }
    catch(std::exception const& e)
        {
        fatal_error() << error_context_ << e.what() << LMI_FLUSH;
        }
}

/// Parse an xml stream.
///
/// XMLWRAPP !! xmlwrapp has no such ctor as
///   xml::tree_parser(std::istream&)
/// Therefore, read the std::istream into a std::string with
/// istream_to_string(), and pass that to the xml::tree_parser ctor
/// that takes a char* and a byte count.
///
/// Precondition: argument is an xml stream for which 0 == rdstate().
///
/// Postconditions: member parser_ is a non-null pointer; the object
/// it points to is valid in that its operator!() returns false.
///
/// Throws: std::runtime_error, via fatal_error(), if a precondition
/// is violated, or if xml-library calls throw an exception derived
/// from std::exception.

xml_lmi::dom_parser::dom_parser(std::istream& is)
    :initializer_(new Initializer)
{
    try
        {
        error_context_ = "Unable to parse xml stream: ";
        if(0 != is.rdstate())
            {
            throw std::runtime_error("Stream state is not 'good'.");
            }
        std::string s;
        istream_to_string(is, s);
        parser_.reset(new DomParser(s.c_str(), 1 + s.size()));
        if(0 == parser_.get())
            {
            throw std::runtime_error("Parser not initialized.");
            }
        if(true == parser_->operator!())
            {
            throw std::runtime_error
                ("Parser failed: " + parser_->get_error_message()
                );
            }
        }
    catch(std::exception const& e)
        {
        fatal_error() << error_context_ << e.what() << LMI_FLUSH;
        }
}

/// Throws: nothing unless member parser_'s destructor does.

xml_lmi::dom_parser::~dom_parser()
{}

/// Return the parsed document.
///
/// Preconditions: member parser_ has a document.
///
/// Throws: std::runtime_error, via fatal_error(), if a precondition
/// is violated, or if xml-library calls throw an exception derived
/// from std::exception. Ctor postconditions are assumed to have been
/// satisfied and are not tested.

xml_lmi::Document const& xml_lmi::dom_parser::document() const
{
    try
        {
        return parser_->get_document();
        }
    catch(std::exception const& e)
        {
        fatal_error() << error_context_ << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

/// Return the parsed document's root node.
///
/// Preconditions: member parser_ has a document that is not null and
/// has a root node; the argument, if not empty, matches the name of
/// that root node.
///
/// Throws: std::runtime_error, via fatal_error(), if a precondition
/// is violated, or if xml-library calls throw an exception derived
/// from std::exception. Ctor postconditions are assumed to have been
/// satisfied and are not tested.

xml_lmi::Element const& xml_lmi::dom_parser::root_node
    (std::string const& expected_name
    ) const
{
    try
        {
        xml_lmi::Document const& document = parser_->get_document();
        xml_lmi::Element const& root = document.get_root_node();
        if(!expected_name.empty() && expected_name != root.get_name())
            {
            std::ostringstream oss;
            oss
                << "Root-node name is '"
                << root.get_name()
                << "', but '"
                << expected_name
                << "' was expected."
                ;
            throw std::runtime_error(oss.str());
            }

        return root;
        }
    catch(std::exception const& e)
        {
        fatal_error() << error_context_ << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

xml_lmi::xml_document::xml_document(std::string const& root_node_name)
    :initializer_(new Initializer)
    ,document_   (new xml_lmi::Document(root_node_name.c_str()))
{
}

xml_lmi::xml_document::~xml_document()
{}

xml_lmi::Element& xml_lmi::xml_document::root_node()
{
    return document_->get_root_node();
}

std::string xml_lmi::xml_document::str()
{
    std::string s;
    document_->save_to_string(s);
    return s;
}

void add_node
    (Element&           element
    ,std::string const& name
    ,std::string const& content
    )
{
    element.push_back(xml_lmi::Element(name.c_str(), content.c_str()));
}

xml_lmi::ElementContainer child_elements
    (xml_lmi::Element const& parent
    ,std::string const& name
    )
{
    try
        {
        xml_lmi::ElementContainer z;
        typedef xml::node::const_iterator NodeConstIterator;
        for(NodeConstIterator i = parent.begin(); i != parent.end(); ++i)
            {
            if(!i->is_text() && (name.empty() || name == i->get_name()))
                {
                // Note that 'z.push_back(&*i);' does not work.
                z.push_back(i->self());
                }
            }
        return z;
        }
    catch(std::exception const& e)
        {
        fatal_error() << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

std::string get_content(Element const& element)
{
    try
        {
        std::string s;
        typedef xml::node::const_iterator NodeConstIterator;
        for(NodeConstIterator i = element.begin(); i != element.end(); ++i)
            {
            if(i->is_text())
                {
                char const* content = i->get_content();
                if(content)
                    {
                    s += i->get_content();
                    }
                }
            }
        return s;
        }
    catch(std::exception const& e)
        {
        fatal_error() << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

std::string get_name(Element const& element)
{
    try
        {
        char const* name = element.get_name();
        return name ? name : "";
        }
    catch(std::exception const& e)
        {
        fatal_error() << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

bool get_attr
    (Element const&     element
    ,std::string const& name
    ,std::string&       value
    )
{
    try
        {
        xml::attributes const& attrs = element.get_attributes();
        xml::attributes::const_iterator i = attrs.find(name.c_str());
        if(i != attrs.end())
            {
            value = i->get_value();
            return true;
            }
        else
            {
            return false;
            }
        }
    catch(std::exception const& e)
        {
        fatal_error() << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}

void set_attr
    (Element&           element
    ,std::string const& name
    ,std::string const& value
    )
{
    try
        {
        element.get_attributes().insert(name.c_str(), value.c_str());
        }
    catch(std::exception const& e)
        {
        fatal_error() << e.what() << LMI_FLUSH;
        throw std::logic_error("Unreachable"); // Silence compiler warning.
        }
}
} // namespace xml_lmi

std::ostream& operator<<(std::ostream& os, xml_lmi::xml_document const& d)
{
    os << d.document();
    return os;
}

