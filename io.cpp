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
#include <iostream>
#include <iomanip>

#include "io.h"
#include "database.h"

#include "gameservers.h"

#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

bool IO::loadAccount(Account& account, const std::string& name)
{
	Database* db = Database::getInstance();
	std::ostringstream query;

	query << "SELECT `id`, `password`, `salt`, `premdays`, `lastday`, `key`, `warnings` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapeString(name) << " LIMIT 1";
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	account.number = result->getDataInt("id");
	account.name = name;
	account.password = result->getDataString("password");
	account.salt = result->getDataString("salt");
	account.premiumDays = std::max((int32_t)0, std::min((int32_t)GRATIS_PREMIUM, result->getDataInt("premdays")));
	account.lastDay = result->getDataInt("lastday");
	account.recoveryKey = result->getDataString("key");
	account.warnings = result->getDataInt("warnings");

	result->free();
	query.str("");

	query << "SELECT `name`, `world_id`, `group_id`, `online` FROM `players` WHERE `account_id` = " << account.number << " AND `deleted` = 0";
	if(!(result = db->storeQuery(query.str())))
		return true;

	do
	{
		std::string name = result->getDataString("name");
		if(result->getDataInt("group_id") >= g_config.getNumber(ConfigManager::GAMEMASTER_GROUP))
		{
			uint16_t hax = 0;
			for(GameServersMap::const_iterator it = GameServers::getInstance()->getFirstServer(); it != GameServers::getInstance()->getLastServer(); ++it, ++hax)
				account.charList[name + asString(hax)] = Character(name, it->second, -1);
		}
		else if(GameServer* srv = GameServers::getInstance()->getServerById(result->getDataInt("world_id")))
			account.charList[name] = Character(name, srv, result->getDataInt("online"));
		else
			std::clog << "[Warning - IO::loadAccount] Invalid server for player '" << name << "'." << std::endl;
	}
	while(result->next());
	result->free();
	return true;
}

Account IO::loadAccount(uint32_t accountId)
{
	Database* db = Database::getInstance();
	std::ostringstream query;

	query << "SELECT `name`, `password`, `salt`, `premdays`, `lastday`, `key`, `warnings` FROM `accounts` WHERE `id` = " << accountId << " LIMIT 1";
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return Account();

	Account account;
	account.number = accountId;
	account.name = result->getDataString("name");
	account.password = result->getDataString("password");
	account.salt = result->getDataString("salt");
	account.premiumDays = std::max((int32_t)0, std::min((int32_t)GRATIS_PREMIUM, result->getDataInt("premdays")));
	account.lastDay = result->getDataInt("lastday");
	account.recoveryKey = result->getDataString("key");
	account.warnings = result->getDataInt("warnings");

	result->free();
	return account;
}

bool IO::saveAccount(Account account)
{
	Database* db = Database::getInstance();
	std::ostringstream query;
	query << "UPDATE `accounts` SET `premdays` = " << account.premiumDays << ", `warnings` = " << account.warnings << ", `lastday` = " << account.lastDay << " WHERE `id` = " << account.number << db->getUpdateLimiter();
	return db->query(query.str());
}

bool IO::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
	{
		name = it->second;
		return true;
	}

	Database* db = Database::getInstance();
	DBResult* result;
	std::ostringstream query;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0 LIMIT 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	result->free();

	nameCacheMap[guid] = name;
	return true;
}

void IO::updatePremium(Account& account)
{
	uint64_t now = time(NULL);
	if(account.lastDay > now || (now - account.lastDay) < 86400)
		return;

	bool save = false;
	if(account.premiumDays > 0 && account.premiumDays < (uint16_t)GRATIS_PREMIUM)
	{
		if(account.lastDay == 0)
		{
			account.lastDay = now;
			save = true;
		}
		else
		{
			uint32_t days = (now - account.lastDay) / 86400;
			if(days > 0)
			{
				save = true;
				if(account.premiumDays >= days)
				{
					account.premiumDays -= days;
					account.lastDay = now - ((now - account.lastDay) % 86400);
				}
				else
				{
					account.premiumDays = 0;
					account.lastDay = 0;
				}
			}
		}
	}
	else if(account.lastDay != 0)
	{
		account.lastDay = 0;
		save = true;
	}

	if(save && !saveAccount(account))
		std::clog << "> ERROR: Failed to save account: " << account.name << "!" << std::endl;
}

bool IO::updatePremium()
{
	Database* db = Database::getInstance();
	std::ostringstream query;

	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	DBResult* result;
	query << "SELECT `id` FROM `accounts` WHERE `lastday` <= " << time(NULL) - 86400;
	if(!(result = db->storeQuery(query.str())))
		return false;

	Account account;
	do
		updatePremium((account = loadAccount(result->getDataInt("id"))));
	while(result->next());
	result->free();

	query.str("");
	return trans.commit();
}

bool IO::isIpBanished(uint32_t ip, uint32_t mask/* = 0xFFFFFFFF*/) const
{
	if(!ip)
		return false;

	Database* db = Database::getInstance();
	DBResult* result;

	std::ostringstream query;
	query << "SELECT `id`, `value`, `param`, `expires` FROM `bans` WHERE `type` = " << BAN_IP << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	do
	{
		uint32_t value = result->getDataInt("value"), param = result->getDataInt("param");
		if((ip & mask & param) == (value & param & mask))
		{
			result->free();
			return true;
		}
	}
	while(result->next());
	result->free();
	return false;
}

bool IO::checkBanishments() const
{
	Database* db = Database::getInstance();
	std::ostringstream query;

	query << "UPDATE `bans` SET `active` = 0 WHERE `active` = 1 AND `expires` > 0 AND `expires` <= " << time(NULL);
	return db->query(query.str());
}

bool IO::getBanishment(Ban& ban) const
{
	Database* db = Database::getInstance();
	std::ostringstream query;

	query << "SELECT * FROM `bans` WHERE `value` = " << ban.value;
	if(ban.param)
		query << " AND `param` = " << ban.param;

	if(ban.type != BAN_NONE)
		query << " AND `type` = " << ban.type;

	query << " AND `active` = 1 LIMIT 1";
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	ban.id = result->getDataInt("id");
	ban.type = (Ban_t)result->getDataInt("type");
	ban.value = result->getDataInt("value");
	ban.param = result->getDataInt("param");
	ban.expires = result->getDataLong("expires");
	ban.added = result->getDataLong("added");
	ban.adminId = result->getDataInt("admin_id");
	ban.comment = result->getDataString("comment");

	result->free();
	return true;
}
