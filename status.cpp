////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "status.h"
#include "database.h"

#include "connection.h"
#include "networkmessage.h"
#include "outputmessage.h"

#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;

IpConnectMap ProtocolStatus::ipConnectMap;

void ProtocolStatus::onRecvFirstMessage(NetworkMessage& msg)
{
	uint32_t ip = getIP();
	if(ip != LOCALHOST)
	{
		IpConnectMap::const_iterator it = ipConnectMap.find(ip);
		if(it != ipConnectMap.end() && OTSYS_TIME() < it->second + g_config.getNumber(ConfigManager::STATUSQUERY_TIMEOUT))
		{
			getConnection()->close();
			return;
		}

		ipConnectMap[ip] = OTSYS_TIME();
	}

	uint8_t type = msg.get<char>();
	switch(type)
	{
		case 0xFF:
		{
			if(msg.getString(false, 4) == "info")
			{
				if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
				{
					TRACK_MESSAGE(output);
					if(Status* status = Status::getInstance())
					{
						bool sendPlayers = false;
						if(msg.size() > msg.position())
							sendPlayers = msg.get<char>() == 0x01;

						output->putString(status->getStatusString(sendPlayers), false);
					}

					setRawMessages(true); // we dont want the size header, nor encryption
					OutputMessagePool::getInstance()->send(output);
				}
			}

			break;
		}

		case 0x01:
		{
			uint32_t requestedInfo = msg.get<uint16_t>(); // only a byte is necessary, though we could add new infos here
			if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
			{
				TRACK_MESSAGE(output);
				if(Status* status = Status::getInstance())
					status->getInfo(requestedInfo, output, msg);

				OutputMessagePool::getInstance()->send(output);
			}

			break;
		}

		default:
			break;
	}

	getConnection()->close();
}

#ifdef __DEBUG_NET_DETAIL__
void ProtocolStatus::deleteProtocolTask()
{
	std::clog << "Deleting ProtocolStatus" << std::endl;
	Protocol::deleteProtocolTask();
}

#endif
std::string Status::getStatusString(bool sendPlayers) const
{
	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
	xmlNodePtr root = doc->children;

	Database* db = Database::getInstance();
	DBQuery query;

	char buffer[90];
	xmlSetProp(root, (const xmlChar*)"version", (const xmlChar*)"1.0");

	xmlNodePtr p = xmlNewNode(NULL,(const xmlChar*)"serverinfo");
	sprintf(buffer, "%u", (uint32_t)getUptime());
	xmlSetProp(p, (const xmlChar*)"uptime", (const xmlChar*)buffer);
	xmlSetProp(p, (const xmlChar*)"ip", (const xmlChar*)g_config.getString(ConfigManager::IP).c_str());
	xmlSetProp(p, (const xmlChar*)"servername", (const xmlChar*)g_config.getString(ConfigManager::SERVER_NAME).c_str());
	sprintf(buffer, "%d", (int32_t)g_config.getNumber(ConfigManager::LOGIN_PORT));
	xmlSetProp(p, (const xmlChar*)"port", (const xmlChar*)buffer);
	xmlSetProp(p, (const xmlChar*)"location", (const xmlChar*)g_config.getString(ConfigManager::LOCATION).c_str());
	xmlSetProp(p, (const xmlChar*)"url", (const xmlChar*)g_config.getString(ConfigManager::URL).c_str());
	xmlSetProp(p, (const xmlChar*)"server", (const xmlChar*)SOFTWARE_NAME);
	xmlSetProp(p, (const xmlChar*)"version", (const xmlChar*)SOFTWARE_VERSION);
	xmlSetProp(p, (const xmlChar*)"client", (const xmlChar*)SOFTWARE_PROTOCOL);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"owner");
	xmlSetProp(p, (const xmlChar*)"name", (const xmlChar*)g_config.getString(ConfigManager::OWNER_NAME).c_str());
	xmlSetProp(p, (const xmlChar*)"email", (const xmlChar*)g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"players");
	query << "SELECT COUNT(`id`) AS `count` FROM `players` WHERE `online` > 0";
	if(DBResult* result = db->storeQuery(query.str()))
	{
		sprintf(buffer, "%d", result->getDataInt("count"));
		result->free();
	}
	else
		sprintf(buffer, "%d", 0);

	query.str("");
	xmlSetProp(p, (const xmlChar*)"online", (const xmlChar*)buffer);
	sprintf(buffer, "%d", (int32_t)g_config.getNumber(ConfigManager::MAX_PLAYERS));
	xmlSetProp(p, (const xmlChar*)"max", (const xmlChar*)buffer);

	query << "SELECT MAX(`record`) AS `record` FROM `server_record` GROUP BY `world_id`";
	if(DBResult* result = db->storeQuery(query.str()))
	{
		uint32_t record = 0;
		do
			record += result->getDataInt("record");
		while(result->next());
		result->free();

		sprintf(buffer, "%d", record);
	}
	else
		sprintf(buffer, "%d", 0);

	query.str("");
	xmlSetProp(p, (const xmlChar*)"peak", (const xmlChar*)buffer);
	if(sendPlayers)
	{
		std::stringstream ss;
		query << "SELECT `name`, `vocation_id`, `promotion`, `level` FROM `players` WHERE `online` > 0";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			do
			{
				if(!ss.str().empty())
					ss << ";";

				ss << result->getDataString("name") << "," << result->getDataInt("vocation_id") << "," << result->getDataInt("level");
			}
			while(result->next());
			result->free();
		}

		query.str("");
		xmlNodeSetContent(p, (const xmlChar*)ss.str().c_str());
	}

	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"monsters");
	sprintf(buffer, "%d", 0);
	xmlSetProp(p, (const xmlChar*)"total", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"npcs");
	sprintf(buffer, "%d", 0);
	xmlSetProp(p, (const xmlChar*)"total", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"map");
	xmlSetProp(p, (const xmlChar*)"name", (const xmlChar*)g_config.getString(ConfigManager::MAP_NAME).c_str());
	xmlSetProp(p, (const xmlChar*)"author", (const xmlChar*)g_config.getString(ConfigManager::MAP_AUTHOR).c_str());

	sprintf(buffer, "%u", (uint32_t)g_config.getNumber(ConfigManager::MAP_W));
	xmlSetProp(p, (const xmlChar*)"width", (const xmlChar*)buffer);
	sprintf(buffer, "%u", (uint32_t)g_config.getNumber(ConfigManager::MAP_H));

	xmlSetProp(p, (const xmlChar*)"height", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getString(ConfigManager::MOTD_TEXT).c_str());

	xmlChar* s = NULL;
	int32_t len = 0;
	xmlDocDumpMemory(doc, (xmlChar**)&s, &len);

	std::string xml;
	if(s)
		xml = std::string((char*)s, len);

	xmlFree(s);
	xmlFreeDoc(doc);
	return xml;
}

