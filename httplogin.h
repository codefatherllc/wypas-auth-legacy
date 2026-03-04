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

#ifndef __HTTP_LOGIN__
#define __HTTP_LOGIN__
#include "otsystem.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class HttpSession : public boost::enable_shared_from_this<HttpSession>
{
	public:
		HttpSession(boost::shared_ptr<tcp::socket> socket);
		void start();

	private:
		void doRead();
		void onRead(const boost::system::error_code& ec, std::size_t bytes);
		void handleRequest();
		void handleLogin();
		void handleOptions();
		void sendResponse(boost::shared_ptr<http::response<http::string_body> > res);
		void onWrite(const boost::system::error_code& ec, std::size_t bytes,
			boost::shared_ptr<http::response<http::string_body> > res);

		void addCorsHeaders(http::response<http::string_body>& res);

		boost::shared_ptr<tcp::socket> m_socket;
		beast::flat_buffer m_buffer;
		http::request<http::string_body> m_request;
};

class HttpListener : public boost::enable_shared_from_this<HttpListener>
{
	public:
		HttpListener(boost::asio::io_service& ioc, tcp::endpoint endpoint);
		void run();

	private:
		void doAccept();
		void onAccept(const boost::system::error_code& ec);

		boost::asio::io_service& m_ioc;
		tcp::acceptor m_acceptor;
		boost::shared_ptr<tcp::socket> m_nextSocket;
};
#endif
