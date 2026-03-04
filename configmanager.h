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

#ifndef __CONFIG_MANAGER__
#define __CONFIG_MANAGER__

#ifdef __LUAJIT__
#include <luajit-2.0/lua.hpp>
#elif defined(__ALT_LUA_PATH__)
extern "C"
{
	#include <lua5.1/lua.h>
	#include <lua5.1/lauxlib.h>
	#include <lua5.1/lualib.h>
}
#else
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}
#endif

class ConfigManager
{
	public:
		ConfigManager();
		virtual ~ConfigManager() {}

		enum string_config_t
		{
			DUMMY_STR = 0,
			CONFIG_FILE,
			MAP_NAME,
			SERVER_NAME,
			OWNER_NAME,
			OWNER_EMAIL,
			URL,
			LOCATION,
			IP,
			MOTD_TEXT,
			SQL_HOST,
			SQL_USER,
			SQL_PASS,
			SQL_DB,
			DEFAULT_PRIORITY,
			#ifdef MULTI_SQL_DRIVERS
			SQL_TYPE,
			#endif
			SQL_FILE,
			ENCRYPTION_TYPE,
			RSA_PRIME1,
			RSA_PRIME2,
			RSA_PUBLIC,
			RSA_MODULUS,
			RSA_PRIVATE,
			MAP_AUTHOR,
			RUNFILE,
			OUTPUT_LOG,
			CORES_USED,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum number_config_t
		{
			LOGIN_TRIES = 0,
			RETRY_TIMEOUT,
			LOGIN_TIMEOUT,
			LOGIN_PORT,
			STATUS_PORT,
			SQL_PORT,
			SQL_KEEPALIVE,
			MAX_PLAYERS,
			ENCRYPTION,
			STATUSQUERY_TIMEOUT,
			MYSQL_READ_TIMEOUT,
			MYSQL_WRITE_TIMEOUT,
			MYSQL_RECONNECTION_ATTEMPTS,
			NICE_LEVEL,
			MOTD_ID,
			MAP_W,
			MAP_H,
			SERVICE_THREADS,
			GAMEMASTER_GROUP,
			MAX_PACKETS_PER_SECOND,
			HTTP_PORT,
			IP_ACCESS_EXPIRE_SECONDS,
			LAST_NUMBER_CONFIG /* this must be the last one */
		};

		enum bool_config_t
		{
			ON_OR_OFF_CHARLIST = 0,
			FREE_PREMIUM,
			INIT_PREMIUM_UPDATE,
			BIND_ONLY_GLOBAL_ADDRESS,
			DAEMONIZE,
			TRUNCATE_LOG,
			FORCE_CLOSE_SLOW_CONNECTION,
			HTTP_ENABLED,
			LAST_BOOL_CONFIG /* this must be the last one */
		};

		bool load();
		bool reload() {return m_loaded && load();}
		void startup() {m_startup = false;}

		bool isRunning() const {return !m_startup;}
		bool isLoaded() const {return m_loaded;}

		const std::string& getString(uint32_t _what) const;
		bool getBool(uint32_t _what) const;
		int64_t getNumber(uint32_t _what) const;

		bool setString(uint32_t _what, const std::string& _value);
		bool setNumber(uint32_t _what, int64_t _value);
		bool setBool(uint32_t _what, bool _value);

	private:
		static void moveValue(lua_State* fromL, lua_State* toL);

		std::string getGlobalString(const std::string& _identifier, const std::string& _default = "");
		bool getGlobalBool(const std::string& _identifier, bool _default = false);
		int64_t getGlobalNumber(const std::string& _identifier, const int64_t _default = 0);

		bool m_loaded, m_startup;
		lua_State* L;

		std::string m_confString[LAST_STRING_CONFIG];
		bool m_confBool[LAST_BOOL_CONFIG];
		int64_t m_confNumber[LAST_NUMBER_CONFIG];
};
#endif
