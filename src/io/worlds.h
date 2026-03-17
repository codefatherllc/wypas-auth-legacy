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

#ifndef __WORLDS__
#define __WORLDS__
#include "otsystem.h"

class World
{
	public:
		World(): name("TheForgottenServer"), address(LOCALHOST) {}
		World(std::string _name, uint32_t _address, std::vector<int32_t> _ports):
			name(_name), address(_address), ports(_ports) {}
		virtual ~World() {}

		std::string getName() const {return name;}
		uint32_t getAddress() const {return address;}
		std::vector<int32_t> getPorts() const {return ports;}

	protected:
		std::string name;
		uint32_t address;
		std::vector<int32_t> ports;
};

typedef std::map<uint32_t, World*> WorldsMap;
class Worlds
{
	public:
		Worlds() {}
		virtual ~Worlds() {clear();}

		static Worlds* getInstance()
		{
			static Worlds instance;
			return &instance;
		}

		bool loadFromJson(const std::string& configDir, bool verbose);

		World* getWorldById(uint32_t id) const;

		WorldsMap::const_iterator getFirstWorld() const {return worldList.begin();}
		WorldsMap::const_iterator getLastWorld() const {return worldList.end();}

	protected:
		void clear();

		WorldsMap worldList;
};
#endif
