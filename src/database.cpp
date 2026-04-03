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

#include "database.h"

#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/ssl_mode.hpp>

// Static storage
std::string Database::s_dsn;
thread_local Database::ThreadLocalDB Database::t_holder;

// Worker pool statics
std::vector<std::thread> Database::s_workers;
std::deque<std::function<void()>> Database::s_taskQueue;
std::mutex Database::s_taskMutex;
std::condition_variable Database::s_taskCv;
ThreadState_t Database::s_workerState = STATE_RUNNING;

void Database::init(const std::string& dsn)
{
	s_dsn = dsn;
}

Database* Database::local()
{
	if(!t_holder.db)
	{
		t_holder.db.reset(new Database());
		t_holder.db->connect();
	}

	t_holder.db->use();
	return t_holder.db.get();
}

Database::Database()
	: m_connected(false), m_use(0), m_conn(nullptr), m_lastInsertId(0)
{
}

Database::~Database()
{
	if(m_conn)
	{
		boost::system::error_code ec;
		boost::mysql::diagnostics diag;
		m_conn->close(ec, diag);
		delete m_conn;
	}
}

bool Database::connect()
{
	m_connected = false;

	try
	{
		if(m_conn)
		{
			boost::system::error_code ec;
			boost::mysql::diagnostics diag;
			m_conn->close(ec, diag);
			delete m_conn;
		}

		boost::mysql::any_connection_params conn_params;
		conn_params.initial_buffer_size = 4096;
		m_conn = new boost::mysql::any_connection(m_ioc.get_executor(), conn_params);
		m_conn->set_meta_mode(boost::mysql::metadata_mode::full);

		boost::mysql::connect_params params;
		params.ssl = boost::mysql::ssl_mode::disable;

		// Parse DSN: mysql://user:pass@host:port/database
		std::string remainder = s_dsn;

		auto schemeEnd = remainder.find("://");
		if(schemeEnd != std::string::npos)
			remainder = remainder.substr(schemeEnd + 3);

		std::string userinfo, hostinfo, database;
		auto slashPos = remainder.find('/');
		if(slashPos != std::string::npos)
		{
			database = remainder.substr(slashPos + 1);
			// Strip query parameters (e.g. ?parseTime=true)
			auto qPos = database.find('?');
			if(qPos != std::string::npos)
				database = database.substr(0, qPos);
			remainder = remainder.substr(0, slashPos);
		}

		auto atPos = remainder.find('@');
		if(atPos != std::string::npos)
		{
			userinfo = remainder.substr(0, atPos);
			hostinfo = remainder.substr(atPos + 1);
		}
		else
		{
			hostinfo = remainder;
		}

		std::string user, pass;
		auto colonPos = userinfo.find(':');
		if(colonPos != std::string::npos)
		{
			user = userinfo.substr(0, colonPos);
			pass = userinfo.substr(colonPos + 1);
		}
		else
		{
			user = userinfo;
		}

		std::string host = "localhost";
		unsigned short port = 3306;
		auto portPos = hostinfo.find(':');
		if(portPos != std::string::npos)
		{
			host = hostinfo.substr(0, portPos);
			port = (unsigned short)std::stoi(hostinfo.substr(portPos + 1));
		}
		else if(!hostinfo.empty())
		{
			host = hostinfo;
		}

		params.server_address.emplace_host_and_port(host, port);
		params.username = user;
		params.password = pass;
		params.database = database;

		m_conn->connect(params);
		m_connected = true;
		return true;
	}
	catch(const boost::mysql::error_with_diagnostics& e)
	{
		std::clog << std::endl << "Failed connecting to database - MYSQL ERROR: "
			<< e.what() << " (" << e.get_diagnostics().server_message() << ")" << std::endl;
		return false;
	}
	catch(const std::exception& e)
	{
		std::clog << std::endl << "Failed connecting to database: " << e.what() << std::endl;
		return false;
	}
}

bool Database::reconnectAndRetry(const std::string& sql, bool isStore, DBResult** outResult)
{
	std::clog << "[Database] Connection lost, attempting reconnect..." << std::endl;
	if(!connect())
		return false;

	try
	{
		boost::mysql::results result;
		m_conn->execute(sql, result);
		m_lastInsertId = result.last_insert_id();

		if(isStore && outResult)
		{
			DBResult* dbResult = new DBResult(std::move(result));
			*outResult = verifyResult(dbResult);
		}
		return true;
	}
	catch(const std::exception& e)
	{
		std::clog << "[Database] Retry after reconnect failed: " << e.what() << std::endl;
		m_connected = false;
		return false;
	}
}

bool Database::rollback()
{
	if(!m_connected)
		return false;

	try
	{
		boost::mysql::results result;
		m_conn->execute("ROLLBACK", result);
		return true;
	}
	catch(const boost::mysql::error_with_diagnostics& e)
	{
		std::clog << "rollback() - MYSQL ERROR: " << e.what()
			<< " (" << e.get_diagnostics().server_message() << ")" << std::endl;
		return false;
	}
}

