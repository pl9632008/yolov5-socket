#pragma once
#ifndef _XD_LOGR_H_
#define _XD_LOGR_H_

#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <deque>
#include <time.h>
#include <fstream>
#include <algorithm>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//! LOGR Level
enum ENUM_LOGR_LEVEL
{
	LOGR_LEVEL_TRACE = 0,
	LOGR_LEVEL_DEBUG,
	LOGR_LEVEL_INFO,
	LOGR_LEVEL_WARN,
	LOGR_LEVEL_ERROR,
	LOGR_LEVEL_ALARM,
	LOGR_LEVEL_FATAL,
};


/**
*  ????????: ???????????????????????????????y?????????
*  @param logrlevel: ???????????????څ????????????????????????????????
*                   LOGR_LEVEL_TRACE, LOGR_LEVEL_DEBUG, LOGR_LEVEL_INFO, LOGR_LEVEL_WARN, LOGR_LEVEL_ERROR, LOGR_LEVEL_ALARM, LOGR_LEVEL_FATAL
*  @param savepath: ???????????????????????????????MeterRecognition.logr
*
*  @return: ??
*
**/
void LogrInit(ENUM_LOGR_LEVEL logrlevel, const char* savepath);

/** ??????????
LOGRFMTT("format input *** %s *** %d ***", "LOGRFMTT", 123456);
LOGRFMTD("format input *** %s *** %d ***", "LOGRFMTD", 123456);
LOGRFMTI("format input *** %s *** %d ***", "LOGRFMTI", 123456);
LOGRFMTW("format input *** %s *** %d ***", "LOGRFMTW", 123456);
LOGRFMTE("format input *** %s *** %d ***", "LOGRFMTE", 123456);
LOGRFMTA("format input *** %s *** %d ***", "LOGRFMTA", 123456);
LOGRFMTF("format input *** %s *** %d ***", "LOGRFMTF", 123456);

int value = 10;
string str = "test";
LOGRT("stream input *** " << "LOGRT LOGRT LOGRT LOGRT" << " *** " << value);
LOGRD("stream input *** " << "LOGRD LOGRD LOGRD LOGRD" << " *** " << str);
LOGRI("stream input *** " << "LOGRI LOGRI LOGRI LOGRI" << " *** ");
LOGRW("stream input *** " << "LOGRW LOGRW LOGRW LOGRW" << " *** ");
LOGRE("stream input *** " << "LOGRE LOGRE LOGRE LOGRE" << " *** ");
LOGRA("stream input *** " << "LOGRA LOGRA LOGRA LOGRA" << " *** ");
LOGRF("stream input *** " << "LOGRF LOGRF LOGRF LOGRF" << " *** ");
**/

//! logrger ID type. DO NOT TOUCH
typedef int LogrgerId;

//! the invalid logrger id. DO NOT TOUCH
const int LOGR4Z_INVALID_LOGRGER_ID = -1;

//! the main logrger id. DO NOT TOUCH
//! can use this id to set the main logrger's attribute.
//! example:
//! ILogr4zManager::getPtr()->setLogrgerLevel(LOGR4Z_MAIN_LOGRGER_ID, LOGR_LEVEL_WARN);
//! ILogr4zManager::getPtr()->setLogrgerDisplay(LOGR4Z_MAIN_LOGRGER_ID, false);
const int LOGR4Z_MAIN_LOGRGER_ID = 0;

//! the main logrger name. DO NOT TOUCH
const char*const LOGR4Z_MAIN_LOGRGER_KEY = "Main";

//! check VC VERSION. DO NOT TOUCH
//! format micro cannot support VC6 or VS2003, please use stream input logr, like LOGRI, LOGRD, LOGR_DEBUG, LOGR_STREAM ...
#if _MSC_VER >= 1400 //MSVC >= VS2005
#define LOGR4Z_FORMAT_INPUT_ENABLE
#endif

#ifndef WIN32
#define LOGR4Z_FORMAT_INPUT_ENABLE
#endif


