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
#include <iomanip>

#include "protocollogin.h"
#include "tools.h"

#include "io.h"
#include "worlds.h"
#include "database.h"

#include "outputmessage.h"
#include "connection.h"

#include "tasks.h"
#include "configmanager.h"

extern ConfigManager g_config;

#ifdef __DEBUG_NET_DETAIL__
void ProtocolLogin::deleteProtocolTask()
{
	std::clog << "Deleting ProtocolLogin" << std::endl;
	Protocol::deleteProtocolTask();
}

#endif
void ProtocolLogin::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output)
	{
		TRACK_MESSAGE(output);
		output->put<char>(error);
		output->putString(message);
		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->close();
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	msg.skip(2); // client platform
	uint16_t version = msg.get<uint16_t>();

#ifdef CLIENT_VERSION_DATA
#ifdef CLIENT_VERSION_DAT
	uint32_t datSignature = msg.get<uint32_t>();
#else
	msg.skip(4);
#endif
#ifdef CLIENT_VERSION_SPR
	uint32_t sprSignature = msg.get<uint32_t>();
#else
	msg.skip(4);
#endif

#ifdef CLIENT_VERSION_PIC
	uint32_t picSignature = msg.get<uint32_t>();
#else
	msg.skip(4);
#endif
#else
	msg.skip(12);
#endif
	if(!RSA_decrypt(msg))
	{
		getConnection()->close();
		return;
	}

	uint32_t key[4] = {msg.get<uint32_t>(), msg.get<uint32_t>(), msg.get<uint32_t>(), msg.get<uint32_t>()};
	enableXTEAEncryption();
	setXTEAKey(key);

	std::string name = msg.getString(), password = msg.getString();
	if(name.empty())
		name = "10";

	if((version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) && version != CLIENT_VERSION_CUSTOM)
	{
		disconnectClient(0x0A, CLIENT_VERSION_STRING);
		return;
	}
#ifdef CLIENT_VERSION_DATA
#ifdef CLIENT_VERSION_SPR

	if(sprSignature < CLIENT_VERSION_SPR)
	{
		disconnectClient(0x0A, CLIENT_VERSION_DATA);
		return;
	}
#endif
#ifdef CLIENT_VERSION_DAT

	if(datSignature < CLIENT_VERSION_DAT)
	{
		disconnectClient(0x0A, CLIENT_VERSION_DATA);
		return;
	}
#endif
#ifdef CLIENT_VERSION_PIC

	if(picSignature < CLIENT_VERSION_PIC)
	{
		disconnectClient(0x0A, CLIENT_VERSION_DATA);
		return;
	}
#endif
#endif

	uint32_t clientIp = getConnection()->getIP();
	if(ConnectionManager::getInstance()->isDisabled(clientIp, protocolId))
	{
		disconnectClient(0x0A, "Too many connections attempts from your IP address, please try again later.");
		return;
	}

	Dispatcher::getInstance().addTask(createTask(boost::bind(
		&ProtocolLogin::delegate, this, name, password, clientIp, version)));
}

