// Alert messages for wx interface.
//
// Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022 Gregory W. Chicares.
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

#include "pchfile_wx.hpp"

#include "alert.hpp"

#include "configurable_settings.hpp"
#include "force_linking.hpp"

#include <wx/app.h>                     // wxTheApp
#include <wx/frame.h>
#include <wx/msgdlg.h>
#if defined LMI_MSW
#   include <wx/msw/wrapwin.h>          // HWND etc.
#endif // defined LMI_MSW

#include <iostream>
#include <stdexcept>

LMI_FORCE_LINKING_IN_SITU(alert_wx)

namespace
{
bool volatile ensure_setup = set_alert_functions
    (status_alert
    ,warning_alert
    ,hobsons_choice_alert
    ,alarum_alert
    ,safe_message_alert
    );
} // Unnamed namespace.

/// Show a message on the statusbar, if a statusbar is available.
///
/// Otherwise, simply return. If the message were crucial, then it
/// wouldn't have been relegated to the statusbar.
///
/// Explicitly call Update() to ensure that the message is shown right
/// away. This is arguably unnecessary, but costs practically nothing;
/// see:
///   https://lists.nongnu.org/archive/html/lmi/2018-06/msg00034.html

void status_alert(std::string const& s)
{
    if(wxTheApp)
        if(wxFrame* f = dynamic_cast<wxFrame*>(wxTheApp->GetTopWindow()))
            if(wxStatusBar* b = f->GetStatusBar())
                {
                b->SetStatusText(s);
                b->Update();
                }
}

void warning_alert(std::string const& s)
{
    std::cerr << "Warning: " << s << std::endl;
    wxMessageBox(s, "Warning", wxOK, wxTheApp ? wxTheApp->GetTopWindow() : nullptr);
}

/// It seems silly to offer an option that should never be declined,
/// and then rebuke the user for declining it. However, some users
/// continue to demand this, so they continue to need reproof. Avoid
/// using this in new code.
///
/// The catch-clause throws an exception explicitly because accessing
/// configurable_settings during startup may be problematic.

void hobsons_choice_alert(std::string const& s)
{
    std::cerr << "Hobson's choice: " << s << std::endl;

    wxWindow* w = nullptr;
    if(wxTheApp)
        {
        w = wxTheApp->GetTopWindow();
        }

    if(configurable_settings::instance().offer_hobsons_choice())
        {
        int rc = wxMessageBox
            (s
            ,hobsons_prompt()
            ,wxYES_NO | wxICON_QUESTION
            ,w
            );
        if(wxYES == rc)
            {
            throw hobsons_choice_exception();
            }
        else
            {
            wxMessageBox
                (s
                ,"Warning: the result may be invalid."
                ,wxOK | wxICON_EXCLAMATION
                ,w
                );
            }
        }
    else
        {
        throw std::runtime_error(s);
        }
}

void alarum_alert(std::string const& s)
{
    std::cerr << "Alarum: " << s << std::endl;
    throw std::runtime_error(s);
}

/// Show a message reliably, even before initialization has finished
/// or after termination has begun.
///
/// The msw implementation of wxSafeShowMessage() uses ::MessageBoxA()
/// with a null parent, which adds an undesirable extra "task" to the
/// alt-Tab order, yet doesn't disable the application's top window.
///
/// If MB_TASKMODAL is specified, then the extra "task" is still
/// added, but all of the application's top windows are disabled.
/// Unfortunately, MB_TASKMODAL is in effect ignored unless the parent
/// is null.
///
/// If the main top window (the one returned by wxApp::GetTopWindow())
/// is used as the messagebox's parent, then the extra "task" is not
/// added, but only the parent is disabled. Any other top windows the
/// application may have are not disabled.
///
/// The extra "task" seeming to be the worse evil, this implementation
/// specifies a non-null parent wherever possible. MB_TASKMODAL is
/// nevertheless specified as well, though its beneficial effect is
/// realized only if no parent can be found.

void safe_message_alert(char const* message)
{
    safely_show_on_stderr(message);
#if defined LMI_MSW
    auto handle =
        (wxTheApp && wxTheApp->GetTopWindow())
        ? wxTheApp->GetTopWindow()->GetHandle()
        : nullptr
        ;
    ::MessageBoxA(handle, message, "Error", MB_OK | MB_ICONSTOP | MB_TASKMODAL);
#endif // defined LMI_MSW
}