//////////////////////////////////////////////////////////////////////////
//! -----------------default logrger config, can change on this.-----------
//////////////////////////////////////////////////////////////////////////
//! the max logrger count.
const int LOGR4Z_LOGRGER_MAX = 20;
//! the max logr content length.
const int LOGR4Z_LOGR_BUF_SIZE = 1024 * 8;
//! the max stl container depth.
const int LOGR4Z_LOGR_CONTAINER_DEPTH = 5;

//! all logrger synchronous output or not
const bool LOGR4Z_ALL_SYNCHRONOUS_OUTPUT = true;
//! all logrger synchronous display to the windows debug output
const bool LOGR4Z_ALL_DEBUGOUTPUT_DISPLAY = true;

//! default logrger output file.
const char* const LOGR4Z_DEFAULT_PATH = "./logr/";
//! default logr filter level
const int LOGR4Z_DEFAULT_LEVEL = LOGR_LEVEL_DEBUG;
//! default logrger display
const bool LOGR4Z_DEFAULT_DISPLAY = true;
//! default logrger output to file
const bool LOGR4Z_DEFAULT_OUTFILE = true;
//! default logrger month dir used status
const bool LOGR4Z_DEFAULT_MONTHDIR = false;
//! default logrger output file limit size, unit M byte.
const int LOGR4Z_DEFAULT_LIMITSIZE = 100;
//! default logrger show suffix (file name and line number) 
const bool LOGR4Z_DEFAULT_SHOWSUFFIX = true;
//! support ANSI->OEM console conversion on Windows
#undef LOGR4Z_OEM_CONSOLE
///////////////////////////////////////////////////////////////////////////
//! -----------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////

#ifndef _ZSUMMER_E_BEGIN
#define _ZSUMMER_E_BEGIN namespace zsummer_e {
#endif  
#ifndef _ZSUMMER_E_LOGR4Z_BEGIN
#define _ZSUMMER_E_LOGR4Z_BEGIN namespace logr4z {
#endif
_ZSUMMER_E_BEGIN
_ZSUMMER_E_LOGR4Z_BEGIN


struct LogrData
{
	LogrgerId _id;        //dest logrger id
	int    _type;     //type.
	int    _typeval;
	int    _level;    //logr level
	time_t _time;        //create time
	unsigned int _precise; //create time 
	int _contentLen;
	char _content[LOGR4Z_LOGR_BUF_SIZE]; //content
};

//! logr4z class
class ILogr4zManager
{
public:
	ILogr4zManager() {};
	virtual ~ILogr4zManager() {};

	//! Logr4z Singleton

	static ILogr4zManager * getInstance();
	inline static ILogr4zManager & getRef() { return *getInstance(); }
	inline static ILogr4zManager * getPtr() { return getInstance(); }

	//! Config or overwrite configure
	//! Needs to be called before ILogr4zManager::Start,, OR Do not call.
	virtual bool config(const char * configPath) = 0;
	virtual bool configFromString(const char * configContent) = 0;

	//! Create or overwrite logrger.
	//! Needs to be called before ILogr4zManager::Start, OR Do not call.
	virtual LogrgerId createLogrger(const char* key) = 0;

	//! Start Logr Thread. This method can only be called once by one process.
	virtual bool start() = 0;

	//! Default the method will be calling at process exit auto.
	//! Default no need to call and no recommended.
	virtual bool stop() = 0;

	//! Find logrger. thread safe.
	virtual LogrgerId findLogrger(const char* key) = 0;

	//pre-check the logr filter. if filter out return false. 
	virtual bool prePushLogr(LogrgerId id, int level) = 0;
	//! Push logr, thread safe.
	virtual bool pushLogr(LogrData * pLogr, const char * file = NULL, int line = 0) = 0;

	//! set logrger's attribute, thread safe.
	virtual bool enableLogrger(LogrgerId id, bool enable) = 0; // immediately when enable, and queue up when disable. 
	virtual bool setLogrgerName(LogrgerId id, const char * name) = 0;
	virtual bool setLogrgerPath(LogrgerId id, const char * path) = 0;
	virtual bool setLogrgerLevel(LogrgerId id, int nLevel) = 0; // immediately when enable, and queue up when disable. 
	virtual bool setLogrgerFileLine(LogrgerId id, bool enable) = 0;
	virtual bool setLogrgerDisplay(LogrgerId id, bool enable) = 0;
	virtual bool setLogrgerOutFile(LogrgerId id, bool enable) = 0;
	virtual bool setLogrgerLimitsize(LogrgerId id, unsigned int limitsize) = 0;
	virtual bool setLogrgerMonthdir(LogrgerId id, bool enable) = 0;


