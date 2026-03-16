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

#ifndef __DATABASE__
#define __DATABASE__
#include "otsystem.h"
#include <sstream>

#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/connect_params.hpp>
#include <boost/mysql/results.hpp>
#include <boost/asio/io_context.hpp>

#include <map>
#include <memory>
#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>

class DBResult;

class Database
{
	public:
		friend class DBTransaction;

		static Database* local();
		static void init(const std::string& dsn);

		// Async worker pool
		static void asyncQuery(std::string query);
		static void asyncTask(std::function<void()> task);
		static void startWorkers(size_t count = 2);
		static void stopWorkers();
		static void shutdownWorkers();
		static void joinWorkers();

		bool isMultiLine() {return true;}
		bool connect();
		bool isConnected() const {return m_connected;}
		void use() {m_use = OTSYS_TIME();}

		bool beginTransaction() {return query("BEGIN");}
		bool rollback();
		bool commit();

		bool query(std::string query);
		DBResult* storeQuery(std::string query);

		std::string escapeString(std::string s) {return escapeBlob(s.c_str(), s.length());}
		std::string escapeBlob(const char* s, uint32_t length);

		uint64_t getLastInsertId() {return m_lastInsertId;}
		std::string getStringComparer() {return "= ";}
		std::string getUpdateLimiter() {return " LIMIT 1;";}

		DatabaseEngine_t getDatabaseEngine() {return DATABASE_ENGINE_MYSQL;}

		~Database();

	protected:
		DBResult* verifyResult(DBResult* result);

		Database();

		bool m_connected;
		int64_t m_use;

		boost::asio::io_context m_ioc;
		boost::mysql::any_connection* m_conn;
		uint64_t m_lastInsertId;

	private:
		bool reconnectAndRetry(const std::string& sql, bool isStore, DBResult** outResult);

		static std::string s_dsn;

		struct ThreadLocalDB {
			std::unique_ptr<Database> db;
			~ThreadLocalDB() { db.reset(); }
		};
		static thread_local ThreadLocalDB t_holder;

		static std::vector<std::thread> s_workers;
		static std::deque<std::function<void()>> s_taskQueue;
		static std::mutex s_taskMutex;
		static std::condition_variable s_taskCv;
		static ThreadState_t s_workerState;
		static void workerLoop();
};

class DBResult
{
	friend class Database;
	public:
		int32_t getDataInt(const std::string& s);
		int64_t getDataLong(const std::string& s);
		std::string getDataString(const std::string& s);
		const char* getDataStream(const std::string& s, uint64_t& size);

		void free();
		bool next();

	protected:
		DBResult(boost::mysql::results&& result);
		~DBResult();

		typedef std::map<const std::string, uint32_t> listNames_t;
		listNames_t m_listNames;

		boost::mysql::results m_result;
		std::size_t m_currentRow;
};

class DBInsert
{
	public:
		DBInsert(Database* db);
		~DBInsert() {}

		void setQuery(std::string query);

		bool addRow(std::string row);
		bool addRow(std::ostringstream& row);

		bool execute();

	protected:
		Database* m_db;
		bool m_multiLine;

		uint32_t m_rows;
		std::string m_query, m_buf;
};

class DBTransaction
{
	public:
		DBTransaction(Database* database)
		{
			m_db = database;
			m_state = STATE_FRESH;
		}

		~DBTransaction()
		{
			if(m_state != STATE_READY)
				return;

			m_db->rollback();
		}

		bool begin()
		{
			m_state = STATE_READY;
			return m_db->beginTransaction();
		}

		bool commit()
		{
			if(m_state != STATE_READY)
				return false;

			m_state = STATE_DONE;
			return m_db->commit();
		}

	private:
		Database* m_db;
		enum TransactionStates_t
		{
			STATE_FRESH,
			STATE_READY,
			STATE_DONE
		} m_state;
};
#endif
