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
#include "tools.h"

#include <iostream>
#include <iomanip>

#include <boost/filesystem.hpp>
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "configmanager.h"

extern ConfigManager g_config;

std::string transformToMD5(std::string plainText, bool upperCase)
{
	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, plainText.c_str(), plainText.length());

	uint8_t md[MD5_DIGEST_LENGTH];
	MD5_Final(md, &c);

	char output[(MD5_DIGEST_LENGTH << 1) + 1];
	for(int32_t i = 0; i < (int32_t)sizeof(md); ++i)
		sprintf(output + (i << 1), "%.2X", md[i]);

	if(upperCase)
		return std::string(output);

	return asLowerCaseString(std::string(output));
}

std::string transformToSHA1(std::string plainText, bool upperCase)
{
	SHA_CTX c;
	SHA1_Init(&c);
	SHA1_Update(&c, plainText.c_str(), plainText.length());

	uint8_t md[SHA_DIGEST_LENGTH];
	SHA1_Final(md, &c);

	char output[(SHA_DIGEST_LENGTH << 1) + 1];
	for(int32_t i = 0; i < (int32_t)sizeof(md); ++i)
		sprintf(output + (i << 1), "%.2X", md[i]);

	if(upperCase)
		return std::string(output);

	return asLowerCaseString(std::string(output));
}

std::string transformToSHA256(std::string plainText, bool upperCase)
{
	SHA256_CTX c;
	SHA256_Init(&c);
	SHA256_Update(&c, plainText.c_str(), plainText.length());

	uint8_t md[SHA256_DIGEST_LENGTH];
	SHA256_Final(md, &c);

	char output[(SHA256_DIGEST_LENGTH << 1) + 1];
	for(int32_t i = 0; i < (int32_t)sizeof(md); ++i)
		sprintf(output + (i << 1), "%.2X", md[i]);

	if(upperCase)
		return std::string(output);

	return asLowerCaseString(std::string(output));
}

std::string transformToSHA512(std::string plainText, bool upperCase)
{
	SHA512_CTX c;
	SHA512_Init(&c);
	SHA512_Update(&c, plainText.c_str(), plainText.length());

	uint8_t md[SHA512_DIGEST_LENGTH];
	SHA512_Final(md, &c);

	char output[(SHA512_DIGEST_LENGTH << 1) + 1];
	for(int32_t i = 0; i < (int32_t)sizeof(md); ++i)
		sprintf(output + (i << 1), "%.2X", md[i]);

	if(upperCase)
		return std::string(output);

	return asLowerCaseString(std::string(output));
}

void _encrypt(std::string& str, bool upperCase)
{
	switch(g_config.getNumber(ConfigManager::ENCRYPTION))
	{
		case ENCRYPTION_MD5:
			str = transformToMD5(str, upperCase);
			break;
		case ENCRYPTION_SHA1:
			str = transformToSHA1(str, upperCase);
			break;
		case ENCRYPTION_SHA256:
			str = transformToSHA256(str, upperCase);
			break;
		case ENCRYPTION_SHA512:
			str = transformToSHA512(str, upperCase);
			break;
		default:
		{
			if(upperCase)
				std::transform(str.begin(), str.end(), str.begin(), upchar);

			break;
		}
	}
}

bool encryptTest(std::string plain, std::string& hash)
{
	std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
	_encrypt(plain, true);
	return plain == hash;
}

bool replaceString(std::string& text, const std::string& key, const std::string& value)
{
	if(text.find(key) == std::string::npos)
		return false;

	std::string::size_type start = 0, pos = 0;
	while((start = text.find(key, pos)) != std::string::npos)
	{
		text.replace(start, key.size(), value);
		pos = start + value.size();
	}

	return true;
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t) + 1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void toUpperCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), upchar);
}

std::string asLowerCaseString(const std::string& source)
{
	std::string s = source;
	toLowerCaseString(s);
	return s;
}

std::string asUpperCaseString(const std::string& source)
{
	std::string s = source;
	toUpperCaseString(s);
	return s;
}

bool booleanString(std::string source)
{
	toLowerCaseString(source);
	return (source == "yes" || source == "true" || atoi(source.c_str()) > 0);
}

