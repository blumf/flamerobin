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

  Contributor(s):
*/

//
//
//
//
//------------------------------------------------------------------------------
#ifndef FR_METADATAITEMWITHCOLUMNS_H
#define FR_METADATAITEMWITHCOLUMNS_H

#include "metadataitem.h"
#include "collection.h"
#include "column.h"
//------------------------------------------------------------------------------
class YxMetadataItemWithColumns: public YxMetadataItem
{
protected:
	YMetadataCollection <YColumn>columnsM;
public:
	bool getChildren(std::vector<YxMetadataItem *>& temp);
	bool checkAndLoadColumns();
	virtual bool loadColumns();
	YColumn *addColumn(YColumn &c);
	YxMetadataItemWithColumns();
};
//------------------------------------------------------------------------------
#endif