	//! Update logrger's attribute from config file, thread safe.
	virtual bool setAutoUpdate(int interval/*per second, 0 is disable auto update*/) = 0;
	virtual bool updateConfig() = 0;

	//! Logr4z status statistics, thread safe.
	virtual bool isLogrgerEnable(LogrgerId id) = 0;
	virtual unsigned long long getStatusTotalWriteCount() = 0;
	virtual unsigned long long getStatusTotalWriteBytes() = 0;
	virtual unsigned long long getStatusWaitingCount() = 0;
	virtual unsigned int getStatusActiveLogrgers() = 0;

	virtual LogrData * makeLogrData(LogrgerId id, int level) = 0;
	virtual void freeLogrData(LogrData * logr) = 0;
};

class Logr4zStream;
class Logr4zBinary;

#ifndef _ZSUMMER_E_END
#define _ZSUMMER_E_END }
#endif  
#ifndef _ZSUMMER_E_LOGR4Z_END
#define _ZSUMMER_E_LOGR4Z_END }
#endif

_ZSUMMER_E_LOGR4Z_END
_ZSUMMER_E_END



//! base macro.
#define LOGR_STREAM(id, level, file, line, logr) \
do{\
    if (zsummer_e::logr4z::ILogr4zManager::getPtr()->prePushLogr(id,level)) \
    {\
        zsummer_e::logr4z::LogrData * pLogr = zsummer_e::logr4z::ILogr4zManager::getPtr()->makeLogrData(id, level); \
        zsummer_e::logr4z::Logr4zStream ss(pLogr->_content + pLogr->_contentLen, LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen);\
        ss << logr;\
        pLogr->_contentLen += ss.getCurrentLen(); \
        zsummer_e::logr4z::ILogr4zManager::getPtr()->pushLogr(pLogr, file, line);\
    }\
} while (0)


//! fast macro
#define LOG_TRACE(id, logr) LOGR_STREAM(id, LOGR_LEVEL_TRACE, __FILE__, __LINE__, logr)
#define LOG_DEBUG(id, logr) LOGR_STREAM(id, LOGR_LEVEL_DEBUG, __FILE__, __LINE__, logr)
#define LOG_INFO(id, logr)  LOGR_STREAM(id, LOGR_LEVEL_INFO, __FILE__, __LINE__, logr)
#define LOG_WARN(id, logr)  LOGR_STREAM(id, LOGR_LEVEL_WARN, __FILE__, __LINE__, logr)
#define LOG_ERROR(id, logr) LOGR_STREAM(id, LOGR_LEVEL_ERROR, __FILE__, __LINE__, logr)
#define LOG_ALARM(id, logr) LOGR_STREAM(id, LOGR_LEVEL_ALARM, __FILE__, __LINE__, logr)
#define LOG_FATAL(id, logr) LOGR_STREAM(id, LOGR_LEVEL_FATAL, __FILE__, __LINE__, logr)

//! super macro.
#define LOGT( logr ) LOG_TRACE(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGD( logr ) LOG_DEBUG(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGI( logr ) LOG_INFO(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGW( logr ) LOG_WARN(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGE( logr ) LOG_ERROR(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGA( logr ) LOG_ALARM(LOGR4Z_MAIN_LOGRGER_ID, logr )
#define LOGF( logr ) LOG_FATAL(LOGR4Z_MAIN_LOGRGER_ID, logr )


