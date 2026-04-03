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

#ifndef __DEFINITIONS__
#define __DEFINITIONS__

#define CLIENT_VERSION_MIN 963
#define CLIENT_VERSION_MAX 9632
#define CLIENT_VERSION_CUSTOM 964
#define CLIENT_VERSION_STRING "WypasOTS has been updated, please restart the game using launcher!\nAlternatively, you can download portable version from http://wots.pl"

#define CLIENT_VERSION_DAT 0
#define CLIENT_VERSION_SPR 0
#define CLIENT_VERSION_PIC 0
#define CLIENT_VERSION_DATA "WypasOTS has been updated, please restart the game using launcher!\nAlternatively, you can download portable version from http://wots.pl"

#define SOFTWARE_NAME "TheForgottenLoginServer"
#define SOFTWARE_VERSION "2.0"
#define SOFTWARE_CODENAME "TFLS 2.0"
#define SOFTWARE_PROTOCOL "9.63"

#define NETWORK_RETRY_TIMEOUT 5000
#define NETWORK_SOCKET_SIZE 4096
#define NETWORK_HEADER_SIZE 2
#define NETWORK_CRYPTOHEADER_SIZE (NETWORK_HEADER_SIZE + 6)
#define NETWORK_MAX_SIZE 16384
#define NETWORK_BODY_SIZE (NETWORK_MAX_SIZE - NETWORK_CRYPTOHEADER_SIZE - 6)

#define LOCALHOST 2130706433
#define GRATIS_PREMIUM 65535

enum DatabaseEngine_t
{
	DATABASE_ENGINE_NONE = 0,
	DATABASE_ENGINE_MYSQL,
};

enum ThreadState_t
{
	STATE_RUNNING = 0,
	STATE_CLOSING,
	STATE_TERMINATED
};

enum Encryption_t
{
	ENCRYPTION_PLAIN = 0,
	ENCRYPTION_MD5,
	ENCRYPTION_SHA1,
	ENCRYPTION_SHA256,
	ENCRYPTION_SHA512
};

#define MAX_RAND_RANGE 10000000
#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_ASIO_ENABLE_CANCELIO 1

#ifdef __GNUC__
	#define __USE_ZLIB__
#endif

#ifndef __EXCEPTION_TRACER__
	#define DEBUG_REPORT
#endif
#endif
