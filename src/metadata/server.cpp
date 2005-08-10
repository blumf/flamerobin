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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "config.h"
#include "visitor.h"
#include "server.h"
//------------------------------------------------------------------------------
Server::Server()
    : MetadataItem()
{
	typeM = ntServer;

	hostnameM = "";
	portM = "3050";

	databasesM.setParent(this);
	databasesM.setType(ntServer);
}
//------------------------------------------------------------------------------
bool Server::getChildren(std::vector<MetadataItem *>& temp)
{
	return databasesM.getChildren(temp);
}
//------------------------------------------------------------------------------
bool Server::orderedChildren() const
{
    bool ordered = false;
    config().getValue("OrderDatabasesInTree", ordered);
    return ordered;
}
//------------------------------------------------------------------------------
// returns pointer to object in vector
Database* Server::addDatabase(Database& db)
{
	Database *temp = databasesM.add(db);
	temp->setParent(this);					// grab it from collection
	notify();
	return temp;
}
//------------------------------------------------------------------------------
void Server::removeDatabase(Database *db)
{
	databasesM.remove(db);
	notify();
}
//------------------------------------------------------------------------------
void Server::createDatabase(Database *db, int pagesize, int dialect)
{
	std::ostringstream extra_params;
    if (pagesize)
        extra_params << "PAGE_SIZE " << pagesize << " ";

    std::string charset = db->getCharset();
    if (!charset.empty())
        extra_params << "DEFAULT CHARACTER SET " << charset << " ";

	IBPP::Database db1;
	db1 = IBPP::DatabaseFactory(hostnameM, db->getPath(), db->getUsername(),
        db->getPassword(), "", charset, extra_params.str());
	db1->Create(dialect);
}
//------------------------------------------------------------------------------
const MetadataCollection<Database> *Server::getDatabases() const
{
	return &databasesM;
};
//------------------------------------------------------------------------------
std::string Server::getHostname() const
{
	return hostnameM;
}
//------------------------------------------------------------------------------
std::string Server::getPort() const
{
	return portM;
}
//------------------------------------------------------------------------------
bool Server::hasConnectedDatabase() const
{
	for (MetadataCollection<Database>::const_iterator it = databasesM.begin();
		it != databasesM.end(); ++it)
	{
		if ((*it).isConnected())
			return true;
	}
	return false;
}
//------------------------------------------------------------------------------
void Server::setHostname(std::string hostname)
{
	hostnameM = hostname;	
}
//------------------------------------------------------------------------------
void Server::setPort(std::string port)
{
	portM = port;	
}
//------------------------------------------------------------------------------
const std::string Server::getTypeName() const
{
	return "SERVER";
}
//------------------------------------------------------------------------------
void Server::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------
std::string Server::getConnectionString() const
{
    std::string hostname = getHostname();
    std::string port = getPort();
    if (!hostname.empty() && !port.empty())
        return hostname + "/" + port;
    else
        return hostname;
}
//------------------------------------------------------------------------------