std::string ucfirst(std::string source)
{
	for(uint16_t i = 0; i < (uint16_t)source.length(); ++i)
	{
		if(source[i] != ' ')
		{
			source[i] = upchar(source[i]);
			break;
		}
	}

	return source;
}

std::string ucwords(std::string source)
{
	bool tmp = true;
	for(uint16_t i = 0; i < (uint16_t)source.length(); ++i)
	{
		if(source[i] == ' ')
			tmp = true;
		else if(tmp)
		{
			source[i] = upchar(source[i]);
			tmp = false;
		}
	}

	return source;
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atoi(nodeValue);
	xmlFree(nodeValue);
	return true;
}

bool readXMLInteger64(xmlNodePtr node, const char* tag, int64_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atoll(nodeValue);
	xmlFree(nodeValue);
	return true;
}

bool readXMLFloat(xmlNodePtr node, const char* tag, float& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atof(nodeValue);
	xmlFree(nodeValue);
	return true;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	if(!utf8ToLatin1(nodeValue, value))
		value = nodeValue;

	xmlFree(nodeValue);
	return true;
}

bool readXMLContentString(xmlNodePtr node, std::string& value)
{
	char* nodeValue = (char*)xmlNodeGetContent(node);
	if(!nodeValue)
		return false;

	if(!utf8ToLatin1(nodeValue, value))
		value = nodeValue;

	xmlFree(nodeValue);
	return true;
}

bool parseXMLContentString(xmlNodePtr node, std::string& value)
{
	bool result = false;
	std::string compareValue;
	while(node)
	{
		if(xmlStrcmp(node->name, (const xmlChar*)"text") && node->type != XML_CDATA_SECTION_NODE)
		{
			node = node->next;
			continue;
		}

		if(!readXMLContentString(node, compareValue))
		{
			node = node->next;
			continue;
		}

		trim_left(compareValue, "\r");
		trim_left(compareValue, "\n");
		trim_left(compareValue, " ");
		if(compareValue.length() > value.length())
		{
			value = compareValue;
			if(!result)
				result = true;
		}

		node = node->next;
	}

	return result;
}

std::string getLastXMLError()
{
	std::ostringstream ss;
	xmlErrorPtr lastError = xmlGetLastError();
	if(lastError->line)
		ss << "Line: " << lastError->line << ", ";

	ss << "Info: " << lastError->message << std::endl;
	return ss.str();
}

bool utf8ToLatin1(char* inText, std::string& outText)
{
	outText = "";
	if(!inText)
		return false;

	int32_t inLen = strlen(inText);
	if(!inLen)
		return false;

	int32_t outLen = inLen << 1;
	uint8_t* outBuf = new uint8_t[outLen];

	int32_t res = UTF8Toisolat1(outBuf, &outLen, (uint8_t*)inText, &inLen);
	if(res < 0)
	{
		delete[] outBuf;
		return false;
	}

	outBuf[outLen] = '\0';
	outText = (char*)outBuf;

	delete[] outBuf;
	return true;
}

bool latin1ToUtf8(char* inText, std::string& outText)
{
	outText = "";
	if(!inText)
		return false;

	int32_t inLen = strlen(inText);
	if(!inLen)
		return false;

	int32_t outLen = inLen << 1;
	uint8_t* outBuf = new uint8_t[outLen];

	int32_t res = isolat1ToUTF8(outBuf, &outLen, (uint8_t*)inText, &inLen);
	if(res < 0)
	{
		delete[] outBuf;
		return false;
	}

	outBuf[outLen] = '\0';
	outText = (char*)outBuf;

	delete[] outBuf;
	return true;
}

StringVec explodeString(const std::string& string, const std::string& separator, bool trim/* = true*/, uint16_t limit/* = 0*/)
{
	StringVec returnVector;
	size_t start = 0, end = 0;

	uint16_t i = 1;
	while((end = string.find(separator, start)) != std::string::npos)
	{
		std::string t = string.substr(start, end - start);
		if(trim)
			trimString(t);

		returnVector.push_back(t);
		start = end + separator.size();

		++i;
		if(limit > 0 && i > limit)
			break;
	}

	returnVector.push_back(string.substr(start));
	return returnVector;
}

IntegerVec vectorAtoi(StringVec stringVector)
{
	IntegerVec returnVector;
	for(StringVec::iterator it = stringVector.begin(); it != stringVector.end(); ++it)
	{
		int32_t number = atoi((*it).c_str());
		if(number || (*it) == "0")
			returnVector.push_back(number);
	}

	return returnVector;
}

bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return ((flags & flag) == flag);
}

int32_t round(float v)
{
	int32_t t = (int32_t)std::floor(v);
	if((v - t) > 0.5)
		return t + 1;

	return t;
}

uint32_t rand24b()
{
	return ((rand() << 12) ^ rand()) & 0xFFFFFF;
}

float box_muller(float m, float s)
{
	// normal random variate generator
	// mean m, standard deviation s
	float x1, x2, w, y1;
	static float y2;

	static bool useLast = false;
	if(useLast) // use value from previous call
	{
		y1 = y2;
		useLast = false;
		return (m + y1 * s);
	}

	do
	{
		double r1 = (((float)(rand()) / RAND_MAX));
		double r2 = (((float)(rand()) / RAND_MAX));

		x1 = 2.0 * r1 - 1.0;
		x2 = 2.0 * r2 - 1.0;
		w = x1 * x1 + x2 * x2;
	}
	while(w >= 1.0);
	w = sqrt((-2.0 * log(w)) / w);

	y1 = x1 * w;
	y2 = x2 * w;

	useLast = true;
	return (m + y1 * s);
}

int32_t random_range(int32_t lowestNumber, int32_t highestNumber, DistributionType_t type /*= DISTRO_UNIFORM*/)
{
	if(highestNumber == lowestNumber)
		return lowestNumber;

	if(lowestNumber > highestNumber)
		std::swap(lowestNumber, highestNumber);

	switch(type)
	{
		case DISTRO_UNIFORM:
			return (lowestNumber + ((int32_t)rand24b() % (highestNumber - lowestNumber + 1)));
		case DISTRO_NORMAL:
			return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * (float)std::min((float)1, std::max((float)0, box_muller(0.5, 0.25)))));
		default:
			break;
	}

	const float randMax = 16777216;
	return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * float(1.f - sqrt((1.f * rand24b()) / randMax))));
}

char upchar(char character)
{
	if((character >= 97 && character <= 122) || (character <= -1 && character >= -32))
		character -= 32;

	return character;
}

bool isNumber(char character)
{
	return (character >= 48 && character <= 57);
}

bool isLowercaseLetter(char character)
{
	return (character >= 97 && character <= 122);
}

bool isUppercaseLetter(char character)
{
	return (character >= 65 && character <= 90);
}

bool isPasswordCharacter(char character)
{
	return ((character >= 33 && character <= 47) || (character >= 58 && character <= 64) || (character >= 91 && character <= 96) || (character >= 123 && character <= 126));
}

bool isValidAccountName(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]))
			return false;
	}

	return true;
}

bool isValidPassword(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]) && !isPasswordCharacter(text[size]))
			return false;
	}

	return true;
}

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter/* = true*/)
{
	uint32_t textLength = text.length(), lenBeforeSpace = 1, lenBeforeQuote = 1, lenBeforeDash = 1, repeatedCharacter = 0;
	char lastChar = 32;
	if(forceUppercaseOnFirstLetter)
	{
		if(!isUppercaseLetter(text[0]))
			return false;
	}
	else if(!isLowercaseLetter(text[0]) && !isUppercaseLetter(text[0]))
		return false;

	for(uint32_t size = 1; size < textLength; size++)
	{
		if(text[size] != 32)
		{
			lenBeforeSpace++;
			if(text[size] != 39)
				lenBeforeQuote++;
			else
			{
				if(lenBeforeQuote <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeQuote = 0;
			}

			if(text[size] != 45)
				lenBeforeDash++;
			else
			{
				if(lenBeforeDash <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeDash = 0;
			}

			if(text[size] == lastChar)
			{
				repeatedCharacter++;
				if(repeatedCharacter > 2)
					return false;
			}
			else
				repeatedCharacter = 0;

			lastChar = text[size];
		}
		else
		{
			if(lenBeforeSpace <= 1 || size == textLength - 1 || text[size + 1] == 32)
				return false;

			lenBeforeSpace = lenBeforeQuote = lenBeforeDash = 0;
		}

		if(!(isLowercaseLetter(text[size]) || text[size] == 32 || text[size] == 39 || text[size] == 45
			|| (isUppercaseLetter(text[size]) && text[size - 1] == 32)))
			return false;
	}

	return true;
}

bool isNumbers(std::string text)
{
	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isNumber(text[size]))
			return false;
	}

	return true;
}

