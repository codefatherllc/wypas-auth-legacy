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
#include "otsystem.h"
#include <signal.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <unistd.h>
#include <termios.h>

#include <boost/config.hpp>

#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#include "server.h"
#include "networkmessage.h"
#include "tasks.h"
#include "scheduler.h"

#include "protocollogin.h"

#include "database.h"
#include "tools.h"
#include "worlds.h"
#include "configmanager.h"

#include "io.h"
#include "textlogger.h"

#ifdef __NO_BOOST_EXCEPTIONS__
#include <exception>

inline void boost::throw_exception(std::exception const & e)
{
	std::clog << "Boost exception: " << e.what() << std::endl;
}
#endif

RSA* g_RSA;
ConfigManager g_config;

boost::mutex g_loaderLock;
boost::condition_variable g_loaderSignal;
boost::unique_lock<boost::mutex> g_loaderUniqueLock(g_loaderLock);

// Parsed from --path or defaults to "."
static std::string g_configDir = ".";

bool argumentsHandler(StringVec args)
{
	StringVec tmp;
	for(StringVec::iterator it = args.begin(); it != args.end(); ++it)
	{
		if((*it) == "--help")
		{
			std::clog << "Usage:\n"
			"\n"
			"\t--path=$1\t\tPath to config directory (containing config.json).\n"
			"\t--runfile=$1\t\tSpecifies run file. Will contain the pid\n"
			"\t\t\t\tof the server process as long as run status.\n";
			std::clog << "\t--log=$1\t\tWhole standard output will be logged to\n"
			"\t\t\t\tthis file.\n"
			"\t--daemon, -d\t\tRun as daemon.\n"
			"\t--version, -v\t\tShow version.\n";
			return false;
		}

		if((*it) == "--version" || (*it) == "-v")
		{
			std::clog << SOFTWARE_NAME << ", version " << SOFTWARE_VERSION << " (" << SOFTWARE_CODENAME << ")\n"
			"Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << ".\n"
			"A login server developed by Elf.\n"
			"Visit our forum for updates, support and resources: http://otland.net.\n";
			return false;
		}

		tmp = explodeString((*it), "=");
		if(tmp[0] == "--path")
			g_configDir = tmp[1];
		else if(tmp[0] == "--runfile" || tmp[0] == "--run-file" || tmp[0] == "--pidfile" || tmp[0] == "--pid-file")
			g_config.setString(ConfigManager::RUNFILE, tmp[1]);
		else if(tmp[0] == "--log")
			g_config.setString(ConfigManager::OUTPUT_LOG, tmp[1]);
		else if(tmp[0] == "--daemon" || tmp[0] == "-d")
			g_config.setBool(ConfigManager::DAEMONIZE, true);
	}

	return true;
}

int32_t getch()
{
	struct termios oldt;
	tcgetattr(STDIN_FILENO, &oldt);

	struct termios newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	int32_t ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

void runfileHandler(void)
{
	std::ofstream runfile(g_config.getString(ConfigManager::RUNFILE).c_str(), std::ios::trunc | std::ios::out);
	runfile.close();
}

void allocationHandler()
{
	puts("Allocation failed, server out of memory!\nDecrease size of your map or compile in a 64-bit mode.");
	getch();
	std::exit(-1);
}

void startupErrorMessage(std::string error = "")
{
	if(error.length() > 0)
		std::clog << std::endl << "> ERROR: " << error << std::endl;

	getch();
	std::exit(-1);
}

void motdReloadTask()
{
	g_config.reloadMotd();
	Scheduler::getInstance().addEvent(createSchedulerTask(60000, boost::bind(&motdReloadTask)));
}

void otlogin(StringVec args, ServiceManager* services);
int main(int argc, char* argv[])
{
	StringVec args = StringVec(argv, argv + argc);
	if(argc > 1 && !argumentsHandler(args))
		return 0;

	std::set_new_handler(allocationHandler);
	ServiceManager servicer;
	g_config.startup();

	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;

	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);

	OutputHandler::getInstance();
	Dispatcher::getInstance().addTask(createTask(boost::bind(otlogin, args, &servicer)));
	g_loaderSignal.wait(g_loaderUniqueLock);

	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	if(servicer.isRunning())
	{
		std::clog << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Online!" << std::endl << std::endl;
		servicer.run();
	}
	else
		std::clog << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Offline! No services available..." << std::endl << std::endl;

	Dispatcher::getInstance().exit();
	Scheduler::getInstance().exit();

	Database::shutdownWorkers();
	Database::joinWorkers();
	return 0;
}

