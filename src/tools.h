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

#ifndef __TOOLS__
#define __TOOLS__
#include "otsystem.h"

typedef std::vector<std::string> StringVec;
typedef std::vector<int32_t> IntegerVec;

enum FileType_t
{
	FILE_TYPE_LOG,
	FILE_TYPE_OTHER,
	FILE_TYPE_CONFIG
};

enum DistributionType_t
{
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
};

template <typename T>
inline void asString(const T& object, std::string& s)
{
	std::ostringstream ss;
	ss << object;
	s = ss.str();
}

template <typename T>
inline std::string asString(const T& object)
{
	std::ostringstream ss;
	ss << object;
	return ss.str();
}

template<class T>
inline T fromString(const std::string& s)
{
	std::istringstream ss (s);
	T t;
	ss >> t;
	return t;
}

void trim_right(std::string& source, const std::string& t);
void trim_left(std::string& source, const std::string& t);
std::string trimString(std::string& str);

void toLowerCaseString(std::string& source);
void toUpperCaseString(std::string& source);

std::string asLowerCaseString(const std::string& source);
std::string asUpperCaseString(const std::string& source);

bool replaceString(std::string& text, const std::string& key, const std::string& value);
bool booleanString(std::string source);

char upchar(char character);
std::string ucfirst(std::string source);
std::string ucwords(std::string source);

bool isNumber(char character);
bool isNumbers(std::string text);

bool isLowercaseLetter(char character);
bool isUppercaseLetter(char character);

bool isPasswordCharacter(char character);

bool isValidAccountName(std::string text);
bool isValidPassword(std::string text);
bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);

std::string transformToMD5(std::string plainText, bool upperCase);
std::string transformToSHA1(std::string plainText, bool upperCase);
std::string transformToSHA256(std::string plainText, bool upperCase);
std::string transformToSHA512(std::string plainText, bool upperCase);

void _encrypt(std::string& str, bool upperCase);
bool encryptTest(std::string plain, std::string& hash);

StringVec explodeString(const std::string& string, const std::string& separator, bool trim = true, uint16_t limit = 0);
IntegerVec vectorAtoi(StringVec stringVector);

bool checkText(std::string text, std::string str);
std::string convertIPAddress(uint32_t ip);

std::string formatDate(time_t _time = 0);
std::string formatDateEx(time_t _time = 0, std::string format = "%d %b %Y, %H:%M:%S");
std::string formatTime(time_t _time = 0, bool miliseconds = false);

uint32_t rand24b();
float box_muller(float m, float s);
int32_t random_range(int32_t lowestNumber, int32_t highestNumber, DistributionType_t type = DISTRO_UNIFORM);

int32_t roundValue(float v);
bool hasBitSet(uint32_t flag, uint32_t flags);
uint32_t adlerChecksum(uint8_t* data, size_t length);

bool parseIntegerVec(std::string str, IntegerVec& intVector);

bool fileExists(const std::string& filename);
std::string getFilePath(FileType_t type, std::string name = "");
#endif
