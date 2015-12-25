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

#ifndef __DISPATCHER__
#define __DISPATCHER__
#include "otsystem.h"

#include <boost/function.hpp>
#define DISPATCHER_TASK_EXPIRATION 2000

class CallbackTask;

class Task
{
	public:
		Task(boost::system_time expiration):
			m_expiration(expiration) {}
		virtual ~Task() {}

		virtual CallbackTask* getCallback() {return NULL;}

		void unsetExpiration() {m_expiration = boost::date_time::not_a_date_time;}
		bool hasExpired() const
		{
			if(m_expiration == boost::date_time::not_a_date_time)
				return false;

			return m_expiration < boost::get_system_time();
		}

	protected:
		boost::system_time m_expiration;
};

class CallbackTask : public Task
{
	public:
		virtual ~CallbackTask() {}

		CallbackTask(const boost::function<void (void)>& callback):
			Task(boost::date_time::not_a_date_time),
			m_callback(callback) {}
		CallbackTask(uint32_t ms, const boost::function<void (void)>& callback):
			Task(boost::get_system_time() + boost::posix_time::milliseconds(ms)),
			m_callback(callback) {}

		void operator()() {if(m_callback) m_callback();}
		virtual CallbackTask* getCallback() {return this;}

	protected:
		boost::function<void (void)> m_callback;
};

inline CallbackTask* createTask(boost::function<void (void)> f)
{
	return new CallbackTask(f);
}
inline CallbackTask* createTask(uint32_t expiration, boost::function<void (void)> f)
{
	return new CallbackTask(expiration, f);
}

class Dispatcher
{
	public:
		virtual ~Dispatcher() {}
		static Dispatcher& getInstance()
		{
			static Dispatcher dispatcher;
			return dispatcher;
		}

		void addTask(Task* task, bool front = false);

		void stop();
		void shutdown();
		void exit() {if(m_thread.joinable()) m_thread.join();}

		virtual void tasksThread();

	protected:
		virtual void flush();
		Dispatcher();

		boost::thread m_thread;
		boost::mutex m_taskLock;
		boost::condition_variable m_taskSignal;

		std::list<Task*> m_taskList;
		ThreadState_t m_threadState;
};
#endif
