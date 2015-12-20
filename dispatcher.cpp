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
#include "dispatcher.h"
#include "outputmessage.h"

Dispatcher::Dispatcher()
{
	m_threadState = STATE_RUNNING;
	m_thread = boost::thread(boost::bind(&Dispatcher::tasksThread, this));
}

void Dispatcher::tasksThread()
{
	OutputMessagePool* outputPool = NULL;
	boost::unique_lock<boost::mutex> taskLockUnique(m_taskLock, boost::defer_lock);
	while(m_threadState != STATE_TERMINATED)
	{
		Task* task = NULL;
		// check if there are tasks waiting
		taskLockUnique.lock();
		if(m_taskList.empty()) //if the list is empty wait for signal
			m_taskSignal.wait(taskLockUnique);

		if(!m_taskList.empty() && m_threadState != STATE_TERMINATED)
		{
			// take the first task
			task = m_taskList.front();
			m_taskList.pop_front();
		}

		taskLockUnique.unlock();
		// finally execute the task...
		if(!task)
			continue;

		if(!task->hasExpired())
		{
			if((outputPool = OutputMessagePool::getInstance()))
				outputPool->startExecutionFrame();

			(*task)();
			if(outputPool)
				outputPool->sendAll();
		}

		delete task;
	}
}

void Dispatcher::addTask(Task* task, bool front/* = false*/)
{
	bool signal = false;
	m_taskLock.lock();
	if(m_threadState == STATE_RUNNING)
	{
		signal = m_taskList.empty();
		if(front)
			m_taskList.push_front(task);
		else
			m_taskList.push_back(task);
	}
	else
	{
		#ifdef __DEBUG_SCHEDULER__
		std::clog << "[Error - Dispatcher::addTask] Dispatcher thread is terminated." << std::endl;
		#endif
		delete task;
		task = NULL;
	}

	m_taskLock.unlock();
	// send a signal if the list was empty
	if(signal)
		m_taskSignal.notify_one();
}

void Dispatcher::flush()
{
	Task* task = NULL;
	OutputMessagePool* outputPool = OutputMessagePool::getInstance();
	while(!m_taskList.empty())
	{
		task = m_taskList.front();
		m_taskList.pop_front();

		(*task)();
		delete task;
		if(outputPool)
			outputPool->sendAll();
	}
}

void Dispatcher::stop()
{
	m_taskLock.lock();
	m_threadState = STATE_CLOSING;
	m_taskLock.unlock();
}

void Dispatcher::shutdown()
{
	m_taskLock.lock();
	m_threadState = STATE_TERMINATED;

	flush();
	m_taskLock.unlock();
}

Helper::Helper()
{
	m_threadState = STATE_RUNNING;
	m_thread = boost::thread(boost::bind(&Helper::tasksThread, this));
}

void Helper::tasksThread()
{
	boost::unique_lock<boost::mutex> taskLockUnique(m_taskLock, boost::defer_lock);
	while(m_threadState != STATE_TERMINATED)
	{
		Task* task = NULL;
		// check if there are tasks waiting
		taskLockUnique.lock();
		if(m_taskList.empty()) //if the list is empty wait for signal
			m_taskSignal.wait(taskLockUnique);

		if(!m_taskList.empty() && m_threadState != STATE_TERMINATED)
		{
			// take the first task
			task = m_taskList.front();
			m_taskList.pop_front();
		}

		taskLockUnique.unlock();
		// finally execute the task...
		if(!task)
			continue;

		if(!task->hasExpired())
			(*task)();

		delete task;
	}
}

void Helper::flush()
{
	Task* task = NULL;
	while(!m_taskList.empty())
	{
		task = m_taskList.front();
		m_taskList.pop_front();

		(*task)();
		delete task;
	}
}
