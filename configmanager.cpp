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

#include "configmanager.h"
#include "tools.h"

ConfigManager::ConfigManager()
{
	L = NULL;
	m_loaded = false;
	m_startup = true;

	m_confNumber[ENCRYPTION] = ENCRYPTION_SHA256;
	m_confString[CONFIG_FILE] = getFilePath(FILE_TYPE_CONFIG, "config.lua");

	m_confNumber[LOGIN_PORT] = m_confNumber[STATUS_PORT] = 0;
	m_confString[IP] = m_confString[RUNFILE] = m_confString[OUTPUT_LOG] = "";
	m_confBool[DAEMONIZE] = false;
}

bool ConfigManager::load()
{
	if(L)
		lua_close(L);

	L = luaL_newstate();
	if(!L)
		return false;

	luaL_openlibs(L);
	if(luaL_dofile(L, m_confString[CONFIG_FILE].c_str()))
	{
		lua_close(L);
		L = NULL;
		return false;
	}

	if(!m_loaded) //info that must be loaded one time- unless we reset the modules involved
	{
		if(m_confString[IP] == "")
			m_confString[IP] = getGlobalString("ip", "127.0.0.1");

		if(m_confNumber[LOGIN_PORT] == 0)
			m_confNumber[LOGIN_PORT] = getGlobalNumber("loginPort", 7171);

		if(m_confNumber[STATUS_PORT] == 0)
			m_confNumber[STATUS_PORT] = getGlobalNumber("statusPort", 7171);

		if(m_confString[RUNFILE] == "")
			m_confString[RUNFILE] = getGlobalString("runFile", "");

		if(m_confString[OUTPUT_LOG] == "")
			m_confString[OUTPUT_LOG] = getGlobalString("outputLog", "");

		#ifdef MULTI_SQL_DRIVERS
		m_confString[SQL_TYPE] = getGlobalString("sqlType", "sqlite");
		#endif
		m_confString[SQL_HOST] = getGlobalString("sqlHost", "localhost");
		m_confNumber[SQL_PORT] = getGlobalNumber("sqlPort", 3306);
		m_confString[SQL_DB] = getGlobalString("sqlDatabase", "theforgottenserver");
		m_confString[SQL_USER] = getGlobalString("sqlUser", "root");
		m_confString[SQL_PASS] = getGlobalString("sqlPass", "");
		m_confString[SQL_FILE] = getGlobalString("sqlFile", "forgottenserver.s3db");
		m_confNumber[SQL_KEEPALIVE] = getGlobalNumber("sqlKeepAlive", 0);
		m_confNumber[MYSQL_READ_TIMEOUT] = getGlobalNumber("mysqlReadTimeout", 10);
		m_confNumber[MYSQL_WRITE_TIMEOUT] = getGlobalNumber("mysqlWriteTimeout", 10);
		m_confNumber[MYSQL_RECONNECTION_ATTEMPTS] = getGlobalNumber("mysqlReconnectionAttempts", 3);
		m_confBool[BIND_ONLY_GLOBAL_ADDRESS] = getGlobalBool("bindOnlyGlobalAddress", false);
		m_confNumber[SERVICE_THREADS] = getGlobalNumber("serviceThreads", 1);
		m_confBool[TRUNCATE_LOG] = getGlobalBool("truncateLogOnStartup", true);
		m_confString[DEFAULT_PRIORITY] = getGlobalString("defaultPriority", "high");
		m_confString[ENCRYPTION_TYPE] = getGlobalString("encryptionType", "sha256");
		m_confString[RSA_PRIME1] = getGlobalString("rsaPrime1", "14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
		m_confString[RSA_PRIME2] = getGlobalString("rsaPrime2", "7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
		m_confString[RSA_PUBLIC] = getGlobalString("rsaPublic", "65537");
		m_confString[RSA_MODULUS] = getGlobalString("rsaModulus", "109120132967399429278860960508995541528237502902798129123468757937266291492576446330739696001110603907230888610072655818825358503429057592827629436413108566029093628212635953836686562675849720620786279431090218017681061521755056710823876476444260558147179707119674283982419152118103759076030616683978566631413");
		m_confString[RSA_PRIVATE] = getGlobalString("rsaPrivate", "46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	}

	m_confString[MAP_NAME] = getGlobalString("mapName", "forgotten.otbm");
	m_confString[MAP_AUTHOR]			= getGlobalString("mapAuthor", "Unknown");
	m_confNumber[LOGIN_TRIES]			= getGlobalNumber("loginTries", 3);
	m_confNumber[RETRY_TIMEOUT]			= getGlobalNumber("retryTimeout", 30000);
	m_confNumber[LOGIN_TIMEOUT]			= getGlobalNumber("loginTimeout", 5000);
	m_confNumber[MAX_PLAYERS]			= getGlobalNumber("maxPlayers", 1000);
	m_confString[SERVER_NAME]			= getGlobalString("serverName");
	m_confString[OWNER_NAME]			= getGlobalString("ownerName");
	m_confString[OWNER_EMAIL]			= getGlobalString("ownerEmail");
	m_confString[URL]				= getGlobalString("url");
	m_confString[LOCATION]				= getGlobalString("location");
	m_confString[MOTD_TEXT]				= getGlobalString("motdText");
	m_confString[MOTD_ID]				= getGlobalString("motdId");
	m_confBool[FREE_PREMIUM]			= getGlobalBool("freePremium", false);
	m_confNumber[STATUSQUERY_TIMEOUT]		= getGlobalNumber("statusTimeout", 300000);
	m_confBool[INIT_PREMIUM_UPDATE]			= getGlobalBool("updatePremiumStateAtStartup", true);
	m_confString[CORES_USED]			= getGlobalString("coresUsed", "-1");
	m_confNumber[NICE_LEVEL]			= getGlobalNumber("niceLevel", 5);
	m_confBool[DAEMONIZE]				= getGlobalBool("daemonize", false);
	m_confNumber[MAP_W]			= getGlobalNumber("mapWidth", 0);
	m_confNumber[MAP_H]			= getGlobalNumber("mapHeight", 0);
	m_confBool[ON_OR_OFF_CHARLIST]			= getGlobalBool("displayOnOrOffAtCharlist", true);
	m_confNumber[GAMEMASTER_GROUP]		= getGlobalNumber("gamemasterGroup", 4);
	m_confNumber[MAX_PACKETS_PER_SECOND]		= getGlobalNumber("maxPacketsPerSecond", 25);
	m_confBool[FORCE_CLOSE_SLOW_CONNECTION]		= getGlobalNumber("forceSlowConnectionsToDisconnect", false);
	m_loaded = true;
	return true;
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if((m_loaded && _what < LAST_STRING_CONFIG) || _what <= CONFIG_FILE)
		return m_confString[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getString] " << _what << std::endl;

	return m_confString[DUMMY_STR];
}

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_loaded && _what < LAST_BOOL_CONFIG)
		return m_confBool[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getBool] " << _what << std::endl;

	return false;
}

int64_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_loaded && _what < LAST_NUMBER_CONFIG)
		return m_confNumber[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getNumber] " << _what << std::endl;

	return 0;
}

