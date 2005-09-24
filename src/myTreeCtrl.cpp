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

  $Id$

  Contributor(s): Nando Dessena
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

#include <wx/imaglist.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>

#include <stack>

#include "config/Config.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "images.h"
#include "metadata/root.h"
#include "myTreeCtrl.h"
#include "treeitem.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(myTreeCtrl, wxTreeCtrl)
    EVT_CONTEXT_MENU(myTreeCtrl::OnContextMenu)
    EVT_TREE_BEGIN_DRAG(myTreeCtrl::ID_tree_ctrl, myTreeCtrl::OnBeginDrag)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void myTreeCtrl::OnBeginDrag(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    if (item.IsOk())
    {
        SelectItem(item);
        MetadataItem *m = getMetadataItem(item);
        if (!m)
            return;
        wxString test;
        test.Printf(wxT("OBJECT:%ld"), (uintptr_t)m);
        wxTextDataObject textData(test);
        wxDropSource source(textData, this);
        source.DoDragDrop(wxDrag_AllowMove);
    }
    else
        event.Skip();
}
//-----------------------------------------------------------------------------
//! Creates context menu
void myTreeCtrl::OnContextMenu(wxContextMenuEvent& event)
{
    // select item under the mouse first, since right-click doesn't change selection under GTK
    wxPoint pos = ScreenToClient(event.GetPosition());
    int flags;
    wxTreeItemId item = HitTest(pos, flags);
    if (item.IsOk() && (flags & (wxTREE_HITTEST_ONITEMBUTTON|wxTREE_HITTEST_ONITEMICON|wxTREE_HITTEST_ONITEMLABEL)))
        SelectItem(item);
    else
    {
        item = GetSelection();
        wxRect r;
        if (item.IsOk() && GetBoundingRect(item, r, true))
        {
            pos = r.GetPosition();
            pos.y += r.height/2;
        }
    }

    wxMenu MyMenu(0);    // create context menu, depending on type of clicked item
    if (!item.IsOk())
        item = GetRootItem();
    MetadataItem* i = getMetadataItem(item);
    if (!i)
        return;
    ContextMenuMetadataItemVisitor cmv(&MyMenu);
    i->acceptVisitor(&cmv);

    PopupMenu(&MyMenu, pos);
}
//-----------------------------------------------------------------------------
myTreeCtrl::myTreeCtrl(wxWindow* parent, const wxPoint& pos, const wxSize& size, long style):
    wxTreeCtrl(parent, ID_tree_ctrl, pos, size, style)
{
    loadImages();
}
//-----------------------------------------------------------------------------
//! Override wxWidgets method, since it's buggy (doesn't handle negative values properly)
void myTreeCtrl::SetSpacing(short spacing)
{
    wxTreeCtrl::SetSpacing(spacing);
    m_spacing = spacing;
}
//-----------------------------------------------------------------------------
//! returns the object that selected wxTree node observes
MetadataItem *myTreeCtrl::getSelectedMetadataItem()
{
    return getMetadataItem(GetSelection());
}
//-----------------------------------------------------------------------------
//! returns the database of the object that selected wxTree node observes
Database *myTreeCtrl::getSelectedDatabase()
{
    MetadataItem *m = getSelectedMetadataItem();
    if (!m)
        return 0;
    return m->getDatabase();
}
//-----------------------------------------------------------------------------
//! returns the server of the object that selected wxTree node observes
Server *myTreeCtrl::getSelectedServer()
{
    MetadataItem *m = getSelectedMetadataItem();
    Server *s = dynamic_cast<Server *>(m);
    if (s)
        return s;
    Database *d = getSelectedDatabase();
    if (!d)
        return 0;
    return d->getServer();
}
//-----------------------------------------------------------------------------
//! returns the object that some wxTree node observes
MetadataItem *myTreeCtrl::getMetadataItem(wxTreeItemId item)
{
    if (!item.IsOk())
        return 0;

    TreeItem *d = (TreeItem *)(GetItemData(item));
    if (!d)
        return 0;

    return d->getObservedMetadata();
}
//-----------------------------------------------------------------------------
//! returns index of image in wxImageList for given NodeType
int myTreeCtrl::getItemImage(NodeType t)
{
    return imageMapM[t];
}
//-----------------------------------------------------------------------------
//! creates wxImageList from icons in tree
void myTreeCtrl::loadImages()
{
    for (int i=0; i<ntLastType; i++)
        imageMapM[i] = 0;

    wxImageList* imageList = new wxImageList(16, 16);
    for (int i=0; i<ntLastType; i++)
    {
        wxBitmap bmp = getImage((NodeType)i);
        if (!bmp.Ok())
            continue;
        wxIcon icon;
        icon.CopyFromBitmap(bmp);
        imageMapM[i] = imageList->Add(icon);
    }
    AssignImageList(imageList);    // autodeleted
}
//-----------------------------------------------------------------------------
bool myTreeCtrl::selectMetadataItem(MetadataItem* item)
{
    if (item == 0)
        return false;
    // create a stack of parent metadata items (break before root node)
    std::stack<MetadataItem*> metaitems;
    metaitems.push(item);
    MetadataItem* parent = item->getParent();
    while (parent != 0 && parent->getParent() != 0)
    {
        metaitems.push(parent);
        parent = parent->getParent();
    }
    // walk stack of metadata items to find item in treenode hierarchy
    wxTreeItemId parentNode = GetRootItem();
    while (!metaitems.empty())
    {
        parent = metaitems.top();
        wxTreeItemIdValue cookie;
        wxTreeItemId node = GetFirstChild(parentNode, cookie);
        while (node.IsOk())
        {
            MetadataItem* nodeItem = getMetadataItem(node);
            if (parent == nodeItem)
            {
                if (item == nodeItem)
                {
                    SelectItem(node);
                    EnsureVisible(node);
                    return true;
                }
                metaitems.pop();
                parentNode = node;
                break;
            }
            node = GetNextChild(parentNode, cookie);
        }
        if (!node.IsOk())    // not found
            break;
    }
    return false;
}
//-----------------------------------------------------------------------------
