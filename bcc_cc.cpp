// Frontend for borland C++ compilers.
//
// Copyright (C) 2001, 2004, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022 Gregory W. Chicares.
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

#include "getopt.hpp"
#include "handle_exceptions.hpp"        // report_exception()
#include "license.hpp"
#include "system_command.hpp"

#include <iostream>
#include <string>

int try_main(int argc, char* argv[])
{
    int c;
    int option_index = 0;
    // Long options are meta-options not directly supported by the borland
    // compiler. TRICKY !! They are aliased to unlikely octal values.
    static Option long_options[] =
      {
        {"help",         NO_ARG,   nullptr, 001, nullptr, "display this help and exit"},
        {"license",      NO_ARG,   nullptr, 002, nullptr, "display license and exit"},
        {"accept",       NO_ARG,   nullptr, 003, nullptr, "accept license (-l to display)"},
        {"program",      REQD_ARG, nullptr, 004, nullptr, "compiler's filepath"},
        {"include",      REQD_ARG, nullptr, 'I', nullptr, "include path"},
        {"out-file",     REQD_ARG, nullptr, 'o', nullptr, "output object file"},
        {nullptr,        NO_ARG,   nullptr, 000, nullptr, ""}
      };

    bool license_accepted = false;
    bool show_license     = false;
    bool show_help        = false;

    std::string out_file;
    std::string program;

    std::string compiler_options;

    GetOpt getopt_long
        (argc
        ,argv
        ,""
        ,long_options
        ,&option_index
        ,true
        );
    getopt_long.opterr = false;
    while(EOF != (c = getopt_long()))
        {
        switch(c)
            {
            case 001:
                {
                show_help = true;
                }
                break;

            case 002:
                {
                show_license = true;
                }
                break;

            case 003:
                {
                license_accepted = true;
                }
                break;

            case 004:
                {
                program = getopt_long.optarg;
                }
                break;

            case 'I':
                {
                std::string s(getopt_long.optarg);
                compiler_options += "-I" + s + " ";
                }
                break;

            case 'o':
                {
                out_file = getopt_long.optarg;
                }
                break;

            case '?':
                {
                // Forward any unrecognized options to the tool.
                int offset = getopt_long.optind - 1;
                std::string s;
                if(0 < offset)
                    {
                    s = getopt_long.nargv[offset];
                    compiler_options += s + " ";
                    }
                else
                    {
                    std::cerr << "Internal error\n";
                    }
                }
                break;

            default:
                std::cerr << "getopt returned character code 0" << c << '\n';
            }
        }

    if((c = getopt_long.optind) < argc)
        {
        while(c < argc)
            {
            // Forward any unrecognized arguments to the tool.
            //
            // Replace any leading slash with a backslash, because
            // this compiler doesn't recognize posix paths.
            // Accordingly, always indicate options with '-',
            // never with '/'.
            std::string s(argv[c++]);
            if('/' == s.at(0))
                {
                s.at(0) = '\\';
                }
            compiler_options += s + " ";
            }
        }

    if(!license_accepted)
        {
        std::cerr << license_notices_as_text() << "\n\n";
        }

    if(show_license)
        {
        std::cerr << license_as_text() << "\n\n";
        return EXIT_SUCCESS;
        }

    if(show_help)
        {
        getopt_long.usage();
        return EXIT_SUCCESS;
        }

    if("" == program)
        {
        std::cerr << "Must specify --program=(compiler name)\n";
        }

    // '-oout_file' must precede the name of the source file (which we
    // parse as part of 'compiler_options') or else it's ignored, because
    // the borland compiler treats the order of its arguments as
    // significant.
    std::string command_line =
          program
        + " "
        + "-o"
        + out_file
        + " "
        + compiler_options
        ;
    try
        {
        system_command(command_line);
        }
    catch(...)
        {
        report_exception();
        std::cerr
            << "Command failed."
            << "\ncommand line: " << command_line
            << '\n'
            << "\nout_file: "     << out_file
            << "\nprogram: "      << program
            << "\noptions: "      << compiler_options
            << '\n'
            ;
        }
    return 0;
}