bool Database::commit()
{
	if(!m_connected)
		return false;

	try
	{
		boost::mysql::results result;
		m_conn->execute("COMMIT", result);
		return true;
	}
	catch(const boost::mysql::error_with_diagnostics& e)
	{
		std::clog << "commit() - MYSQL ERROR: " << e.what()
			<< " (" << e.get_diagnostics().server_message() << ")" << std::endl;
		return false;
	}
}

bool Database::query(std::string query)
{
	if(!m_connected)
		return false;

	try
	{
		boost::mysql::results result;
		m_conn->execute(query, result);
		m_lastInsertId = result.last_insert_id();
		return true;
	}
	catch(const boost::mysql::error_with_diagnostics& e)
	{
		std::clog << "query(): " << query << " - MYSQL ERROR: " << e.what()
			<< " (" << e.get_diagnostics().server_message() << ")" << std::endl;
		return reconnectAndRetry(query, false, nullptr);
	}
	catch(const std::exception& e)
	{
		std::clog << "query(): " << query << " - ERROR: " << e.what() << std::endl;
		return reconnectAndRetry(query, false, nullptr);
	}
}

DBResult* Database::storeQuery(std::string query)
{
	if(!m_connected)
		return NULL;

	try
	{
		boost::mysql::results result;
		m_conn->execute(query, result);
		m_lastInsertId = result.last_insert_id();

		DBResult* dbResult = new DBResult(std::move(result));
		return verifyResult(dbResult);
	}
	catch(const boost::mysql::error_with_diagnostics& e)
	{
		std::clog << "storeQuery(): " << query << " - MYSQL ERROR: " << e.what()
			<< " (" << e.get_diagnostics().server_message() << ")" << std::endl;
		DBResult* outResult = NULL;
		reconnectAndRetry(query, true, &outResult);
		return outResult;
	}
	catch(const std::exception& e)
	{
		std::clog << "storeQuery(): " << query << " - ERROR: " << e.what() << std::endl;
		DBResult* outResult = NULL;
		reconnectAndRetry(query, true, &outResult);
		return outResult;
	}
}

std::string Database::escapeBlob(const char* s, uint32_t length)
{
	if(!m_connected || length == 0)
		return "''";

	std::string output = "'";
	output.reserve(length * 2 + 3);
	for(uint32_t i = 0; i < length; ++i)
	{
		switch(s[i])
		{
			case '\0': output += "\\0"; break;
			case '\n': output += "\\n"; break;
			case '\r': output += "\\r"; break;
			case '\\': output += "\\\\"; break;
			case '\'': output += "\\'"; break;
			case '"':  output += "\\\""; break;
			case '\x1a': output += "\\Z"; break;
			default: output += s[i]; break;
		}
	}
	output += "'";
	return output;
}

DBResult* Database::verifyResult(DBResult* result)
{
	if(result->next())
		return result;

	result->free();
	return NULL;
}

// --- DBInsert ---

DBInsert::DBInsert(Database* db)
{
	m_db = db;
	m_rows = 0;
	m_multiLine = m_db->isMultiLine();
}

void DBInsert::setQuery(std::string query)
{
	m_query = query;
	m_buf = "";
	m_rows = 0;
}

bool DBInsert::addRow(std::string row)
{
	if(!m_multiLine)
		return m_db->query(m_query + "(" + row + ")");

	m_rows++;
	int32_t size = m_buf.length();
	if(!size)
		m_buf = "(" + row + ")";
	else if(size > 8192)
	{
		if(!execute())
			return false;

		m_buf = "(" + row + ")";
	}
	else
		m_buf += ",(" + row + ")";

	return true;
}

bool DBInsert::addRow(std::ostringstream& row)
{
	bool ret = addRow(row.str());
	row.str("");
	return ret;
}

bool DBInsert::execute()
{
	if(!m_multiLine || m_buf.length() < 1 || !m_rows)
		return true;

	bool ret = m_db->query(m_query + m_buf);
	m_rows = 0;
	m_buf = "";
	return ret;
}

// --- DBResult ---

int32_t DBResult::getDataInt(const std::string& s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_currentRow == 0 || m_currentRow > m_result.rows().size())
		{
			std::clog << "[DEBUG-DB] getDataInt(" << s << "): m_currentRow=" << m_currentRow << " rows=" << m_result.rows().size() << " -> skip" << std::endl;
			return 0;
		}

		auto row = m_result.rows().at(m_currentRow - 1);
		auto field = row.at(it->second);
		std::clog << "[DEBUG-DB] getDataInt(" << s << "): col=" << it->second << " null=" << field.is_null() << " int64=" << field.is_int64() << " uint64=" << field.is_uint64() << " string=" << field.is_string() << std::endl;
		if(field.is_null())
			return 0;

		if(field.is_int64())
			return (int32_t)field.as_int64();
		if(field.is_uint64())
			return (int32_t)field.as_uint64();
		if(field.is_string())
			return atoi(std::string(field.as_string()).c_str());
		return 0;
	}

	std::clog << "Error during getDataInt(" << s << ")." << std::endl;
	return 0;
}