bool ConfigManager::setString(uint32_t _what, const std::string& _value)
{
	if(_what < LAST_STRING_CONFIG)
	{
		m_confString[_what] = _value;
		return true;
	}

	std::clog << "[Warning - ConfigManager::setString] " << _what << std::endl;
	return false;
}

bool ConfigManager::setNumber(uint32_t _what, int64_t _value)
{
	if(_what < LAST_NUMBER_CONFIG)
	{
		m_confNumber[_what] = _value;
		return true;
	}

	std::clog << "[Warning - ConfigManager::setNumber] " << _what << std::endl;
	return false;
}

bool ConfigManager::setBool(uint32_t _what, bool _value)
{
	if(_what < LAST_BOOL_CONFIG)
	{
		m_confBool[_what] = _value;
		return true;
	}

	std::clog << "[Warning - ConfigManager::setBool] " << _what << std::endl;
	return false;
}

std::string ConfigManager::getGlobalString(const std::string& _identifier, const std::string& _default/* = ""*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isstring(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	int32_t len = (int32_t)lua_strlen(L, -1);
	std::string ret(lua_tostring(L, -1), len);

	lua_pop(L, 1);
	return ret;
}

bool ConfigManager::getGlobalBool(const std::string& _identifier, bool _default/* = false*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isboolean(L, -1))
	{
		lua_pop(L, 1);
		return booleanString(ConfigManager::getGlobalString(_identifier, _default ? "yes" : "no"));
	}

	bool val = (lua_toboolean(L, -1) != 0);
	lua_pop(L, 1);
	return val;
}

int64_t ConfigManager::getGlobalNumber(const std::string& _identifier, const int64_t _default/* = 0*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	double val = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return (int64_t)val;
}