void Status::getInfo(uint32_t requestedInfo, OutputMessage_ptr output, NetworkMessage& msg) const
{
	Database* db = Database::getInstance();
	if(requestedInfo & REQUEST_BASIC_SERVER_INFO)
	{
		output->put<char>(0x10);
		output->putString(g_config.getString(ConfigManager::SERVER_NAME).c_str());
		output->putString(g_config.getString(ConfigManager::IP).c_str());

		char buffer[10];
		sprintf(buffer, "%d", (int32_t)g_config.getNumber(ConfigManager::LOGIN_PORT));
		output->putString(buffer);
	}

	if(requestedInfo & REQUEST_SERVER_OWNER_INFO)
	{
		output->put<char>(0x11);
		output->putString(g_config.getString(ConfigManager::OWNER_NAME).c_str());
		output->putString(g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	}

	if(requestedInfo & REQUEST_MISC_SERVER_INFO)
	{
		output->put<char>(0x12);
		output->putString(g_config.getString(ConfigManager::MOTD_TEXT).c_str());
		output->putString(g_config.getString(ConfigManager::LOCATION).c_str());
		output->putString(g_config.getString(ConfigManager::URL).c_str());

		uint64_t uptime = getUptime();
		output->put<uint32_t>((uint32_t)(uptime >> 32));
		output->put<uint32_t>((uint32_t)(uptime));
	}

	if(requestedInfo & REQUEST_PLAYERS_INFO)
	{
		output->put<char>(0x20);
		DBQuery query;

		query << "SELECT COUNT(`id`) AS `count` FROM `players` WHERE `online` > 0";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			output->put<uint32_t>(result->getDataInt("count"));
			result->free();
		}
		else
			output->put<uint32_t>(0);

		output->put<uint32_t>((uint32_t)g_config.getNumber(ConfigManager::MAX_PLAYERS));
		query.str("");

		query << "SELECT MAX(`record`) AS `record` FROM `server_record` GROUP BY `world_id`";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			uint32_t record = 0;
			do
				record += result->getDataInt("record");
			while(result->next());
			result->free();

			output->put<uint32_t>(record);
		}
		else
			output->put<uint32_t>(0);
	}

	if(requestedInfo & REQUEST_SERVER_MAP_INFO)
	{
		output->put<char>(0x30);
		output->putString(g_config.getString(ConfigManager::MAP_NAME).c_str());
		output->putString(g_config.getString(ConfigManager::MAP_AUTHOR).c_str());

		output->put<uint16_t>(g_config.getNumber(ConfigManager::MAP_W));
		output->put<uint16_t>(g_config.getNumber(ConfigManager::MAP_H));
	}

	if(requestedInfo & REQUEST_EXT_PLAYERS_INFO)
	{
		output->put<char>(0x21);
		std::list<std::pair<std::string, uint32_t> > players;

		DBQuery query;
		query << "SELECT `name`, `level` FROM `players` WHERE `online` > 0";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			do
				players.push_back(std::make_pair(result->getDataString("name"), result->getDataInt("level")));
			while(result->next());
			result->free();
		}

		output->put<uint32_t>(players.size());
		for(std::list<std::pair<std::string, uint32_t> >::iterator it = players.begin(); it != players.end(); ++it)
		{
			output->putString(it->first);
			output->put<uint32_t>(it->second);
		}
	}

	if(requestedInfo & REQUEST_PLAYER_STATUS_INFO)
	{
		output->put<char>(0x22);
		const std::string name = msg.getString();

		DBQuery query;
		query << "SELECT `online` FROM `players` WHERE `name` LIKE " << db->escapeString(name) << " LIMIT 1;";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			output->put<char>(result->getDataInt("online") > 0);
			result->free();
		}
		else
			output->put<char>(0x00);
	}

	if(requestedInfo & REQUEST_SERVER_SOFTWARE_INFO)
	{
		output->put<char>(0x23);
		output->putString(SOFTWARE_NAME);
		output->putString(SOFTWARE_VERSION);
		output->putString(SOFTWARE_PROTOCOL);
	}
}
