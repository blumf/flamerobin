/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  Contributor(s): Michael Hieke, Nando Dessena
*/

// -*- C++ -*- generated by wxGlade 0.2.2 on Fri Jul 02 11:09:50 2004

#ifndef SERVERREGISTRATIONINFOFRAME_H
#define SERVERREGISTRATIONINFOFRAME_H

#include <wx/wx.h>
#include "metadata/server.h"
#include "BaseDialog.h"


class ServerRegistrationInfoFrame: public BaseDialog {
public:
    // begin wxGlade: ServerRegistrationInfoFrame::ids
    enum {
		ID_textctrl_server = 100,
        ID_button_ok = wxID_OK ,
        ID_button_cancel = wxID_CANCEL
    };
    // end wxGlade

	void setServer(YServer *s);

	// events
	void OnSettingsChange(wxCommandEvent& event);
	void OnOkButtonClick(wxCommandEvent& event);
	void OnCancelButtonClick(wxCommandEvent& event);

    ServerRegistrationInfoFrame(wxWindow* parent, int id, const wxString& title, 
        const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, 
        long style=wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

private:
	YServer *serverM;

    // begin wxGlade: ServerRegistrationInfoFrame::methods
    void set_properties();
    void do_layout();
    // end wxGlade
	void updateButtons();

protected:
    // begin wxGlade: ServerRegistrationInfoFrame::attributes
    wxStaticText* label_1;
    wxTextCtrl* text_ctrl_1;
    wxStaticText* label_2;
    wxTextCtrl* text_ctrl_2;
    wxButton* button_ok;
    wxButton* button_cancel;
    // end wxGlade

    DECLARE_EVENT_TABLE()
    virtual const std::string getName() const;
};


#endif // SERVERREGISTRATIONINFOFRAME_H
