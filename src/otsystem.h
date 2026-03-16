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

#ifndef __OTSYSTEM__
#define __OTSYSTEM__
#include "definitions.h"

#include <string>
#include <algorithm>
#include <bitset>
#include <queue>
#include <set>
#include <vector>
#include <list>
#include <map>
#include <limits>
#include <iostream>

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstdint>

#ifndef __x86_64__
	#define __x86_64__ 0
#endif

#include <ctime>
#include <cassert>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

inline void OTSYS_SLEEP(int32_t n)
{
	timespec tv;
	tv.tv_sec  = n / 1000;
	tv.tv_nsec = (n % 1000) * 1000000;
	nanosleep(&tv, NULL);
}

inline int64_t OTSYS_TIME()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((int64_t)tv.tv_usec / 1000) + ((int64_t)tv.tv_sec) * 1000;
}

inline uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH
#endif
