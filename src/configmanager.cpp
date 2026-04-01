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
#include <fstream>
#include <sstream>

#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/json.hpp>

#include "configmanager.h"

ConfigManager::ConfigManager()
{
	m_loaded = false;
	m_startup = true;

	m_confNumber[ENCRYPTION] = ENCRYPTION_SHA1;
	m_confString[CONFIG_DIR] = ".";

	m_confString[RUNFILE] = "";
	m_confString[OUTPUT_LOG] = "";
	m_confBool[DAEMONIZE] = false;
}

bool ConfigManager::load(const std::string& configDir)
{
	std::string configPath = configDir + "/config.json";

	std::ifstream file(configPath);
	if(!file.is_open())
	{
		std::clog << "[Error - ConfigManager::load] Cannot open " << configPath << std::endl;
		return false;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	file.close();

	boost::system::error_code ec;
	boost::json::value jv = boost::json::parse(ss.str(), ec);
	if(ec)
	{
		std::clog << "[Error - ConfigManager::load] JSON parse error in " << configPath << ": " << ec.message() << std::endl;
		return false;
	}

	if(!jv.is_object())
	{
		std::clog << "[Error - ConfigManager::load] Root of " << configPath << " is not a JSON object" << std::endl;
		return false;
	}

	const boost::json::object& root = jv.as_object();

	m_confString[CONFIG_DIR] = configDir;

	// database section
	if(auto it = root.find("database"); it != root.end() && it->value().is_object())
	{
		const auto& db = it->value().as_object();
		if(auto v = db.find("dsn"); v != db.end() && v->value().is_string())
			m_confString[DATABASE_DSN] = std::string(v->value().as_string());
		else
			m_confString[DATABASE_DSN] = "mysql://root:@localhost:3306/theforgottenserver";
	}
	else
	{
		m_confString[DATABASE_DSN] = "mysql://root:@localhost:3306/theforgottenserver";
	}

	// auth section — only encryptionType used by legacy
	if(auto it = root.find("auth"); it != root.end() && it->value().is_object())
	{
		const auto& auth = it->value().as_object();
		if(auto v = auth.find("encryptionType"); v != auth.end() && v->value().is_string())
			m_confString[ENCRYPTION_TYPE] = std::string(v->value().as_string());
		else
			m_confString[ENCRYPTION_TYPE] = "sha1";
	}
	else
	{
		m_confString[ENCRYPTION_TYPE] = "sha1";
	}

	// discord section
	if(auto it = root.find("discord"); it != root.end() && it->value().is_object())
	{
		const auto& disc = it->value().as_object();
		if(auto v = disc.find("botToken"); v != disc.end() && v->value().is_string())
			m_confString[DISCORD_BOT_TOKEN] = std::string(v->value().as_string());
		if(auto v = disc.find("motdChannel"); v != disc.end() && v->value().is_string())
			m_confString[DISCORD_MOTD_CHANNEL] = std::string(v->value().as_string());
	}

	// legacy section
	if(auto it = root.find("legacy"); it != root.end() && it->value().is_object())
	{
		const auto& leg = it->value().as_object();

		auto getStr = [&](const char* key, const std::string& def) -> std::string {
			if(auto v = leg.find(key); v != leg.end() && v->value().is_string())
				return std::string(v->value().as_string());
			return def;
		};
		auto getNum = [&](const char* key, int64_t def) -> int64_t {
			if(auto v = leg.find(key); v != leg.end() && v->value().is_int64())
				return v->value().as_int64();
			return def;
		};
		auto getBool_ = [&](const char* key, bool def) -> bool {
			if(auto v = leg.find(key); v != leg.end() && v->value().is_bool())
				return v->value().as_bool();
			return def;
		};

		m_confNumber[LOGIN_TRIES]              = getNum("loginTries", 3);
		m_confNumber[RETRY_TIMEOUT]            = getNum("retryTimeout", 30000);
		m_confNumber[LOGIN_TIMEOUT]            = getNum("loginTimeout", 5000);
		m_confNumber[MAX_PLAYERS]              = getNum("maxPlayers", 1000);
		m_confNumber[NICE_LEVEL]               = getNum("niceLevel", 5);
		m_confNumber[MOTD_ID]                  = 1;
		m_confNumber[SERVICE_THREADS]          = getNum("serviceThreads", 1);
		m_confNumber[GAMEMASTER_GROUP]         = getNum("gamemasterGroup", 4);
		m_confNumber[MAX_PACKETS_PER_SECOND]   = getNum("maxPacketsPerSecond", 25);

		m_confString[SERVER_NAME]  = getStr("serverName", "Forgotten");
		m_confString[OWNER_NAME]   = getStr("ownerName", "");
		m_confString[OWNER_EMAIL]  = getStr("ownerEmail", "@otland.net");
		m_confString[URL]          = getStr("url", "http://otland.net/");
		m_confString[LOCATION]     = getStr("location", "Europe");
		m_confString[MOTD_TEXT]   = "";

		m_confString[LISTEN_ADDR] = getStr("listenAddr", "0.0.0.0:7171");

		if(m_confString[RUNFILE].empty())
			m_confString[RUNFILE] = getStr("runFile", "tfls.pid");
		if(m_confString[OUTPUT_LOG].empty())
			m_confString[OUTPUT_LOG] = getStr("outputLog", "");

		m_confBool[FREE_PREMIUM]              = getBool_("freePremium", false);
		m_confBool[INIT_PREMIUM_UPDATE]       = getBool_("updatePremiumStateAtStartup", true);
		m_confBool[ON_OR_OFF_CHARLIST]        = getBool_("displayOnOrOffAtCharlist", true);
		m_confBool[DAEMONIZE]                 = getBool_("daemonize", false);
		m_confBool[TRUNCATE_LOG]              = getBool_("truncateLogsOnStartup", true);
		m_confBool[FORCE_CLOSE_SLOW_CONNECTION] = getBool_("forceCloseSlowConnection", false);

		// RSA
		if(auto rsaIt = leg.find("rsa"); rsaIt != leg.end() && rsaIt->value().is_object())
		{
			const auto& rsa = rsaIt->value().as_object();
			auto rsaStr = [&](const char* key, const std::string& def) -> std::string {
				if(auto v = rsa.find(key); v != rsa.end() && v->value().is_string())
					return std::string(v->value().as_string());
				return def;
			};
			m_confString[RSA_PRIME1]  = rsaStr("prime1", "14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
			m_confString[RSA_PRIME2]  = rsaStr("prime2", "7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
			m_confString[RSA_PUBLIC]  = rsaStr("public", "65537");
			m_confString[RSA_MODULUS] = rsaStr("modulus", "109120132967399429278860960508995541528237502902798129123468757937266291492576446330739696001110603907230888610072655818825358503429057592827629436413108566029093628212635953836686562675849720620786279431090218017681061521755056710823876476444260558147179707119674283982419152118103759076030616683978566631413");
			m_confString[RSA_PRIVATE] = rsaStr("private", "46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
		}
		else
		{
			m_confString[RSA_PRIME1]  = "14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113";
			m_confString[RSA_PRIME2]  = "7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101";
			m_confString[RSA_PUBLIC]  = "65537";
			m_confString[RSA_MODULUS] = "109120132967399429278860960508995541528237502902798129123468757937266291492576446330739696001110603907230888610072655818825358503429057592827629436413108566029093628212635953836686562675849720620786279431090218017681061521755056710823876476444260558147179707119674283982419152118103759076030616683978566631413";
			m_confString[RSA_PRIVATE] = "46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073";
		}

		// coresUsed — JSON integer array
		m_coresUsed.clear();
		if(auto v = leg.find("coresUsed"); v != leg.end() && v->value().is_array())
		{
			for(const auto& elem : v->value().as_array())
			{
				if(elem.is_int64())
					m_coresUsed.push_back(static_cast<int32_t>(elem.as_int64()));
			}
		}
		if(m_coresUsed.empty())
			m_coresUsed.push_back(-1);
	}
	else
	{
		// No legacy section — use defaults
		m_confNumber[LOGIN_TRIES] = 3;
		m_confNumber[RETRY_TIMEOUT] = 30000;
		m_confNumber[LOGIN_TIMEOUT] = 5000;
		m_confNumber[MAX_PLAYERS] = 1000;
		m_confNumber[NICE_LEVEL] = 5;
		m_confNumber[MOTD_ID] = 1;
		m_confNumber[SERVICE_THREADS] = 1;
		m_confNumber[GAMEMASTER_GROUP] = 4;
		m_confNumber[MAX_PACKETS_PER_SECOND] = 25;
		m_confString[SERVER_NAME] = "Forgotten";
		m_confString[MOTD_TEXT] = "";
		m_confBool[ON_OR_OFF_CHARLIST] = true;
		m_confBool[TRUNCATE_LOG] = true;
		m_coresUsed.clear();
		m_coresUsed.push_back(-1);
	}

	m_loaded = true;
	reloadMotd();
	return true;
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if((m_loaded && _what < LAST_STRING_CONFIG) || _what <= CONFIG_DIR)
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

void ConfigManager::reloadMotd()
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = net::ssl;
	using tcp = net::ip::tcp;

	const std::string& botToken = m_confString[DISCORD_BOT_TOKEN];
	const std::string& channelId = m_confString[DISCORD_MOTD_CHANNEL];

	if(botToken.empty() || channelId.empty())
	{
		if(m_confString[MOTD_TEXT].empty())
			m_confString[MOTD_TEXT] = "Welcome to Wypas!";
		return;
	}

	try
	{
		net::io_context ioc;
		ssl::context ctx(ssl::context::tlsv12_client);
		ctx.set_default_verify_paths();

		tcp::resolver resolver(ioc);
		beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

		SSL_set_tlsext_host_name(stream.native_handle(), "discord.com");

		auto const results = resolver.resolve("discord.com", "443");
		beast::get_lowest_layer(stream).connect(results);
		stream.handshake(ssl::stream_base::client);

		std::string target = "/api/v10/channels/" + channelId + "/messages?limit=1";
		http::request<http::string_body> req{http::verb::get, target, 11};
		req.set(http::field::host, "discord.com");
		req.set(http::field::user_agent, "WypasBot/1.0");
		req.set(http::field::authorization, "Bot " + botToken);

		http::write(stream, req);

		beast::flat_buffer buffer;
		http::response<http::string_body> res;
		http::read(stream, buffer, res);

		beast::error_code ec;
		stream.shutdown(ec);

		if(res.result() != http::status::ok)
		{
			std::clog << "[Warning - ConfigManager::reloadMotd] Discord API returned HTTP " << res.result_int() << std::endl;
			if(m_confString[MOTD_TEXT].empty())
				m_confString[MOTD_TEXT] = "Welcome to Wypas!";
			return;
		}

		auto jv = boost::json::parse(res.body());
		if(!jv.is_array() || jv.as_array().empty())
		{
			std::clog << "[Warning - ConfigManager::reloadMotd] Discord API returned empty or invalid response" << std::endl;
			if(m_confString[MOTD_TEXT].empty())
				m_confString[MOTD_TEXT] = "Welcome to Wypas!";
			return;
		}

		auto& msg = jv.as_array()[0].as_object();
		std::string content(msg["content"].as_string());

		if(!content.empty())
		{
			m_confString[MOTD_TEXT] = content;
			m_confNumber[MOTD_ID]++;
		}
	}
	catch(const std::exception& e)
	{
		std::clog << "[Warning - ConfigManager::reloadMotd] Discord fetch failed: " << e.what() << std::endl;
		if(m_confString[MOTD_TEXT].empty())
			m_confString[MOTD_TEXT] = "Welcome to Wypas!";
	}
}
