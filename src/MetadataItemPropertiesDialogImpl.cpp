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

  File: $Id$

  Contributor(s): Nando Dessena, Michael Hieke
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
//-----------------------------------------------------------------------------
#include <string>
#include <fstream>
#include <sstream>
#include "ugly.h"
#include "config.h"
#include "dberror.h"
#include "metadata/metadataitem.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "urihandler.h"
#include "MetadataItemPropertiesDialog.h"
#include "frutils.h"
//-----------------------------------------------------------------------------
myHtmlWindow::myHtmlWindow(wxWindow *parent, wxWindowID id)
	: wxHtmlWindow(parent, id)
{
}
//-----------------------------------------------------------------------------
//! Link is in format: "protocol://action?name=value&amp;name=value...etc.
void myHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	std::string addr = wx2std(link.GetHref());
	YURI uri(addr);

	if (uri.protocol != "fr")		// call default handler for other protocols
	{
		wxHtmlWindow::OnLinkClicked(link);
		return;
	}

	if (!getURIProcessor().handleURI(addr))
		::wxMessageBox(_("Feature not yet implemented."), _("Information"), wxICON_INFORMATION | wxOK);
}
//-----------------------------------------------------------------------------
//! converts chars that have special meaning in HTML, so they get displayed
std::string& escapeHtmlChars(std::string& s, bool processNewlines = true)
{
	typedef std::pair<char, std::string> par;
	std::vector<par> symbol_table;
	symbol_table.push_back(par('&', "&amp;"));		// this has to go first, since others use &
	symbol_table.push_back(par('<', "&lt;"));
	symbol_table.push_back(par('>', "&gt;"));
	symbol_table.push_back(par('"', "&quot;"));
	if (processNewlines)							// BR has to be at end, since it adds < and >
		symbol_table.push_back(par('\n', "<BR>"));

	for (std::vector<par>::iterator it = symbol_table.begin(); it != symbol_table.end(); ++it)
	{
		std::string::size_type pos;
		while (true)
		{
			pos = s.find((*it).first);
			if (pos == std::string::npos)
				break;
			s.replace(pos, 1, (*it).second);
		}
	}
	return s;
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesDialog::setPage(const std::string& type)
{
	if (type == "constraints")
		pageTypeM = ptConstraints;
	else if (type == "dependencies")
		pageTypeM = ptDependencies;
	else if (type == "triggers")
		pageTypeM = ptTableTriggers;
	// add more page types here
	// when needed
	else
		pageTypeM = ptSummary;
	loadPage();
}
//-----------------------------------------------------------------------------
//! processes commands found in HTML template

//! command is in format:   {%action:data%}
//! data field can be empty
void MetadataItemPropertiesDialog::processCommand(std::string cmd, YxMetadataItem *object, std::string& htmlpage)
{
	std::string::size_type pos = cmd.find(':');
	std::string suffix;
	if (pos != std::string::npos)
	{
		suffix = cmd.substr(pos+1);
		cmd = cmd.substr(0, pos);
	}

	if (cmd == "object_name")
		htmlpage += object->getName();

	else if (cmd == "object_type")
		htmlpage += object->getTypeName();

	else if (cmd == "object_address")
		htmlpage += wx2std(wxString::Format(wxT("%d"), (int)object));

	else if (cmd == "parent_window")
		htmlpage += wx2std(wxString::Format(wxT("%d"), (int)this));

	else if (cmd == "app_path")
		htmlpage += getApplicationPath();

	else if (cmd == "object_description")
	{
		std::string s = object->getDescription();
		if (s == "")
			s = "No description";
		htmlpage += escapeHtmlChars(s);
	}

	else if (cmd == "columns")	// table and view columns
	{
		YxMetadataItemWithColumns *m = dynamic_cast<YxMetadataItemWithColumns *>(object);
		if (!m)
			return;
		std::vector<YxMetadataItem *> tmp;
		if (m->checkAndLoadColumns() && m->getChildren(tmp))
			for (std::vector<YxMetadataItem *>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				processHtmlCode(htmlpage, suffix, *it);
	}

	else if (cmd == "triggers")	// table triggers,  triggers:after or triggers:befor  <- not a typo
	{
		YTable *t = dynamic_cast<YTable *>(object);
		if (!t)
			return;
		std::vector<YTrigger *> tmp;
		bool result;
		if (suffix.substr(0, 5) == "after")
			result = t->getTriggers(tmp, YTrigger::afterTrigger);
		else
			result = t->getTriggers(tmp, YTrigger::beforeTrigger);
		suffix.erase(0, 5);
		if (result)
		{
			for (std::vector<YTrigger *>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				processHtmlCode(htmlpage, suffix, *it);
		}
		else
			::wxMessageBox(std2wx(lastError().getMessage()), _("Error"), wxOK);
	}

	else if (cmd == "depends_on" || cmd == "depend_of")
	{
		YxMetadataItem *m = dynamic_cast<YxMetadataItem *>(object);
		if (!m)
			return;
		std::vector<Dependency> tmp;
		if (m->getDependencies(tmp, cmd == "depends_on"))
		{
			for (std::vector<Dependency>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				processHtmlCode(htmlpage, suffix, &(*it));
		}
		else
			::wxMessageBox(std2wx(lastError().getMessage()), _("Error"), wxOK);
	}

	else if (cmd == "dependency_columns")
	{
		Dependency *d = dynamic_cast<Dependency *>(object);
		if (!d)
			return;
		htmlpage += d->getFields();
	}

	else if (cmd == "primary_key")
	{
		YTable *t = dynamic_cast<YTable *>(object);
		if (!t)
			return;
		ColumnConstraint *pk = t->getPrimaryKey();
		if (!pk)
			return;
		processHtmlCode(htmlpage, suffix, pk);
	}

	else if (cmd == "foreign_keys")
	{
		YTable *t = dynamic_cast<YTable *>(object);
		if (!t)
			return;
		std::vector<ForeignKey> *fk = t->getForeignKeys();
		if (!fk)
			return;
		for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
			processHtmlCode(htmlpage, suffix, &(*it));
	}

	else if (cmd == "check_constraints")
	{
		YTable *t = dynamic_cast<YTable *>(object);
		if (!t)
			return;
		std::vector<CheckConstraint> *c = t->getCheckConstraints();
		if (!c)
			return;
		for (std::vector<CheckConstraint>::iterator it = c->begin(); it != c->end(); ++it)
			processHtmlCode(htmlpage, suffix, &(*it));
	}

	else if (cmd == "unique_constraints")
	{
		YTable *t = dynamic_cast<YTable *>(object);
		if (!t)
			return;
		std::vector<ColumnConstraint> *c = t->getUniqueConstraints();
		if (!c)
			return;
		for (std::vector<ColumnConstraint>::iterator it = c->begin(); it != c->end(); ++it)
			processHtmlCode(htmlpage, suffix, &(*it));
	}

	else if (cmd == "check_source")
	{
		CheckConstraint *c = dynamic_cast<CheckConstraint *>(object);
		if (!c)
			return;
		htmlpage += escapeHtmlChars(c->sourceM);
	}

	else if (cmd == "constraint_columns")
	{
		ColumnConstraint *c = dynamic_cast<ColumnConstraint *>(object);
		if (!c)
			return;
		htmlpage += c->getColumnList();
	}

	else if (cmd == "fk_referenced_columns" || cmd == "fk_table")
	{
		ForeignKey *fk = dynamic_cast<ForeignKey *>(object);
		if (!fk)
			return;
		if (cmd == "fk_table")
			htmlpage += fk->referencedTableM;
		else
			htmlpage += fk->getReferencedColumnList();
	}

	else if (cmd == "fk_update" || cmd == "fk_delete")	// table and view columns
	{
		ForeignKey *fk = dynamic_cast<ForeignKey *>(object);
		if (!fk)
			return;
		if (cmd == "fk_update")
			htmlpage += fk->updateActionM;
		else
			htmlpage += fk->deleteActionM;
	}

	else if (cmd == "column_datatype")
	{
		YColumn *c = dynamic_cast<YColumn *>(object);
		if (c)
		{
			YDomain *d = c->getDomain();
			htmlpage += d->getDatatypeAsString();
        	// TODO: ability to show domain name together with data type. Make it an option?
			if (!d->isSystem())
				// TODO: make the domain name a link to the domain's property page?
				htmlpage += " (" + d->getPrintableName() + ")";
		}
	}

	else if (cmd == "column_nulloption")
	{
		YColumn *c = dynamic_cast<YColumn *>(object);
		if (c)
			htmlpage += (c->isNullable() ? "" : "<b>not null</b>");
	}

	else if (cmd == "input_parameters")	// SP params
	{
		YProcedure *p = dynamic_cast<YProcedure *>(object);
		if (!p)
			return;
		std::vector<YxMetadataItem *> tmp;
		p->lockSubject();
		if (p->checkAndLoadParameters() && p->getChildren(tmp))
			for (std::vector<YxMetadataItem *>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				if (((YParameter *)(*it))->getParameterType() == ptInput)
					processHtmlCode(htmlpage, suffix, *it);
		p->unlockSubject(true, false);
	}

	else if (cmd == "output_parameters")	// SP params
	{
		YProcedure *p = dynamic_cast<YProcedure *>(object);
		if (!p)
			return;
		std::vector<YxMetadataItem *> tmp;
		p->lockSubject();
		if (p->checkAndLoadParameters() && p->getChildren(tmp))
			for (std::vector<YxMetadataItem *>::iterator it = tmp.begin(); it != tmp.end(); ++it)
				if (((YParameter *)(*it))->getParameterType() == ptOutput)
					processHtmlCode(htmlpage, suffix, *it);
		p->unlockSubject(true, false);
	}

	else if (cmd == "view_source")
	{
		YView *v = dynamic_cast<YView *>(object);
		std::string src;
		if (!v || !v->getSource(src))
			return;
		htmlpage += escapeHtmlChars(src, false);
	}

	else if (cmd == "procedure_source")
	{
		YProcedure *p = dynamic_cast<YProcedure *>(object);
		std::string src;
		if (!p || !p->getSource(src))
			return;
		htmlpage += escapeHtmlChars(src, false);
	}

	else if (cmd == "trigger_source")
	{
		YTrigger *t = dynamic_cast<YTrigger *>(object);
		std::string src;
		if (!t || !t->getSource(src))
			return;
		htmlpage += escapeHtmlChars(src, false);
	}

	else if (cmd == "trigger_info")
	{
		YTrigger *t = dynamic_cast<YTrigger *>(object);
		std::string object, type;
		bool active;
		int position;
		if (!t || !t->getTriggerInfo(object, active, position, type))
			return;
		std::string text(active ? "Active ": "Inactive ");
		text += type + " trigger for " + object + " at position ";
		std::stringstream s;
		s << text << position;
		text = s.str();
		htmlpage += escapeHtmlChars(text, false);
	}

    else if (cmd == "generator_value")
    {
		YGenerator *g = dynamic_cast<YGenerator *>(object);
		if (!g)
			return;
		std::ostringstream ss;
        ss << g->getValue();
        std::string s = ss.str();
        htmlpage += escapeHtmlChars(s, false);
    }

	else if (cmd == "exception_number")
	{
		YException *e = dynamic_cast<YException *>(object);
		if (!e)
			return;
        std::ostringstream ss;
        ss << e->getNumber();
        std::string s = ss.str();
		htmlpage += escapeHtmlChars(s, false);
	}

	else if (cmd == "exception_message")
	{
		YException *e = dynamic_cast<YException *>(object);
		if (!e)
			return;
        std::string s = e->getMessage();
		htmlpage += escapeHtmlChars(s, false);
	}

    else if (cmd == "varcolor")
	{
		static bool first = false;
		first = !first;
		pos = suffix.find('/');
		if (first)
			htmlpage += suffix.substr(0, pos);
		else
			htmlpage += suffix.substr(pos + 1);
	}
}
//-----------------------------------------------------------------------------
//! recreate html page if something changes
void MetadataItemPropertiesDialog::update()
{
	// if table or view columns change, we need to reattach
	if (objectM->getType() == ntTable || objectM->getType() == ntView)	// also observe columns
	{
		YxMetadataItemWithColumns *t = dynamic_cast<YxMetadataItemWithColumns *>(objectM);
		if (!t)
			return;
		t->checkAndLoadColumns();		// load column data if needed
		std::vector<YxMetadataItem *> temp;
		objectM->getChildren(temp);
		for (std::vector<YxMetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
			(*it)->attach(this);
	}

	// if description of procedure params change, we need to reattach
	if (objectM->getType() == ntProcedure)
	{
		YProcedure *p = dynamic_cast<YProcedure *>(objectM);
		if (!p)
			return;
		p->lockSubject();
		p->checkAndLoadParameters();		// load column data if needed
		std::vector<YxMetadataItem *> temp;
		objectM->getChildren(temp);
		for (std::vector<YxMetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
			(*it)->attach(this);
		p->unlockSubject(false, false);
	}

	loadPage();
}
//-----------------------------------------------------------------------------
//! determine the path, load and display html page
void MetadataItemPropertiesDialog::loadPage()
{
	std::string htmlpage = config().getHtmlTemplatesPath();
	switch (pageTypeM)
	{
		case ptSummary:
			htmlpage += objectM->getTypeName() + ".html";
			break;
		case ptConstraints:
			htmlpage += objectM->getTypeName() + "constraints.html";
			break;
		case ptTableTriggers:
			htmlpage += "TABLEtriggers.html";
			break;
		case ptDependencies:
			htmlpage += "dependencies.html";
			break;
	}
	processHtmlFile(htmlpage);	// load HTML template, parse, and fill the wxHTML control: window_1
}
//-----------------------------------------------------------------------------
//! closes window if observed object gets removed (disconnecting, dropping, etc)
void MetadataItemPropertiesDialog::removeObservedObject(YxSubject *object)
{
	YxObserver::removeObservedObject(object);
	if (object == objectM)	// main observed object is getting destoryed
		Close();
}
//-----------------------------------------------------------------------------
//! processes the given html template file
void MetadataItemPropertiesDialog::processHtmlFile(std::string filename)
{
	using namespace std;
	string htmlpage;		// create html page into variable

	ifstream file(filename.c_str());			// read entire file into string buffer
	if (!file)
	{
		::wxMessageBox(std2wx(filename), _("File not found"), wxICON_WARNING);
		return;
	}
	stringstream ss;
	ss << file.rdbuf();
	string s(ss.str());
	file.close();

	processHtmlCode(htmlpage, s);

	int x = 0, y = 0;
	window_1->GetViewStart(&x, &y);			// save scroll position
	window_1->SetPage(std2wx(htmlpage));
	window_1->Scroll(x, y);					// restore scroll position
}
//-----------------------------------------------------------------------------
//! processes html template code given in the htmlsource string
void MetadataItemPropertiesDialog::processHtmlCode(std::string& htmlpage, std::string htmlsource, YxMetadataItem *object)
{
	if (object == 0)
		object = objectM;

	using namespace std;
	string::size_type pos = 0, oldpos = 0, endpos = 0;
	while (true)
	{
		pos = htmlsource.find("{%", pos);
		if (pos == string::npos)
		{
			htmlpage += htmlsource.substr(oldpos);
			break;
		}

		string::size_type check, startpos = pos;
		int cnt = 1;
		while (cnt > 0)
		{
			endpos = htmlsource.find("%}", startpos+1);
			if (endpos == string::npos)
				break;

			check = htmlsource.find("{%", startpos+1);
			if (check == string::npos)
				startpos = endpos;
			else
			{
				startpos = (check < endpos ? check : endpos);
				if (startpos == check)
					cnt++;
			}
			if (startpos == endpos)
				cnt--;
			startpos++;
		}

		if (cnt > 0)	// no matching closing %}
			break;

		htmlpage += htmlsource.substr(oldpos, pos-oldpos);
		string cmd = htmlsource.substr(pos+2, endpos-pos-2);	// 2 = start_marker_len = end_marker_len
		processCommand(cmd, object, htmlpage);
		oldpos = pos = endpos+2;
	}
}
//-----------------------------------------------------------------------------
const std::string MetadataItemPropertiesDialog::getName() const
{
	return "MIPDialog";
}
//-----------------------------------------------------------------------------
const std::string MetadataItemPropertiesDialog::getStorageName() const
{
	if (storageNameM == "unassigned")
    {
        StorageGranularity g;
        if (!config().getValue("MetadataFrameStorageGranularity", g))
		    g = sgFrame;

    	switch (g)
	    {
     	    case sgFrame:
          	    storageNameM = getName();
                break;
		    case sgObjectType:
			    storageNameM = getName() + "::" + objectM->getTypeName();
                break;
		    case sgObject:
      		    storageNameM = getName() + "::" + objectM->getItemPath();
                break;
		    default:
			    storageNameM = "";
                break;
	    }
    }
    return storageNameM;
}
//-----------------------------------------------------------------------------
const YxMetadataItem *MetadataItemPropertiesDialog::getObservedObject() const
{
	return objectM;
}
//-----------------------------------------------------------------------------
const wxRect MetadataItemPropertiesDialog::getDefaultRect() const
{
	return wxRect(-1, -1, 600, 420);
}
//-----------------------------------------------------------------------------
class PageHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    static const PageHandler handlerInstance;	// singleton; registers itself on creation.
};
const PageHandler PageHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool PageHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "page")
		return false;

	std::string ms = uriObj.getParam("parent_window");		// window
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	MetadataItemPropertiesDialog *m = (MetadataItemPropertiesDialog *)mo;

	if (m)
		m->setPage(uriObj.getParam("type"));
	return true;
}
//-----------------------------------------------------------------------------
class PropertiesHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    static const PropertiesHandler handlerInstance;	// singleton; registers itself on creation.
};
const PropertiesHandler PropertiesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool PropertiesHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "properties")
		return false;

	std::string ms = uriObj.getParam("parent_window");		// window
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	MetadataItemPropertiesDialog *parent = (MetadataItemPropertiesDialog *)mo;

	NodeType n = getTypeByName(uriObj.getParam("object_type"));
	YDatabase *d = parent->getObservedObject()->getDatabase();
	if (!d)
		return true;
	YxMetadataItem *object = d->findByNameAndType(n, uriObj.getParam("object_name"));
	if (!object)
	{
		::wxMessageBox(_("Cannot find destination object\nThis should never happen."), _("Error"), wxICON_ERROR);
		return true;
	}

	// check if window with properties of that object is already open and show it
	MetadataItemPropertiesDialog *mip;
	wxWindow *mainFrame = parent->GetParent();
	if (!mainFrame)
		return true;
    for (wxWindowListNode *node = mainFrame->GetChildren().GetFirst(); node; node = node->GetNext())
    {
		wxWindow *child = node->GetData();
		mip = dynamic_cast<MetadataItemPropertiesDialog *>(child);
		if (mip && mip->getObservedObject() == object)
		{
			mip->Raise();
			return true;
		}
    }

	mip = new MetadataItemPropertiesDialog(mainFrame, object);
	mip->Show();
	return true;
}
//-----------------------------------------------------------------------------
