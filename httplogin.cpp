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
#include "httplogin.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "io.h"
#include "gameservers.h"
#include "database.h"
#include "tools.h"
#include "configmanager.h"

extern ConfigManager g_config;

static std::map<std::string, std::pair<int, time_t>> s_loginAttempts;

// HttpSession

HttpSession::HttpSession(boost::shared_ptr<tcp::socket> socket)
	: m_socket(socket)
{
}

void HttpSession::start()
{
	doRead();
}

void HttpSession::doRead()
{
	http::async_read(*m_socket, m_buffer, m_request,
		boost::bind(&HttpSession::onRead, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void HttpSession::onRead(const boost::system::error_code& ec, std::size_t)
{
	if(ec == http::error::end_of_stream)
	{
		boost::system::error_code shutdownEc;
		m_socket->shutdown(tcp::socket::shutdown_send, shutdownEc);
		return;
	}

	if(ec)
	{
		std::clog << "[Error - HttpSession::onRead] " << ec.message() << std::endl;
		return;
	}

	handleRequest();
}

void HttpSession::addCorsHeaders(http::response<http::string_body>& res)
{
	res.set("Access-Control-Allow-Origin", "*");
	res.set("Access-Control-Allow-Methods", "POST, OPTIONS");
	res.set("Access-Control-Allow-Headers", "Content-Type");
	res.set("Access-Control-Max-Age", "86400");
}

void HttpSession::handleRequest()
{
	if(m_request.method() == http::verb::options)
	{
		handleOptions();
		return;
	}

	if(m_request.method() == http::verb::post && m_request.target() == "/login")
	{
		handleLogin();
		return;
	}

	boost::shared_ptr<http::response<http::string_body> > res =
		boost::make_shared<http::response<http::string_body> >(http::status::not_found, m_request.version());
	res->set(http::field::content_type, "application/json");
	addCorsHeaders(*res);
	res->body() = "{\"error\":\"Not found\"}";
	res->prepare_payload();
	sendResponse(res);
}

void HttpSession::handleOptions()
{
	boost::shared_ptr<http::response<http::string_body> > res =
		boost::make_shared<http::response<http::string_body> >(http::status::no_content, m_request.version());
	addCorsHeaders(*res);
	res->prepare_payload();
	sendResponse(res);
}

void HttpSession::handleLogin()
{
	boost::shared_ptr<http::response<http::string_body> > res =
		boost::make_shared<http::response<http::string_body> >(http::status::ok, m_request.version());
	res->set(http::field::content_type, "application/json");
	addCorsHeaders(*res);

	// Parse JSON request body using boost::property_tree
	boost::property_tree::ptree reqTree;
	try
	{
		std::istringstream iss(m_request.body());
		boost::property_tree::read_json(iss, reqTree);
	}
	catch(std::exception& e)
	{
		res->result(http::status::bad_request);
		res->body() = "{\"error\":\"Invalid JSON\"}";
		res->prepare_payload();
		sendResponse(res);
		return;
	}

	std::string accountName, password;
	int32_t clientVersion = 0;
	try
	{
		accountName = reqTree.get<std::string>("account");
		password = reqTree.get<std::string>("password");
		clientVersion = reqTree.get<int32_t>("client_version", 0);
	}
	catch(std::exception&)
	{
		res->result(http::status::bad_request);
		res->body() = "{\"error\":\"Missing account or password\"}";
		res->prepare_payload();
		sendResponse(res);
		return;
	}

	if(accountName.empty())
		accountName = "10";

	// Get client IP
	boost::system::error_code ec;
	tcp::endpoint remoteEp = m_socket->remote_endpoint(ec);
	std::string clientIpStr = "0.0.0.0";
	if(!ec)
		clientIpStr = remoteEp.address().to_string();

	// Rate limiting: max 5 attempts per 60 seconds per IP
	{
		time_t now = time(NULL);
		auto& entry = s_loginAttempts[clientIpStr];
		if(now - entry.second > 60)
		{
			entry.first = 0;
			entry.second = now;
		}

		if(++entry.first > 5)
		{
			res->result(http::status::too_many_requests);
			res->body() = "{\"error\":\"Too many login attempts. Please try again later.\"}";
			res->prepare_payload();
			sendResponse(res);
			return;
		}
	}

	// Check banishments
	IO::getInstance()->checkBanishments();

	// Load account
	Account account;
	if(!IO::getInstance()->loadAccount(account, accountName) ||
		(account.number != 10 && !encryptTest(account.salt + password, account.password)))
	{
		res->result(http::status::unauthorized);
		res->body() = "{\"error\":\"Invalid account name or password.\"}";
		res->prepare_payload();
		sendResponse(res);
		return;
	}

	// Check account ban
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
		ss << "Your account has been " << (deletion ? "deleted" : "banished") << " at: "
			<< formatDateEx(ban.added, "%d %b %Y") << " by: " << name_
			<< ". The comment given was: " << ban.comment << ".";

		boost::property_tree::ptree errTree;
		errTree.put("error", ss.str());

		std::ostringstream oss;
		boost::property_tree::write_json(oss, errTree);
		res->result(http::status::forbidden);
		res->body() = oss.str();
		res->prepare_payload();
		sendResponse(res);
		return;
	}

	// Filter characters by version
	uint16_t version = (uint16_t)clientVersion;
	if(!version)
		version = CLIENT_VERSION_MIN;

	Characters charList;
	for(Characters::iterator it = account.charList.begin(); it != account.charList.end(); ++it)
	{
		if((version >= it->second.server->getVersionMin() && version <= it->second.server->getVersionMax())
			|| version == it->second.server->getVersionCustom())
			charList[it->first] = it->second;
	}

	// Update premium
	IO::getInstance()->updatePremium(account);

	// Generate session token and grant IP access
	std::string sessionToken = IO::generateSessionToken();
	int32_t expireSeconds = (int32_t)g_config.getNumber(ConfigManager::IP_ACCESS_EXPIRE_SECONDS);
	IO::getInstance()->grantIpAccess(clientIpStr, sessionToken, expireSeconds);

	// Build JSON response
	boost::property_tree::ptree respTree;

	// MOTD
	std::ostringstream motdSs;
	motdSs << g_config.getNumber(ConfigManager::MOTD_ID) << "\n" << g_config.getString(ConfigManager::MOTD_TEXT);
	respTree.put("motd", motdSs.str());

	// Characters array
	boost::property_tree::ptree charsArray;
	for(Characters::iterator it = charList.begin(); it != charList.end(); ++it)
	{
		boost::property_tree::ptree charNode;
		charNode.put("name", it->second.name);
		charNode.put("world", it->second.server->getName());

		// Convert address from network byte order to string
		uint32_t addr = it->second.server->getAddress();
		std::ostringstream addrSs;
		addrSs << (addr & 0xFF) << "." << ((addr >> 8) & 0xFF) << "."
			<< ((addr >> 16) & 0xFF) << "." << ((addr >> 24) & 0xFF);
		charNode.put("address", addrSs.str());

		IntegerVec ports = it->second.server->getPorts();
		if(!ports.empty())
			charNode.put("port", ports[random_range(0, ports.size() - 1)]);

		charsArray.push_back(std::make_pair("", charNode));
	}
	respTree.add_child("characters", charsArray);

	// Premium days
	if(g_config.getBool(ConfigManager::FREE_PREMIUM))
		respTree.put("premium_days", (int32_t)GRATIS_PREMIUM);
	else
		respTree.put("premium_days", (int32_t)account.premiumDays);

	// Session token
	respTree.put("token", sessionToken);

	std::ostringstream oss;
	boost::property_tree::write_json(oss, respTree);
	res->body() = oss.str();
	res->prepare_payload();
	sendResponse(res);
}

void HttpSession::sendResponse(boost::shared_ptr<http::response<http::string_body> > res)
{
	http::async_write(*m_socket, *res,
		boost::bind(&HttpSession::onWrite, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred, res));
}

void HttpSession::onWrite(const boost::system::error_code& ec, std::size_t,
	boost::shared_ptr<http::response<http::string_body> > res)
{
	if(ec)
	{
		std::clog << "[Error - HttpSession::onWrite] " << ec.message() << std::endl;
		return;
	}

	if(res->need_eof())
	{
		boost::system::error_code shutdownEc;
		m_socket->shutdown(tcp::socket::shutdown_send, shutdownEc);
		return;
	}

	// Read another request
	m_buffer.consume(m_buffer.size());
	m_request = {};
	doRead();
}

// HttpListener

HttpListener::HttpListener(boost::asio::io_service& ioc, tcp::endpoint endpoint)
	: m_ioc(ioc), m_acceptor(ioc)
{
	boost::system::error_code ec;
	m_acceptor.open(endpoint.protocol(), ec);
	if(ec)
	{
		std::clog << "[Error - HttpListener] open: " << ec.message() << std::endl;
		return;
	}

	m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
	if(ec)
	{
		std::clog << "[Error - HttpListener] set_option: " << ec.message() << std::endl;
		return;
	}

	m_acceptor.bind(endpoint, ec);
	if(ec)
	{
		std::clog << "[Error - HttpListener] bind: " << ec.message() << std::endl;
		return;
	}

	m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
	if(ec)
	{
		std::clog << "[Error - HttpListener] listen: " << ec.message() << std::endl;
		return;
	}
}

void HttpListener::run()
{
	doAccept();
}

void HttpListener::doAccept()
{
	m_nextSocket = boost::make_shared<tcp::socket>(m_ioc);
	m_acceptor.async_accept(*m_nextSocket,
		boost::bind(&HttpListener::onAccept, shared_from_this(),
			boost::asio::placeholders::error));
}

void HttpListener::onAccept(const boost::system::error_code& ec)
{
	if(ec)
	{
		std::clog << "[Error - HttpListener::onAccept] " << ec.message() << std::endl;
	}
	else
	{
		boost::shared_ptr<HttpSession> session(new HttpSession(m_nextSocket));
		session->start();
	}

	doAccept();
}