int64_t DBResult::getDataLong(const std::string& s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_currentRow == 0 || m_currentRow > m_result.rows().size())
			return 0;

		auto row = m_result.rows().at(m_currentRow - 1);
		auto field = row.at(it->second);
		if(field.is_null())
			return 0;

		if(field.is_int64())
			return field.as_int64();
		if(field.is_uint64())
			return (int64_t)field.as_uint64();
		if(field.is_string())
			return atoll(std::string(field.as_string()).c_str());
		return 0;
	}

	std::clog << "Error during getDataLong(" << s << ")." << std::endl;
	return 0;
}

std::string DBResult::getDataString(const std::string& s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_currentRow == 0 || m_currentRow > m_result.rows().size())
			return std::string();

		auto row = m_result.rows().at(m_currentRow - 1);
		auto field = row.at(it->second);
		if(field.is_null())
			return std::string();

		if(field.is_string())
			return std::string(field.as_string());
		if(field.is_int64())
			return std::to_string(field.as_int64());
		if(field.is_uint64())
			return std::to_string(field.as_uint64());
		if(field.is_blob())
			return std::string(reinterpret_cast<const char*>(field.as_blob().data()), field.as_blob().size());
		return std::string();
	}

	std::clog << "Error during getDataString(" << s << ")." << std::endl;
	return std::string();
}

const char* DBResult::getDataStream(const std::string& s, uint64_t& size)
{
	size = 0;
	listNames_t::iterator it = m_listNames.find(s);
	if(it == m_listNames.end())
	{
		std::clog << "Error during getDataStream(" << s << ")." << std::endl;
		return NULL;
	}

	if(m_currentRow == 0 || m_currentRow > m_result.rows().size())
		return NULL;

	auto row = m_result.rows().at(m_currentRow - 1);
	auto field = row.at(it->second);
	if(field.is_null())
		return NULL;

	if(field.is_blob())
	{
		auto blob = field.as_blob();
		size = blob.size();
		return reinterpret_cast<const char*>(blob.data());
	}
	if(field.is_string())
	{
		auto sv = field.as_string();
		size = sv.size();
		return sv.data();
	}

	return NULL;
}

void DBResult::free()
{
	delete this;
}

bool DBResult::next()
{
	if(m_currentRow < m_result.rows().size())
	{
		m_currentRow++;
		return true;
	}
	return false;
}

DBResult::~DBResult()
{
}

DBResult::DBResult(boost::mysql::results&& result)
	: m_result(std::move(result)), m_currentRow(0)
{
	auto meta = m_result.meta();
	std::clog << "[DEBUG-DB] DBResult: rows=" << m_result.rows().size() << " cols=" << meta.size() << " names:";
	for(std::size_t i = 0; i < meta.size(); ++i)
	{
		m_listNames[std::string(meta[i].column_name())] = (uint32_t)i;
		std::clog << " " << meta[i].column_name();
	}
	std::clog << std::endl;
}

// --- Async worker pool ---

void Database::asyncQuery(std::string query)
{
	asyncTask([q = std::move(query)]()
	{
		Database* db = Database::local();
		if(db && db->isConnected())
			db->query(q);
	});
}

void Database::asyncTask(std::function<void()> task)
{
	{
		std::lock_guard<std::mutex> lock(s_taskMutex);
		if(s_workerState != STATE_RUNNING)
			return;

		s_taskQueue.emplace_back(std::move(task));
	}
	s_taskCv.notify_one();
}

void Database::workerLoop()
{
	std::unique_lock<std::mutex> lock(s_taskMutex, std::defer_lock);
	std::function<void()> task;
	std::srand((uint32_t)OTSYS_TIME());

	while(s_workerState != STATE_TERMINATED)
	{
		lock.lock();
		if(s_taskQueue.empty())
			s_taskCv.wait(lock);

		if(!s_taskQueue.empty() && s_workerState != STATE_TERMINATED)
		{
			task = std::move(s_taskQueue.front());
			s_taskQueue.pop_front();
		}

		lock.unlock();

		if(!task)
			continue;

		try
		{
			task();
		}
		catch(const std::exception&) {}

		task = nullptr;
	}
}

void Database::startWorkers(size_t count)
{
	s_workerState = STATE_RUNNING;
	s_workers.reserve(count);
	for(size_t i = 0; i < count; ++i)
		s_workers.emplace_back(&Database::workerLoop);
}

void Database::stopWorkers()
{
	s_taskMutex.lock();
	s_workerState = STATE_CLOSING;
	s_taskMutex.unlock();
}

void Database::shutdownWorkers()
{
	s_taskMutex.lock();
	s_workerState = STATE_TERMINATED;

	while(!s_taskQueue.empty())
	{
		auto task = std::move(s_taskQueue.front());
		s_taskQueue.pop_front();
		try
		{
			task();
		}
		catch(const std::exception&) {}
	}

	s_taskMutex.unlock();
	s_taskCv.notify_all();
}

void Database::joinWorkers()
{
	for(auto& w : s_workers)
	{
		if(w.joinable())
			w.join();
	}
}
