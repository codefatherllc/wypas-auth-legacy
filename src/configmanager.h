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

#include <boost/json.hpp>
#include "tools.h"

class ConfigManager
{
	public:
		ConfigManager();
		virtual ~ConfigManager() {}

		enum string_config_t
		{
			DUMMY_STR = 0,
			CONFIG_DIR,
			DATABASE_DSN,
			SERVER_NAME,
			OWNER_NAME,
			OWNER_EMAIL,
			URL,
			LOCATION,
			MOTD_TEXT,
			ENCRYPTION_TYPE,
			RSA_PRIME1,
			RSA_PRIME2,
			RSA_PUBLIC,
			RSA_MODULUS,
			RSA_PRIVATE,
			LISTEN_ADDR,
			RUNFILE,
			OUTPUT_LOG,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum number_config_t
		{
			LOGIN_TRIES = 0,
			RETRY_TIMEOUT,
			LOGIN_TIMEOUT,
			MAX_PLAYERS,
			ENCRYPTION,
			NICE_LEVEL,
			MOTD_ID,
			SERVICE_THREADS,
			GAMEMASTER_GROUP,
			MAX_PACKETS_PER_SECOND,
			LAST_NUMBER_CONFIG /* this must be the last one */
		};

		enum bool_config_t
		{
			ON_OR_OFF_CHARLIST = 0,
			FREE_PREMIUM,
			INIT_PREMIUM_UPDATE,
			DAEMONIZE,
			TRUNCATE_LOG,
			FORCE_CLOSE_SLOW_CONNECTION,
			LAST_BOOL_CONFIG /* this must be the last one */
		};

		bool load(const std::string& configDir);
		void startup() {m_startup = false;}

		bool isRunning() const {return !m_startup;}
		bool isLoaded() const {return m_loaded;}

		const std::string& getString(uint32_t _what) const;
		bool getBool(uint32_t _what) const;
		int64_t getNumber(uint32_t _what) const;

		bool setString(uint32_t _what, const std::string& _value);
		bool setNumber(uint32_t _what, int64_t _value);
		bool setBool(uint32_t _what, bool _value);

		IntegerVec getCoresUsed() const { return m_coresUsed; }

	private:
		bool m_loaded, m_startup;

		std::string m_confString[LAST_STRING_CONFIG];
		bool m_confBool[LAST_BOOL_CONFIG];
		int64_t m_confNumber[LAST_NUMBER_CONFIG];
		IntegerVec m_coresUsed;
};
#endif
