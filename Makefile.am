noinst_PROGRAMS = theforgottenloginserver

CXXFLAGS = -pipe -g
AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA_CFLAGS) $(DEBUG_FLAGS) \
$(MYSQL_FLAGS) $(SQLITE_FLAGS) $(PROFILER_FLAGS) $(OPTIONAL_FLAGS) \
-D_THREAD_SAFE -D_REENTRANT -Wall -Wextra -Werror -Wno-strict-aliasing

theforgottenloginserver_LDADD = $(LUA_LIBS)

if USE_MYSQL
MAYBE_MYSQL = databasemysql.cpp databasemysql.h
endif
if USE_SQLITE
MAYBE_SQLITE = databasesqlite.cpp databasesqlite.h
endif

theforgottenloginserver_SOURCES = config.h configmanager.cpp configmanager.h connection.cpp \
	connection.h const.h database.cpp database.h $(MAYBE_MYSQL) $(MAYBE_SQLITE) \
	gameservers.cpp gameservers.h httplogin.cpp httplogin.h io.cpp io.h \
	networkmessage.cpp networkmessage.h \
	otpch.h otlogin.cpp otsystem.h outputmessage.cpp outputmessage.h protocol.cpp \
	protocol.h protocollogin.cpp protocollogin.h protocolold.cpp protocolold.h \
	scheduler.cpp scheduler.h server.cpp server.h status.cpp status.h tasks.cpp \
	tasks.h textlogger.cpp textlogger.h tools.cpp tools.h
