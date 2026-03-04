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

#ifndef __IO__
#define __IO__
#include "otsystem.h"
#include "tools.h"

enum Ban_t
{
	BAN_NONE = 0,
	BAN_IP = 1,
	BAN_PLAYER = 2,
	BAN_ACCOUNT = 3,
	BAN_NOTATION = 4
};

class GameServer;
struct Character
{
	Character(): server(NULL), status(0) {}
	Character(const std::string& _name, GameServer* _server, int8_t _status):
		name(_name), server(_server), status(_status) {}

	std::string name;
	GameServer* server;
	int8_t status;
};

typedef std::map<std::string, Character> Characters;
class Account
{
	public:
		Account() {premiumDays = warnings = number = lastDay = 0;}
		virtual ~Account() {}

		uint16_t premiumDays, warnings;
		uint32_t number, lastDay;
		std::string name, password, recoveryKey, salt;
		Characters charList;
};

struct Ban
{
	Ban_t type;
	uint32_t id, value, param, added, adminId;
	int32_t expires;
	std::string comment;

	Ban()
	{
		type = BAN_NONE;
		id = value = param = added = adminId = expires = 0;
	}
};

class IO
{
	public:
		virtual ~IO() {}
		static IO* getInstance()
		{
			static IO instance;
			return &instance;
		}

		bool loadAccount(Account& account, const std::string& name);
		bool getNameByGuid(uint32_t guid, std::string& name);

		void updatePremium(Account& account);
		bool updatePremium();

		bool isIpBanished(uint32_t ip, uint32_t mask = 0xFFFFFFFF) const;
		bool checkBanishments() const;
		bool getBanishment(Ban& ban) const;

		bool createIpAccessTable();
		bool grantIpAccess(const std::string& ip, const std::string& sessionToken, int32_t expireSeconds);
		bool cleanupExpiredIpAccess();
		void cleanupExpiredIpAccessRecurring();
		static std::string generateSessionToken();

	protected:
		IO() {}

		Account loadAccount(uint32_t accountId);
		bool saveAccount(Account account);

		typedef std::map<uint32_t, std::string> NameCacheMap;
		NameCacheMap nameCacheMap;
};
#endif