void otlogin(StringVec, ServiceManager* services)
{
#if !defined(__ROOT_PERMISSION__)
	if(!getuid() || !geteuid())
	{
		std::clog << "> WARNING: " << SOFTWARE_NAME << " has been executed as super user! It is "
			<< "recommended to run as a normal user." << std::endl << "Continue? (y/N)" << std::endl;
		char buffer = getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Aborted.");
	}
#endif

	std::clog << SOFTWARE_NAME << ", version " << SOFTWARE_VERSION << " (" << SOFTWARE_CODENAME << ")" << std::endl
		<< "Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << "." << std::endl
		<< "A login server developed by Elf." << std::endl
		<< "Visit our forum for updates, support and resources: http://otland.net." << std::endl << std::endl;
	std::ostringstream ss;
#ifdef __DEBUG__
	ss << " GLOBAL";
#endif
#ifdef __DEBUG_NET__
	ss << " NET";
#endif
#ifdef __DEBUG_NET_DETAIL__
	ss << " NET-DETAIL";
#endif
#ifdef __DEBUG_SCHEDULER__
	ss << " SCHEDULER";
#endif
#ifdef __SQL_QUERY_DEBUG__
	ss << " SQL-QUERIES";
#endif

	std::string debug = ss.str();
	if(!debug.empty())
		std::clog << ">> Debugging:" << debug << "." << std::endl;

	std::clog << ">> Loading config (" << g_configDir << "/config.json)" << std::endl;
	if(!g_config.load(g_configDir))
		startupErrorMessage("Unable to load " + g_configDir + "/config.json!");

	if(g_config.getBool(ConfigManager::DAEMONIZE))
	{
		std::clog << "> Daemonization... ";
		if(fork())
		{
			std::clog << "succeed, bye!" << std::endl;
			exit(0);
		}
		else
			std::clog << "failed, continuing." << std::endl;
	}

	std::clog << "> Opening logs" << std::endl;
	Logger::getInstance()->open();

	IntegerVec cores = g_config.getCoresUsed();
	if(cores[0] != -1)
	{
#ifndef __APPLE__
		cpu_set_t mask;
		CPU_ZERO(&mask);
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			CPU_SET((*it), &mask);

		sched_setaffinity(getpid(), (int32_t)sizeof(mask), &mask);
#endif
	}

	std::string runPath = g_config.getString(ConfigManager::RUNFILE);
	if(!runPath.empty() && runPath.length() > 2)
	{
		std::ofstream runFile(runPath.c_str(), std::ios::trunc | std::ios::out);
		runFile << getpid();
		runFile.close();
		atexit(runfileHandler);
	}

	if(!nice(g_config.getNumber(ConfigManager::NICE_LEVEL))) {}

	std::string encryptionType = asLowerCaseString(g_config.getString(ConfigManager::ENCRYPTION_TYPE));
	if(encryptionType == "md5")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_MD5);
		std::clog << "> Using MD5 encryption" << std::endl;
	}
	else if(encryptionType == "sha1")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA1);
		std::clog << "> Using SHA1 encryption" << std::endl;
	}
	else if(encryptionType == "sha256")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA256);
		std::clog << "> Using SHA256 encryption" << std::endl;
	}
	else if(encryptionType == "sha512")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA512);
		std::clog << "> Using SHA512 encryption" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_PLAIN);
		std::clog << "> Using plaintext encryption" << std::endl << std::endl
			<< "> WARNING: This method is completely unsafe!" << std::endl
			<< "> Please set auth.encryptionType = \"sha1\" (or any other available method) in config.json" << std::endl;
		boost::this_thread::sleep(boost::posix_time::seconds(15));
	}

	std::clog << ">> Loading RSA key";
	g_RSA = RSA_new();

	BIGNUM *p = NULL, *q = NULL, *d = NULL, *n = NULL, *e = NULL;
	BN_dec2bn(&p, g_config.getString(ConfigManager::RSA_PRIME1).c_str());
	BN_dec2bn(&q, g_config.getString(ConfigManager::RSA_PRIME2).c_str());
	BN_dec2bn(&d, g_config.getString(ConfigManager::RSA_PRIVATE).c_str());
	BN_dec2bn(&n, g_config.getString(ConfigManager::RSA_MODULUS).c_str());
	BN_dec2bn(&e, g_config.getString(ConfigManager::RSA_PUBLIC).c_str());

	RSA_set0_key(g_RSA, n, e, d);
	RSA_set0_factors(g_RSA, p, q);

	if(RSA_check_key(g_RSA))
	{
		std::clog << std::endl << "> Calculating dmp1, dmq1 and iqmp for RSA...";

		BN_CTX* ctx = BN_CTX_new();
		BN_CTX_start(ctx);

		const BIGNUM *d_read = NULL, *p_read = NULL, *q_read = NULL;
		RSA_get0_key(g_RSA, NULL, NULL, &d_read);
		RSA_get0_factors(g_RSA, &p_read, &q_read);

		BIGNUM *r1 = BN_CTX_get(ctx), *r2 = BN_CTX_get(ctx);
		BIGNUM *dmp1 = BN_new(), *dmq1 = BN_new(), *iqmp = BN_new();

		BN_sub(r1, p_read, BN_value_one());
		BN_sub(r2, q_read, BN_value_one());
		BN_mod(dmp1, d_read, r1, ctx);
		BN_mod(dmq1, d_read, r2, ctx);
		BN_mod_inverse(iqmp, q_read, p_read, ctx);

		RSA_set0_crt_params(g_RSA, dmp1, dmq1, iqmp);

		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		std::clog << " done" << std::endl;
	}
	else
	{
		std::ostringstream s;

		s << std::endl << "> OpenSSL failed - " << ERR_error_string(ERR_get_error(), NULL);
		startupErrorMessage(s.str());
	}

	// Init database from DSN
	std::clog << ">> Starting SQL connection" << std::endl;
	Database::init(g_config.getString(ConfigManager::DATABASE_DSN));

	Database* db = Database::local();
	if(!db || !db->isConnected())
		startupErrorMessage("Couldn't estabilish connection to SQL database!");

	Database::startWorkers(2);

	std::clog << ">> Loading worlds" << std::endl;
	if(!Worlds::getInstance()->loadFromJson(g_configDir, true))
		startupErrorMessage("Unable to load worlds!");

	if(g_config.getBool(ConfigManager::INIT_PREMIUM_UPDATE))
	{
		std::clog << ">> Updating premium status" << std::endl;
		IO::getInstance()->updatePremium();
	}

	Scheduler::getInstance().addEvent(createSchedulerTask(60000, boost::bind(&motdReloadTask)));
	std::clog << ">> MOTD reload scheduled (every 60s from Discord)" << std::endl;

	std::clog << ">> Starting to dominate the world... done." << std::endl
		<< ">> Binding services..." << std::endl;

	// Parse legacy.listenAddr as host:port
	std::string listenAddr = g_config.getString(ConfigManager::LISTEN_ADDR);

	std::string bindHost = "0.0.0.0";
	uint16_t bindPort = 7171;
	{
		// Parse host:port
		auto colonPos = listenAddr.rfind(':');
		if(colonPos != std::string::npos)
		{
			bindHost = listenAddr.substr(0, colonPos);
			bindPort = static_cast<uint16_t>(std::atoi(listenAddr.substr(colonPos + 1).c_str()));
		}
	}

	IPAddressList ipList;
	boost::system::error_code ec;
	IPAddress bindAddr = boost::asio::ip::make_address(bindHost, ec);
	if(ec)
	{
		startupErrorMessage("Cannot parse bind address: " + bindHost + " (" + ec.message() + ")");
	}
	ipList.push_back(bindAddr);

	std::clog << "> Binding to " << bindHost << ":" << bindPort << std::endl;
	services->add<ProtocolLogin>(bindPort, ipList);

	std::clog << "> Bound ports: ";
	std::list<uint16_t> ports = services->getPorts();
	for(std::list<uint16_t>::iterator it = ports.begin(); it != ports.end(); ++it)
		std::clog << (*it) << "\t";

	std::clog << std::endl << ">> Everything smells good, login server is starting up..." << std::endl;
	g_loaderSignal.notify_all();
}
