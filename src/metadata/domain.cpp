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

  Contributor(s): Nando Dessena
*/

//------------------------------------------------------------------------------
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

#include <sstream>
#include <string>
#include <ibpp.h>
#include "dberror.h"
#include "database.h"
#include "domain.h"
//------------------------------------------------------------------------------
YDomain::YDomain():
	YxMetadataItem()
{
	typeM = ntDomain;
	infoLoadedM = false;	// I had a 2 hour session with debugger to found out that this was missing
}
//------------------------------------------------------------------------------
bool YDomain::loadInfo()
{
	IBPP::Database& db = getDatabase()->getDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"select t.rdb$type, f.rdb$field_sub_type, f.rdb$field_length,"
			" f.rdb$field_precision, f.rdb$field_scale, c.rdb$character_set_name"
			" from rdb$fields f"
			" join rdb$types t on f.rdb$field_type=t.rdb$type"
			" left outer join rdb$character_sets c on c.rdb$character_set_id = f.rdb$character_set_id"
			" where f.rdb$field_name = ?"
			" and t.rdb$field_name='RDB$FIELD_TYPE'"
		);

		st1->Set(1, getName());
		st1->Execute();
		st1->Fetch();
		st1->Get(1, &datatypeM);
		if (st1->IsNull(2))
			subtypeM = 0;
		else
			st1->Get(2, &subtypeM);
		st1->Get(3, &lengthM);
		if (st1->IsNull(4))
			precisionM = 0;
		else
			st1->Get(4, &precisionM);
		if (st1->IsNull(5))
			scaleM = 0;
		else
			st1->Get(5, &scaleM);
		if (st1->IsNull(6))
			charsetM = "";
		else
		{
			st1->Get(6, charsetM);
			charsetM.erase(charsetM.find_last_not_of(" ")+1);
		}

		tr1->Commit();
		notify();
		infoLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
//! returns column's datatype as human readable string. It can also be used to construct DDL for tables
std::string YDomain::getDatatypeAsString()
{
	if (!infoLoadedM)
		loadInfo();

	std::ostringstream retval;		// this will be returned

	// special case (mess that some tools (ex. IBExpert) make by only setting scale and not changing type)
	if (datatypeM == 27 && scaleM < 0)
	{
		retval << "Numeric(15," << -scaleM << ")";
		return retval.str();
	}

	// LONG&INT64: INT/SMALLINT (prec=0), DECIMAL(sub_type=2), NUMERIC(sub_type=1)
	if (datatypeM == 7 || datatypeM == 8 || datatypeM == 16)
	{
		if (scaleM == 0)
		{
			if (datatypeM == 7)
				return "Smallint";
			else if (datatypeM == 8)
				return "Integer";
			else
				return "Numeric(18,0)";
		}
		else
		{
			retval << (subtypeM == 2 ? "Decimal(" : "Numeric(");
			if (precisionM <= 0 || precisionM > 18)
				retval << 18;
			else
				retval << precisionM;
			retval << "," << -scaleM << ")";
			return retval.str();
		}
	}

	std::string names[] = {
		"Char",
		"Float",
		"Double precision",
		"Timestamp",
		"Varchar",
		"Blob",
		"Date",
		"Time"
	};
	short mapper[8] = { 14, 10, 27, 35, 37, 261, 12, 13 };

	for (int i=0; i<8; ++i)
	{
		if (mapper[i] == datatypeM)
		{
			retval << names[i];
			break;
		}
	}

	if (datatypeM == 14 || datatypeM == 37)	// char & varchar, add (length)
		retval << "(" << lengthM << ")";

	if (datatypeM == 261)	// blob
		retval << " sub_type " << subtypeM;

	return retval.str();
}
//------------------------------------------------------------------------------
void YDomain::getDatatypeParts(std::string& type, std::string& size, std::string& scale)
{
	using namespace std;
	string datatype = getDatatypeAsString();
	string::size_type p1 = datatype.find("(");
	if (p1 != string::npos)
	{
		type = datatype.substr(0, p1);
		string::size_type p2 = datatype.find(",");
		if (p2 == string::npos)
			p2 = datatype.find(")");
		else
		{
			string::size_type p3 = datatype.find(")");
			scale = datatype.substr(p2+1, p3-p2-1);
		}
		size = datatype.substr(p1+1, p2-p1-1);
	}
	else
		type = datatype;
}
//------------------------------------------------------------------------------
std::string YDomain::getCharset()
{
	if (!infoLoadedM)
		loadInfo();

	return charsetM;
}
//------------------------------------------------------------------------------
std::string YDomain::getCreateSqlTemplate() const
{
	return	"CREATE DOMAIN domain_name\n"
            "AS datatype\n"
            "DEFAULT {literal | NULL | USER}\n"
            "[NOT NULL]\n"
            "[CHECK (dom_search_condition)]\n"
            "COLLATE collation;\n";
}
//------------------------------------------------------------------------------
const std::string YDomain::getTypeName() const
{
	return "DOMAIN";
}
//------------------------------------------------------------------------------

