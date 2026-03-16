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
#include <fstream>
#include <sstream>

#include "worlds.h"
#include "tools.h"

#include <boost/json.hpp>

void Worlds::clear()
{
	for(WorldsMap::iterator it = worldList.begin(); it != worldList.end(); ++it)
		delete it->second;

	worldList.clear();
}

bool Worlds::reload()
{
	clear();
	return loadFromJson(false);
}

bool Worlds::loadFromJson(bool verbose)
{
	std::string path = getFilePath(FILE_TYPE_CONFIG, "worlds.json");
	std::ifstream file(path);
	if(!file.is_open())
	{
		std::clog << "[Warning - Worlds::loadFromJson] Cannot open worlds file: " << path << std::endl;
		return false;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string content = ss.str();

	try
	{
		boost::json::value json = boost::json::parse(content);
		if(!json.is_array())
		{
			std::clog << "[Error - Worlds::loadFromJson] worlds.json must be a JSON array." << std::endl;
			return false;
		}

		for(auto& entry : json.as_array())
		{
			if(!entry.is_object())
				continue;

			auto& obj = entry.as_object();

			if(!obj.contains("id") || !obj.at("id").is_int64())
			{
				std::clog << "[Error - Worlds::loadFromJson] Missing id, skipping" << std::endl;
				continue;
			}

			uint32_t id = (uint32_t)obj.at("id").as_int64();
			if(worldList.find(id) != worldList.end())
			{
				std::clog << "[Error - Worlds::loadFromJson] Duplicate world id " << id << ", skipping" << std::endl;
				continue;
			}

			std::string name = "Server #" + std::to_string(id);
			if(obj.contains("name") && obj.at("name").is_string())
				name = std::string(obj.at("name").as_string());

			std::string address = "localhost";
			if(obj.contains("address") && obj.at("address").is_string())
				address = std::string(obj.at("address").as_string());

			IntegerVec ports;
			if(obj.contains("ports") && obj.at("ports").is_array())
			{
				for(auto& p : obj.at("ports").as_array())
				{
					if(p.is_int64())
						ports.push_back((int32_t)p.as_int64());
				}
			}

			if(ports.empty())
			{
				ports.push_back(7172);
				std::clog << "[Warning - Worlds::loadFromJson] Missing ports for world " << id << ", using default 7172" << std::endl;
			}

			worldList[id] = new World(name, inet_addr(address.c_str()), ports);
		}
	}
	catch(const std::exception& e)
	{
		std::clog << "[Error - Worlds::loadFromJson] Parse error: " << e.what() << std::endl;
		return false;
	}

	if(verbose)
	{
		std::clog << "> Worlds loaded:" << std::endl;
		for(WorldsMap::iterator it = worldList.begin(); it != worldList.end(); ++it)
		{
			IntegerVec games = it->second->getPorts();
			for(IntegerVec::const_iterator tit = games.begin(); tit != games.end(); ++tit)
				std::clog << it->second->getName() << " (" << it->second->getAddress() << ":" << *tit << ")" << std::endl;
		}
	}

	return true;
}

World* Worlds::getWorldById(uint32_t id) const
{
	WorldsMap::const_iterator it = worldList.find(id);
	if(it != worldList.end())
		return it->second;

	return NULL;
}