void ProtocolLogin::delegate(const std::string& name, const std::string& password, uint32_t clientIp, uint16_t version)
{
	IO::getInstance()->checkBanishments();
	if(IO::getInstance()->isIpBanished(clientIp))
	{
		disconnectClient(0x0A, "Your IP is banished!");
		return;
	}

	Account account;
	if(!IO::getInstance()->loadAccount(account, name) || (account.number != 10 && !encryptTest(account.salt + password, account.password)))
	{
		ConnectionManager::getInstance()->addAttempt(clientIp, protocolId, false);
		disconnectClient(0x0A, "Invalid account name or password.");
		return;
	}

	Ban ban;
	ban.value = account.number;

	ban.type = BAN_ACCOUNT;
	if(IO::getInstance()->getBanishment(ban))
	{
		bool deletion = ban.expires < 0;
		std::string name_ = "Automatic ";
		if(!ban.adminId)
			name_ += (deletion ? "deletion" : "banishment");
		else
			IO::getInstance()->getNameByGuid(ban.adminId, name_);

		std::ostringstream ss;
		ss << "Your account has been " << (deletion ? "deleted" : "banished") << " at:\n" << formatDateEx(ban.added, "%d %b %Y")
			<< " by: " << name_ << ".\nThe comment given was:\n" << ban.comment << ".\nYour " << (deletion ?
			"account won't be undeleted" : "banishment will be lifted at:\n") << (deletion ? "" : formatDateEx(ban.expires)) << ".";

		disconnectClient(0x0A, ss.str().c_str());
		return;
	}

	// No version filtering — 9.63 only
	Characters& charList = account.charList;

	// remove premium days
	IO::getInstance()->updatePremium(account);
	if(account.number != 10 && !charList.size())
	{
		disconnectClient(0x0A, std::string("This account does not contain any character for this client yet.\nCreate a new character on the "
			+ g_config.getString(ConfigManager::SERVER_NAME) + " website at " + g_config.getString(ConfigManager::URL) + ".").c_str());
		return;
	}

	ConnectionManager::getInstance()->addAttempt(clientIp, protocolId, true);

	if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
	{
		TRACK_MESSAGE(output);
		output->put<char>(0x14);

		char motd[1300];
		sprintf(motd, "%d\n%s", (int32_t)g_config.getNumber(ConfigManager::MOTD_ID), g_config.getString(ConfigManager::MOTD_TEXT).c_str());
		output->putString(motd);

		//Add char list
		output->put<char>(0x64);
		if(account.number == 10)
		{
			Database* db = Database::local();
			std::ostringstream query;

			query << "SELECT `name`, `world_id`, `level`, `broadcasting` FROM `players` WHERE `broadcasting` > 0";
			if(DBResult* result = db->storeQuery(query.str()))
			{
				std::map<std::string, std::pair<bool, std::pair<World*, int32_t> > > tmp;
				do
				{
					uint8_t t = result->getDataInt("broadcasting");
					if((t & 1) == 1)
					{
						World* srv = Worlds::getInstance()->getWorldById(result->getDataInt("world_id"));
						if(srv)
							tmp[result->getDataString("name")] = std::make_pair(((t & 2) == 2), std::make_pair(srv, result->getDataInt("level")));
					}
				}
				while(result->next());
				result->free();

				output->put<char>((uint8_t)tmp.size());
				for(std::map<std::string, std::pair<bool, std::pair<World*, int32_t> > >::iterator it = tmp.begin(); it != tmp.end(); ++it)
				{
					std::ostringstream s;
					s << it->second.second.second;

					output->putString(it->first);
					if(it->second.first)
						s << "*";

					output->putString(s.str());
					output->put<uint32_t>(it->second.second.first->getAddress());

					IntegerVec games = it->second.second.first->getPorts();
					output->put<uint16_t>(games[random_range(0, games.size() - 1)]);
				}
			}
			else
				output->put<char>(0);
		}
		else
		{
			output->put<char>((uint8_t)charList.size());
			for(Characters::iterator it = charList.begin(); it != charList.end(); ++it)
			{
				output->putString(it->second.name);
				if(!g_config.getBool(ConfigManager::ON_OR_OFF_CHARLIST) || it->second.status < 0)
					output->putString(it->second.server->getName());
				else if(it->second.status)
					output->putString("Online");
				else
					output->putString("Offline");

				output->put<uint32_t>(it->second.server->getAddress());
				IntegerVec games = it->second.server->getPorts();
				output->put<uint16_t>(games[random_range(0, games.size() - 1)]);
			}
		}

		//Add premium days
		if(g_config.getBool(ConfigManager::FREE_PREMIUM))
			output->put<uint16_t>(GRATIS_PREMIUM);
		else
			output->put<uint16_t>(account.premiumDays);

		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->close();
}
