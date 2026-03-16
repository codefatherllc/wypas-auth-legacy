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
// along with this program. If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include <iostream>

#include "networkmessage.h"

std::string NetworkMessage::getString(bool peek/* = false*/, uint16_t size/* = 0*/)
{
	if(!size)
		size = get<uint16_t>(peek);

	uint16_t position = m_position;
	if(peek)
		position += 2;

	if(size >= (NETWORK_MAX_SIZE - position))
		return std::string();

	char* v = (char*)(m_buffer + position);
	if(peek)
		return std::string(v, size);

	m_position += size;
	return std::string(v, size);
}

void NetworkMessage::putString(const char* value, uint32_t length, bool addSize/* = true*/)
{
	uint32_t size = (uint32_t)length;
	if(!hasSpace(size + (addSize ? 2 : 0)) || size > (addSize ? 8192 : NETWORK_BODY_SIZE))
		return;

	if(addSize)
		put<uint16_t>(size);

	memcpy((char*)(m_buffer + m_position), value, length);
	m_position += size;
	m_size += size;
}

void NetworkMessage::putPadding(uint32_t amount)
{
	if(!hasSpace(amount))
		return;

	memset((void*)&m_buffer[m_position], 0x33, amount);
	m_size += amount;
}

int32_t NetworkMessage::decodeHeader()
{
	int32_t size = (int32_t)(m_buffer[0] | m_buffer[1] << 8);
	m_size = size;
	return size;
}