bool checkText(std::string text, std::string str)
{
	trimString(text);
	return asLowerCaseString(text) == str;
}

std::string trimString(std::string& str)
{
	str.erase(str.find_last_not_of(" ") + 1);
	return str.erase(0, str.find_first_not_of(" "));
}

std::string formatDate(time_t _time/* = 0*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	std::ostringstream s;
	if(tms)
		s << tms->tm_mday << "/" << (tms->tm_mon + 1) << "/" << (tms->tm_year + 1900) << " " << tms->tm_hour << ":" << tms->tm_min << ":" << tms->tm_sec;
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

std::string formatDateEx(time_t _time/* = 0*/, std::string format/* = "%d %b %Y, %H:%M:%S"*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	char buffer[100];
	if(tms)
		strftime(buffer, 25, format.c_str(), tms);
	else
		sprintf(buffer, "UNIX Time: %d", (int32_t)_time);

	return buffer;
}

std::string formatTime(time_t _time/* = 0*/, bool ms/* = false*/)
{
	if(!_time)
		_time = time(NULL);
	else if(ms)
		ms = false;

	const tm* tms = localtime(&_time);
	std::ostringstream s;
	if(tms)
	{
		s << tms->tm_hour << ":" << tms->tm_min << ":";
		if(tms->tm_sec < 10)
			s << "0";

		s << tms->tm_sec;
		if(ms)
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			int32_t millitm = tv.tv_usec / 1000;

			s << "."; // make it format zzz
			if(millitm < 10)
				s << "0";

			if(millitm < 100)
				s << "0";

			s << millitm;
		}
	}
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

std::string convertIPAddress(uint32_t ip)
{
	char buffer[17];
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
	return buffer;
}

bool parseIntegerVec(std::string str, IntegerVec& intVector)
{
	StringVec strVector = explodeString(str, ";");
	IntegerVec tmpIntVector;
	for(StringVec::iterator it = strVector.begin(); it != strVector.end(); ++it)
	{
		tmpIntVector = vectorAtoi(explodeString((*it), "-"));
		if(!tmpIntVector[0] && it->substr(0, 1) != "0")
			continue;

		intVector.push_back(tmpIntVector[0]);
		if(tmpIntVector.size() > 1)
		{
			while(tmpIntVector[0] < tmpIntVector[1])
				intVector.push_back(++tmpIntVector[0]);
		}
	}

	return true;
}

bool fileExists(const std::string& filename)
{
	return boost::filesystem::exists(filename);
}

uint32_t adlerChecksum(uint8_t* data, size_t length)
{
	if(length > NETWORK_MAX_SIZE || !length)
		return 0;

	const uint16_t adler = 65521;
	uint32_t a = 1, b = 0;
	while(length > 0)
	{
		size_t tmp = length > 5552 ? 5552 : length;
		length -= tmp;
		do
		{
			a += *data++;
			b += a;
		}
		while(--tmp);
		a %= adler;
		b %= adler;
	}

	return (b << 16) | a;
}

std::string getFilePath(FileType_t type, std::string name/* = ""*/)
{
	#ifdef __FILESYSTEM_HIERARCHY_STANDARD__
	std::string path = "/var/lib/tfs/";
	#else
	std::string path = "";
	#endif
	switch(type)
	{
		case FILE_TYPE_OTHER:
			path += name;
			break;
		case FILE_TYPE_LOG:
			#ifndef __FILESYSTEM_HIERARCHY_STANDARD__
			path = "logs/" + name;
			#else
			path = "/var/log/tfls/" + name;
			#endif
			break;
		case FILE_TYPE_CONFIG:
		{
			#if defined(__HOMEDIR_CONF__)
			if(fileExists("~/.tfs/" + name))
				path = "~/.tfs/" + name;
			else
			#endif
			#if defined(__FILESYSTEM_HIERARCHY_STANDARD__)
				path = "/etc/tfls/" + name;
			#else
				path = name;
			#endif
			break;
		}
		default:
			std::clog << "> ERROR: Wrong file type!" << std::endl;
			break;
	}
	return path;
}