//! format input logr.
#ifdef LOGR4Z_FORMAT_INPUT_ENABLE
#ifdef WIN32
#define LOGR_FORMAT(id, level, file, line, logrformat, ...)   \
do{ \
    if (zsummer_e::logr4z::ILogr4zManager::getPtr()->prePushLogr(id,level)) \
    { \
        zsummer_e::logr4z::LogrData * pLogr = zsummer_e::logr4z::ILogr4zManager::getPtr()->makeLogrData(id, level); \
        int len = _snprintf_s(pLogr->_content + pLogr->_contentLen, LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen, _TRUNCATE, logrformat, ##__VA_ARGS__); \
        if (len < 0) len = LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen; \
        pLogr->_contentLen += len; \
        zsummer_e::logr4z::ILogr4zManager::getPtr()->pushLogr(pLogr, file, line); \
    } \
} while (0)
#else
#define LOGR_FORMAT(id, level, file, line, logrformat, ...) \
do{ \
    if (zsummer_e::logr4z::ILogr4zManager::getPtr()->prePushLogr(id,level)) \
    {\
        zsummer_e::logr4z::LogrData * pLogr = zsummer_e::logr4z::ILogr4zManager::getPtr()->makeLogrData(id, level); \
        int len = snprintf(pLogr->_content + pLogr->_contentLen, LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen,logrformat, ##__VA_ARGS__); \
        if (len < 0) len = 0; \
        if (len > LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen) len = LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen; \
        pLogr->_contentLen += len; \
        zsummer_e::logr4z::ILogr4zManager::getPtr()->pushLogr(pLogr, file, line); \
    } \
}while(0)
#endif
//!format string
#define LOGRFMT_TRACE(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_DEBUG(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_INFO(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_WARN(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_ERROR(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_ALARM(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_ALARM, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMT_FATAL(id, fmt, ...)  LOGR_FORMAT(id, LOGR_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGRFMTT( fmt, ...) LOGRFMT_TRACE(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTD( fmt, ...) LOGRFMT_DEBUG(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTI( fmt, ...) LOGRFMT_INFO(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTW( fmt, ...) LOGRFMT_WARN(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTE( fmt, ...) LOGRFMT_ERROR(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTA( fmt, ...) LOGRFMT_ALARM(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#define LOGRFMTF( fmt, ...) LOGRFMT_FATAL(LOGR4Z_MAIN_LOGRGER_ID, fmt,  ##__VA_ARGS__)
#else
	inline void empty_logr_format_function1(LogrgerId id, const char*, ...) {}
inline void empty_logr_format_function2(const char*, ...) {}
#define LOGRFMT_TRACE empty_logr_format_function1
#define LOGRFMT_DEBUG LOGRFMT_TRACE
#define LOGRFMT_INFO LOGRFMT_TRACE
#define LOGRFMT_WARN LOGRFMT_TRACE
#define LOGRFMT_ERROR LOGRFMT_TRACE
#define LOGRFMT_ALARM LOGRFMT_TRACE
#define LOGRFMT_FATAL LOGRFMT_TRACE
#define LOGRFMTT empty_logr_format_function2
#define LOGRFMTD LOGRFMTT
#define LOGRFMTI LOGRFMTT
#define LOGRFMTW LOGRFMTT
#define LOGRFMTE LOGRFMTT
#define LOGRFMTA LOGRFMTT
#define LOGRFMTF LOGRFMTT
#endif


_ZSUMMER_E_BEGIN
_ZSUMMER_E_LOGR4Z_BEGIN

//! optimze from std::stringstream to Logr4zStream
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
class Logr4zBinary
{
public:
	Logr4zBinary(const char * buf, int len)
	{
		_buf = buf;
		_len = len;
	}
	const char * _buf;
	int  _len;
};
class Logr4zStream
{
public:
	inline Logr4zStream(char * buf, int len);
	inline int getCurrentLen() { return (int)(_cur - _begin); }
private:
	template<class T>
	inline Logr4zStream & writeData(const char * ft, T t);
	inline Logr4zStream & writeLongLong(long long t);
	inline Logr4zStream & writeULongLong(unsigned long long t);
	inline Logr4zStream & writePointer(const void * t);
	inline Logr4zStream & writeString(const char * t, size_t len);
	inline Logr4zStream & writeWString(const wchar_t* t);
	inline Logr4zStream & writeBinary(const Logr4zBinary & t);
public:
	inline Logr4zStream & operator <<(const void * t) { return  writePointer(t); }

	inline Logr4zStream & operator <<(const char * t) { return writeString(t, strlen(t)); }
#ifdef WIN32
	inline Logr4zStream & operator <<(const wchar_t * t) { return writeWString(t); }
#endif
	inline Logr4zStream & operator <<(bool t) { return (t ? writeData("%s", "true") : writeData("%s", "false")); }

	inline Logr4zStream & operator <<(char t) { return writeData("%c", t); }

	inline Logr4zStream & operator <<(unsigned char t) { return writeData("%u", (unsigned int)t); }

	inline Logr4zStream & operator <<(short t) { return writeData("%d", (int)t); }

	inline Logr4zStream & operator <<(unsigned short t) { return writeData("%u", (unsigned int)t); }

	inline Logr4zStream & operator <<(int t) { return writeData("%d", t); }

	inline Logr4zStream & operator <<(unsigned int t) { return writeData("%u", t); }

	inline Logr4zStream & operator <<(long t) { return writeLongLong(t); }

	inline Logr4zStream & operator <<(unsigned long t) { return writeULongLong(t); }

	inline Logr4zStream & operator <<(long long t) { return writeLongLong(t); }

	inline Logr4zStream & operator <<(unsigned long long t) { return writeULongLong(t); }

	inline Logr4zStream & operator <<(float t) { return writeData("%.4f", t); }

	inline Logr4zStream & operator <<(double t) { return writeData("%.4lf", t); }

	template<class _Elem, class _Traits, class _Alloc> //support std::string, std::wstring
	inline Logr4zStream & operator <<(const std::basic_string<_Elem, _Traits, _Alloc> & t) { return writeString(t.c_str(), t.length()); }

	inline Logr4zStream & operator << (const zsummer_e::logr4z::Logr4zBinary & binary) { return writeBinary(binary); }

	template<class _Ty1, class _Ty2>
	inline Logr4zStream & operator <<(const std::pair<_Ty1, _Ty2> & t) { return *this << "pair(" << t.first << ":" << t.second << ")"; }

	template<class _Elem, class _Alloc>
	inline Logr4zStream & operator <<(const std::vector<_Elem, _Alloc> & t)
	{
		*this << "vector(" << t.size() << ")[";
		int inputCount = 0;
		for (typename std::vector<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
		{
			inputCount++;
			if (inputCount > LOGR4Z_LOGR_CONTAINER_DEPTH)
			{
				*this << "..., ";
				break;
			}
			*this << *iter << ", ";
		}
		if (!t.empty())
		{
			_cur -= 2;
		}
		return *this << "]";
	}
	template<class _Elem, class _Alloc>
	inline Logr4zStream & operator <<(const std::list<_Elem, _Alloc> & t)
	{
		*this << "list(" << t.size() << ")[";
		int inputCount = 0;
		for (typename std::list<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
		{
			inputCount++;
			if (inputCount > LOGR4Z_LOGR_CONTAINER_DEPTH)
			{
				*this << "..., ";
				break;
			}
			*this << *iter << ", ";
		}
		if (!t.empty())
		{
			_cur -= 2;
		}
		return *this << "]";
	}
	template<class _Elem, class _Alloc>
	inline Logr4zStream & operator <<(const std::deque<_Elem, _Alloc> & t)
	{
		*this << "deque(" << t.size() << ")[";
		int inputCount = 0;
		for (typename std::deque<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
		{
			inputCount++;
			if (inputCount > LOGR4Z_LOGR_CONTAINER_DEPTH)
			{
				*this << "..., ";
				break;
			}
			*this << *iter << ", ";
		}
		if (!t.empty())
		{
			_cur -= 2;
		}
		return *this << "]";
	}
	template<class _Elem, class _Alloc>
	inline Logr4zStream & operator <<(const std::queue<_Elem, _Alloc> & t)
	{
		*this << "queue(" << t.size() << ")[";
		int inputCount = 0;
		for (typename std::queue<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
		{
			inputCount++;
			if (inputCount > LOGR4Z_LOGR_CONTAINER_DEPTH)
			{
				*this << "..., ";
				break;
			}
			*this << *iter << ", ";
		}
		if (!t.empty())
		{
			_cur -= 2;
		}
		return *this << "]";
	}
	template<class _K, class _V, class _Pr, class _Alloc>
	inline Logr4zStream & operator <<(const std::map<_K, _V, _Pr, _Alloc> & t)
	{
		*this << "map(" << t.size() << ")[";
		int inputCount = 0;
		for (typename std::map < _K, _V, _Pr, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
		{
			inputCount++;
			if (inputCount > LOGR4Z_LOGR_CONTAINER_DEPTH)
			{
				*this << "..., ";
				break;
			}
			*this << *iter << ", ";
		}
		if (!t.empty())
		{
			_cur -= 2;
		}
		return *this << "]";
	}

private:
	Logr4zStream() {}
	Logr4zStream(Logr4zStream &) {}
	char *  _begin;
	char *  _end;
	char *  _cur;
};

inline Logr4zStream::Logr4zStream(char * buf, int len)
{
	_begin = buf;
	_end = buf + len;
	_cur = _begin;
}

template<class T>
inline Logr4zStream& Logr4zStream::writeData(const char * ft, T t)
{
	if (_cur < _end)
	{
		int len = 0;
		int count = (int)(_end - _cur);
#ifdef WIN32
		len = _snprintf(_cur, count, ft, t);
		if (len == count || len < 0)
		{
			len = count;
			*(_end - 1) = '\0';
		}
#else
		len = snprintf(_cur, count, ft, t);
		if (len < 0)
		{
			*_cur = '\0';
			len = 0;
		}
		else if (len >= count)
		{
			len = count;
			*(_end - 1) = '\0';
		}
#endif
		_cur += len;
	}
	return *this;
}

inline Logr4zStream & Logr4zStream::writeLongLong(long long t)
{
#ifdef WIN32  
	writeData("%I64d", t);
#else
	writeData("%lld", t);
#endif
	return *this;
}

inline Logr4zStream & Logr4zStream::writeULongLong(unsigned long long t)
{
#ifdef WIN32  
	writeData("%I64u", t);
#else
	writeData("%llu", t);
#endif
	return *this;
}

inline Logr4zStream & Logr4zStream::writePointer(const void * t)
{
#ifdef WIN32
	sizeof(t) == 8 ? writeData("%016I64x", (unsigned long long)t) : writeData("%08I64x", (unsigned long long)t);
#else
	sizeof(t) == 8 ? writeData("%016llx", (unsigned long long)t) : writeData("%08llx", (unsigned long long)t);
#endif
	return *this;
}

inline Logr4zStream & Logr4zStream::writeBinary(const Logr4zBinary & t)
{
	writeData("%s", "\r\n\t[");
	for (int i = 0; i < t._len; i++)
	{
		if (i % 16 == 0)
		{
			writeData("%s", "\r\n\t");
			*this << (void*)(t._buf + i);
			writeData("%s", ": ");
		}
		writeData("%02x ", (unsigned char)t._buf[i]);
	}
	writeData("%s", "\r\n\t]\r\n\t");
	return *this;
}
inline Logr4zStream & zsummer_e::logr4z::Logr4zStream::writeString(const char * t, size_t len)
{
	if (_cur < _end)
	{
		size_t count = (size_t)(_end - _cur);
		if (len > count)
		{
			len = count;
		}
		memcpy(_cur, t, len);
		_cur += len;
		if (_cur >= _end - 1)
		{
			*(_end - 1) = '\0';
		}
		else
		{
			*(_cur + 1) = '\0';
		}
	}
	return *this;
}
inline zsummer_e::logr4z::Logr4zStream & zsummer_e::logr4z::Logr4zStream::writeWString(const wchar_t* t)
{
#ifdef WIN32
	DWORD dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, NULL, 0, NULL, NULL);
	if (dwLen < LOGR4Z_LOGR_BUF_SIZE)
	{
		std::string str;
		str.resize(dwLen, '\0');
		dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, &str[0], dwLen, NULL, NULL);
		if (dwLen > 0)
		{
			writeData("%s", str.c_str());
		}
	}
#else
	//not support
#endif
	return *this;
}



#ifdef WIN32
#pragma warning(pop)
#endif

_ZSUMMER_E_LOGR4Z_END
_ZSUMMER_E_END

#endif
