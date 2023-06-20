#include "Logr.h"

using namespace zsummer_e::logr4z;

void LogrInit(ENUM_LOGR_LEVEL logrlevel, const char* savepath)
{
	ILogr4zManager::getRef().setLogrgerDisplay(LOGR4Z_MAIN_LOGRGER_ID, true);
	ILogr4zManager::getRef().setLogrgerLevel(LOGR4Z_MAIN_LOGRGER_ID, logrlevel);
	ILogr4zManager::getRef().setLogrgerFileLine(LOGR4Z_MAIN_LOGRGER_ID, true);
	ILogr4zManager::getRef().setLogrgerOutFile(LOGR4Z_MAIN_LOGRGER_ID, true);
	ILogr4zManager::getRef().setLogrgerPath(LOGR4Z_MAIN_LOGRGER_ID, savepath);
	ILogr4zManager::getRef().setAutoUpdate(10);
	ILogr4zManager::getRef().start();
	return;
}

#ifdef _WIN32
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define LOGR4Z_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif



_ZSUMMER_E_BEGIN
_ZSUMMER_E_LOGR4Z_BEGIN

static const char *const LOGR_STRING[] =
{
	"LOG_TRACE",
	"LOG_DEBUG",
	"LOG_INFO ",
	"LOG_WARN ",
	"LOG_ERROR",
	"LOG_ALARM",
	"LOG_FATAL",
};

#ifdef WIN32
const static WORD LOGR_COLOR[LOGR_LEVEL_FATAL + 1] = {
	0,
	0,
	FOREGROUND_BLUE | FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_RED,
	FOREGROUND_RED,
	FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_BLUE };
#else

const static char LOGR_COLOR[LOGR_LEVEL_FATAL + 1][50] = {
	"\e[0m",
	"\e[0m",
	"\e[34m\e[1m",//hight blue
	"\e[33m", //yellow
	"\e[31m", //red
	"\e[32m", //green
	"\e[35m" };
#endif

//////////////////////////////////////////////////////////////////////////
//! Logr4zFileHandler
//////////////////////////////////////////////////////////////////////////
class Logr4zFileHandler
{
public:
	Logr4zFileHandler() { _file = NULL; }
	~Logr4zFileHandler() { close(); }
	inline bool isOpen() { return _file != NULL; }
	inline bool open(const char *path, const char * mod)
	{
		if (_file != NULL) { fclose(_file); _file = NULL; }
		_file = fopen(path, mod);
		return _file != NULL;
	}
	inline void close()
	{
		if (_file != NULL) { fclose(_file); _file = NULL; }
	}
	inline void write(const char * data, size_t len)
	{
		if (_file && len > 0)
		{
			if (fwrite(data, 1, len, _file) != len)
			{
				close();
			}
		}
	}
	inline void flush() { if (_file) fflush(_file); }

	inline std::string readLine()
	{
		char buf[500] = { 0 };
		if (_file && fgets(buf, 500, _file) != NULL)
		{
			return std::string(buf);
		}
		return std::string();
	}
	inline const std::string readContent();
public:
	FILE *_file;
};


//////////////////////////////////////////////////////////////////////////
//! UTILITY
//////////////////////////////////////////////////////////////////////////
static void sleepMillisecond(unsigned int ms);
static tm timeToTm(time_t t);
static bool isSameDay(time_t t1, time_t t2);

static void fixPath(std::string &path);
static void trimLogrConfig(std::string &str, std::string extIgnore = std::string());
static std::pair<std::string, std::string> splitPairString(const std::string & str, const std::string & delimiter);


static bool isDirectory(std::string path);
static bool createRecursionDir(std::string path);
static std::string getProcessID();
static std::string getProcessName();



//////////////////////////////////////////////////////////////////////////
//! LockHelper
//////////////////////////////////////////////////////////////////////////
class LockHelper
{
public:
	LockHelper();
	virtual ~LockHelper();

public:
	void lock();
	void unLock();
private:
#ifdef WIN32
	CRITICAL_SECTION _crit;
#else
	pthread_mutex_t  _crit;
#endif
};

//////////////////////////////////////////////////////////////////////////
//! AutoLock
//////////////////////////////////////////////////////////////////////////
class AutoLock
{
public:
	explicit AutoLock(LockHelper & lk) :_lock(lk) { _lock.lock(); }
	~AutoLock() { _lock.unLock(); }
private:
	LockHelper & _lock;
};

//////////////////////////////////////////////////////////////////////////
//! SemHelper
//////////////////////////////////////////////////////////////////////////
class SemHelper
{
public:
	SemHelper();
	virtual ~SemHelper();
public:
	bool create(int initcount);
	bool wait(int timeout = 0);
	bool post();
private:
#ifdef WIN32
	HANDLE _hSem;
#elif defined(__APPLE__)
	dispatch_semaphore_t _semid;
#else
	sem_t _semid;
	bool  _isCreate;
#endif

};



//////////////////////////////////////////////////////////////////////////
//! ThreadHelper
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
static unsigned int WINAPI  threadProc(LPVOID lpParam);
#else
static void * threadProc(void * pParam);
#endif

class ThreadHelper
{
public:
	ThreadHelper() { _hThreadID = 0; }
	virtual ~ThreadHelper() {}
public:
	bool start();
	bool wait();
	virtual void run() = 0;
private:
	unsigned long long _hThreadID;
#ifndef WIN32
	pthread_t _phtreadID;
#endif
};

#ifdef WIN32
unsigned int WINAPI  threadProc(LPVOID lpParam)
{
	ThreadHelper * p = (ThreadHelper *)lpParam;
	p->run();
	return 0;
}
#else
void * threadProc(void * pParam)
{
	ThreadHelper * p = (ThreadHelper *)pParam;
	p->run();
	return NULL;
}
#endif


//////////////////////////////////////////////////////////////////////////
//! LogrData
//////////////////////////////////////////////////////////////////////////
enum LogrDataType
{
	LDT_GENERAL,
	LDT_ENABLE_LOGRGER,
	LDT_SET_LOGRGER_NAME,
	LDT_SET_LOGRGER_PATH,
	LDT_SET_LOGRGER_LEVEL,
	LDT_SET_LOGRGER_FILELINE,
	LDT_SET_LOGRGER_DISPLAY,
	LDT_SET_LOGRGER_OUTFILE,
	LDT_SET_LOGRGER_LIMITSIZE,
	LDT_SET_LOGRGER_MONTHDIR,
	//    LDT_SET_LOGRGER_,
};


//////////////////////////////////////////////////////////////////////////
//! LogrgerInfo
//////////////////////////////////////////////////////////////////////////
struct LogrgerInfo
{
	//! attribute
	std::string _key;   //logrger key
	std::string _name;    // one logrger one name.
	std::string _path;    //path for logr file.
	int  _level;        //filter level
	bool _display;        //display to screen 
	bool _outfile;        //output to file
	bool _monthdir;        //create directory per month 
	unsigned int _limitsize; //limit file's size, unit Million byte.
	bool _enable;        //logrger is enable 
	bool _fileLine;        //enable/disable the logr's suffix.(file name:line number)

						   //! runtime info
	time_t _curFileCreateTime;    //file create time
	unsigned int _curFileIndex; //rolling file index
	unsigned int _curWriteLen;  //current file length
	Logr4zFileHandler    _handle;        //file handle.


	LogrgerInfo()
	{
		_enable = false;
		_path = LOGR4Z_DEFAULT_PATH;
		_level = LOGR4Z_DEFAULT_LEVEL;
		_display = LOGR4Z_DEFAULT_DISPLAY;
		_outfile = LOGR4Z_DEFAULT_OUTFILE;

		_monthdir = LOGR4Z_DEFAULT_MONTHDIR;
		_limitsize = LOGR4Z_DEFAULT_LIMITSIZE;
		_fileLine = LOGR4Z_DEFAULT_SHOWSUFFIX;

		_curFileCreateTime = 0;
		_curFileIndex = 0;
		_curWriteLen = 0;
	}
};


//////////////////////////////////////////////////////////////////////////
//! LogrerManager
//////////////////////////////////////////////////////////////////////////
class LogrerManager : public ThreadHelper, public ILogr4zManager
{
public:
	LogrerManager();
	virtual ~LogrerManager();

	bool configFromStringImpl(std::string content, bool isUpdate);
	//! \B6\C1?\C5\E4\D6\C3\CE?\FE\B2\A2\B8\B2??
	virtual bool config(const char* configPath);
	virtual bool configFromString(const char* configContent);

	//! \B8\B2???\B4\B4\BD\A8
	virtual LogrgerId createLogrger(const char* key);
	virtual bool start();
	virtual bool stop();
	virtual bool prePushLogr(LogrgerId id, int level);
	virtual bool pushLogr(LogrData * pLogr, const char * file, int line);
	//! \B2\E9\D5\D2ID
	virtual LogrgerId findLogrger(const char*  key);
	bool hotChange(LogrgerId id, LogrDataType ldt, int num, const std::string & text);
	virtual bool enableLogrger(LogrgerId id, bool enable);
	virtual bool setLogrgerName(LogrgerId id, const char * name);
	virtual bool setLogrgerPath(LogrgerId id, const char * path);
	virtual bool setLogrgerLevel(LogrgerId id, int nLevel);
	virtual bool setLogrgerFileLine(LogrgerId id, bool enable);
	virtual bool setLogrgerDisplay(LogrgerId id, bool enable);
	virtual bool setLogrgerOutFile(LogrgerId id, bool enable);
	virtual bool setLogrgerLimitsize(LogrgerId id, unsigned int limitsize);
	virtual bool setLogrgerMonthdir(LogrgerId id, bool enable);

	virtual bool setAutoUpdate(int interval);
	virtual bool updateConfig();
	virtual bool isLogrgerEnable(LogrgerId id);
	virtual unsigned long long getStatusTotalWriteCount() { return _ullStatusTotalWriteFileCount; }
	virtual unsigned long long getStatusTotalWriteBytes() { return _ullStatusTotalWriteFileBytes; }
	virtual unsigned long long getStatusWaitingCount() { return _ullStatusTotalPushLogr - _ullStatusTotalPopLogr; }
	virtual unsigned int getStatusActiveLogrgers();
protected:
	virtual LogrData * makeLogrData(LogrgerId id, int level);
	virtual void freeLogrData(LogrData * logr);
	void showColorText(const char *text, int level = LOGR_LEVEL_DEBUG);
	bool onHotChange(LogrgerId id, LogrDataType ldt, int num, const std::string & text);
	bool openLogrger(LogrData * logr);
	bool closeLogrger(LogrgerId id);
	bool popLogr(LogrData *& logr);
	virtual void run();
private:

	//! thread status.
	bool        _runing;
	//! wait thread started.
	SemHelper        _semaphore;

	//! hot change name or path for one logrger
	LockHelper _hotLock;
	int _hotUpdateInterval;
	unsigned int _checksum;

	//! the process info.
	std::string _pid;
	std::string _proName;

	//! config file name
	std::string _configFile;

	//! logrger id manager, [logrger name]:[logrger id].
	std::map<std::string, LogrgerId> _ids;
	// the last used id of _logrgers
	LogrgerId    _lastId;
	LogrgerInfo _logrgers[LOGR4Z_LOGRGER_MAX];

	//! logr queue
	LockHelper    _logrLock;
	std::list<LogrData *> _logrs;
	std::vector<LogrData*> _freeLogrDatas;

	//show color lock
	LockHelper _scLock;
	//status statistics
	//write file
	unsigned long long _ullStatusTotalWriteFileCount;
	unsigned long long _ullStatusTotalWriteFileBytes;

	//Logr queue statistics
	unsigned long long _ullStatusTotalPushLogr;
	unsigned long long _ullStatusTotalPopLogr;



};




//////////////////////////////////////////////////////////////////////////
//! Logr4zFileHandler
//////////////////////////////////////////////////////////////////////////

const std::string Logr4zFileHandler::readContent()
{
	std::string content;

	if (!_file)
	{
		return content;
	}
	char buf[BUFSIZ];
	size_t ret = 0;
	do
	{
		ret = fread(buf, sizeof(char), BUFSIZ, _file);
		content.append(buf, ret);
	} while (ret == BUFSIZ);

	return content;
}




//////////////////////////////////////////////////////////////////////////
//! utility
//////////////////////////////////////////////////////////////////////////


void sleepMillisecond(unsigned int ms)
{
#ifdef WIN32
	::Sleep(ms);
#else
	usleep(1000 * ms);
#endif
}

struct tm timeToTm(time_t t)
{
#ifdef WIN32
#if _MSC_VER < 1400 //VS2003
	return *localtime(&t);
#else //vs2005->vs2013->
	struct tm tt = { 0 };
	localtime_s(&tt, &t);
	return tt;
#endif
#else //linux
	struct tm tt = { 0 };
	localtime_r(&t, &tt);
	return tt;
#endif
}

bool isSameDay(time_t t1, time_t t2)
{
	tm tm1 = timeToTm(t1);
	tm tm2 = timeToTm(t2);
	if (tm1.tm_year == tm2.tm_year
		&& tm1.tm_yday == tm2.tm_yday)
	{
		return true;
	}
	return false;
}


void fixPath(std::string &path)
{
	if (path.empty()) { return; }
	for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
	{
		if (*iter == '\\') { *iter = '/'; }
	}
	if (path.at(path.length() - 1) != '/') { path.append("/"); }
}

static void trimLogrConfig(std::string &str, std::string extIgnore)
{
	if (str.empty()) { return; }
	extIgnore += "\r\n\t ";
	int length = (int)str.length();
	int posBegin = 0;
	int posEnd = 0;

	//trim utf8 file bom
	if (str.length() >= 3
		&& (unsigned char)str[0] == 0xef
		&& (unsigned char)str[1] == 0xbb
		&& (unsigned char)str[2] == 0xbf)
	{
		posBegin = 3;
	}

	//trim character 
	for (int i = posBegin; i<length; i++)
	{
		bool bCheck = false;
		for (int j = 0; j < (int)extIgnore.length(); j++)
		{
			if (str[i] == extIgnore[j])
			{
				bCheck = true;
			}
		}
		if (bCheck)
		{
			if (i == posBegin)
			{
				posBegin++;
			}
		}
		else
		{
			posEnd = i + 1;
		}
	}
	if (posBegin < posEnd)
	{
		str = str.substr(posBegin, posEnd - posBegin);
	}
	else
	{
		str.clear();
	}
}

//split
static std::pair<std::string, std::string> splitPairString(const std::string & str, const std::string & delimiter)
{
	std::string::size_type pos = str.find(delimiter.c_str());
	if (pos == std::string::npos)
	{
		return std::make_pair(str, "");
	}
	return std::make_pair(str.substr(0, pos), str.substr(pos + delimiter.length()));
}

static bool parseConfigLine(const std::string& line, int curLineNum, std::string & key, std::map<std::string, LogrgerInfo> & outInfo)
{
	std::pair<std::string, std::string> kv = splitPairString(line, "=");
	if (kv.first.empty())
	{
		return false;
	}

	trimLogrConfig(kv.first);
	trimLogrConfig(kv.second);
	if (kv.first.empty() || kv.first.at(0) == '#')
	{
		return true;
	}

	if (kv.first.at(0) == '[')
	{
		trimLogrConfig(kv.first, "[]");
		key = kv.first;
		{
			std::string tmpstr = kv.first;
			std::transform(tmpstr.begin(), tmpstr.end(), tmpstr.begin(), ::tolower);
			if (tmpstr == "main")
			{
				key = "Main";
			}
		}
		std::map<std::string, LogrgerInfo>::iterator iter = outInfo.find(key);
		if (iter == outInfo.end())
		{
			LogrgerInfo li;
			li._enable = true;
			li._key = key;
			li._name = key;
			outInfo.insert(std::make_pair(li._key, li));
		}
		else
		{
			std::cout << "logr4z configure warning: duplicate logrger key:[" << key << "] at line:" << curLineNum << std::endl;
		}
		return true;
	}
	trimLogrConfig(kv.first);
	trimLogrConfig(kv.second);
	std::map<std::string, LogrgerInfo>::iterator iter = outInfo.find(key);
	if (iter == outInfo.end())
	{
		std::cout << "logr4z configure warning: not found current logrger name:[" << key << "] at line:" << curLineNum
			<< ", key=" << kv.first << ", value=" << kv.second << std::endl;
		return true;
	}
	std::transform(kv.first.begin(), kv.first.end(), kv.first.begin(), ::tolower);
	//! path
	if (kv.first == "path")
	{
		iter->second._path = kv.second;
		return true;
	}
	else if (kv.first == "name")
	{
		iter->second._name = kv.second;
		return true;
	}
	std::transform(kv.second.begin(), kv.second.end(), kv.second.begin(), ::tolower);
	//! level
	if (kv.first == "level")
	{
		if (kv.second == "trace" || kv.second == "all")
		{
			iter->second._level = LOGR_LEVEL_TRACE;
		}
		else if (kv.second == "debug")
		{
			iter->second._level = LOGR_LEVEL_DEBUG;
		}
		else if (kv.second == "info")
		{
			iter->second._level = LOGR_LEVEL_INFO;
		}
		else if (kv.second == "warn" || kv.second == "warning")
		{
			iter->second._level = LOGR_LEVEL_WARN;
		}
		else if (kv.second == "error")
		{
			iter->second._level = LOGR_LEVEL_ERROR;
		}
		else if (kv.second == "alarm")
		{
			iter->second._level = LOGR_LEVEL_ALARM;
		}
		else if (kv.second == "fatal")
		{
			iter->second._level = LOGR_LEVEL_FATAL;
		}
	}
	//! display
	else if (kv.first == "display")
	{
		if (kv.second == "false" || kv.second == "0")
		{
			iter->second._display = false;
		}
		else
		{
			iter->second._display = true;
		}
	}
	//! output to file
	else if (kv.first == "outfile")
	{
		if (kv.second == "false" || kv.second == "0")
		{
			iter->second._outfile = false;
		}
		else
		{
			iter->second._outfile = true;
		}
	}
	//! monthdir
	else if (kv.first == "monthdir")
	{
		if (kv.second == "false" || kv.second == "0")
		{
			iter->second._monthdir = false;
		}
		else
		{
			iter->second._monthdir = true;
		}
	}
	//! limit file size
	else if (kv.first == "limitsize")
	{
		iter->second._limitsize = atoi(kv.second.c_str());
	}
	//! display logr in file line
	else if (kv.first == "fileline")
	{
		if (kv.second == "false" || kv.second == "0")
		{
			iter->second._fileLine = false;
		}
		else
		{
			iter->second._fileLine = true;
		}
	}
	//! enable/disable one logrger
	else if (kv.first == "enable")
	{
		if (kv.second == "false" || kv.second == "0")
		{
			iter->second._enable = false;
		}
		else
		{
			iter->second._enable = true;
		}
	}
	return true;
}

static bool parseConfigFromString(std::string content, std::map<std::string, LogrgerInfo> & outInfo)
{

	std::string key;
	int curLine = 1;
	std::string line;
	std::string::size_type curPos = 0;
	if (content.empty())
	{
		return true;
	}
	do
	{
		std::string::size_type pos = std::string::npos;
		for (std::string::size_type i = curPos; i < content.length(); ++i)
		{
			//support linux/unix/windows LRCF
			if (content[i] == '\r' || content[i] == '\n')
			{
				pos = i;
				break;
			}
		}
		line = content.substr(curPos, pos - curPos);
		parseConfigLine(line, curLine, key, outInfo);
		curLine++;

		if (pos == std::string::npos)
		{
			break;
		}
		else
		{
			curPos = pos + 1;
		}
	} while (1);
	return true;
}



bool isDirectory(std::string path)
{
#ifdef WIN32
	return PathIsDirectoryA(path.c_str()) ? true : false;
#else
	DIR * pdir = opendir(path.c_str());
	if (pdir == NULL)
	{
		return false;
	}
	else
	{
		closedir(pdir);
		pdir = NULL;
		return true;
	}
#endif
}



bool createRecursionDir(std::string path)
{
	if (path.length() == 0) return true;
	std::string sub;
	fixPath(path);

	std::string::size_type pos = path.find('/');
	while (pos != std::string::npos)
	{
		std::string cur = path.substr(0, pos - 0);
		if (cur.length() > 0 && !isDirectory(cur))
		{
			bool ret = false;
#ifdef WIN32
			ret = CreateDirectoryA(cur.c_str(), NULL) ? true : false;
#else
			ret = (mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
			if (!ret)
			{
				return false;
			}
		}
		pos = path.find('/', pos + 1);
	}

	return true;
}

std::string getProcessID()
{
	std::string pid = "0";
	char buf[260] = { 0 };
#ifdef WIN32
	DWORD winPID = GetCurrentProcessId();
	sprintf(buf, "%06u", winPID);
	pid = buf;
#else
	sprintf(buf, "%06d", getpid());
	pid = buf;
#endif
	return pid;
}


std::string getProcessName()
{
	std::string name = "process";
	char buf[260] = { 0 };
#ifdef WIN32
	if (GetModuleFileNameA(NULL, buf, 259) > 0)
	{
		name = buf;
	}
	std::string::size_type pos = name.rfind("\\");
	if (pos != std::string::npos)
	{
		name = name.substr(pos + 1, std::string::npos);
	}
	pos = name.rfind(".");
	if (pos != std::string::npos)
	{
		name = name.substr(0, pos - 0);
	}

#elif defined(LOGR4Z_HAVE_LIBPROC)
	proc_name(getpid(), buf, 260);
	name = buf;
	return name;;
#else
	sprintf(buf, "/proc/%d/cmdline", (int)getpid());
	Logr4zFileHandler i;
	i.open(buf, "rb");
	if (!i.isOpen())
	{
		return name;
	}
	name = i.readLine();
	i.close();

	std::string::size_type pos = name.rfind("/");
	if (pos != std::string::npos)
	{
		name = name.substr(pos + 1, std::string::npos);
	}
#endif


	return name;
}






//////////////////////////////////////////////////////////////////////////
// LockHelper
//////////////////////////////////////////////////////////////////////////
LockHelper::LockHelper()
{
#ifdef WIN32
	InitializeCriticalSection(&_crit);
#else
	//_crit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&_crit, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
}
LockHelper::~LockHelper()
{
#ifdef WIN32
	DeleteCriticalSection(&_crit);
#else
	pthread_mutex_destroy(&_crit);
#endif
}

void LockHelper::lock()
{
#ifdef WIN32
	EnterCriticalSection(&_crit);
#else
	pthread_mutex_lock(&_crit);
#endif
}
void LockHelper::unLock()
{
#ifdef WIN32
	LeaveCriticalSection(&_crit);
#else
	pthread_mutex_unlock(&_crit);
#endif
}
//////////////////////////////////////////////////////////////////////////
// SemHelper
//////////////////////////////////////////////////////////////////////////
SemHelper::SemHelper()
{
#ifdef WIN32
	_hSem = NULL;
#elif defined(__APPLE__)
	_semid = NULL;
#else
	_isCreate = false;
#endif

}
SemHelper::~SemHelper()
{
#ifdef WIN32
	if (_hSem != NULL)
	{
		CloseHandle(_hSem);
		_hSem = NULL;
	}
#elif defined(__APPLE__)
	if (_semid)
	{
		dispatch_release(_semid);
		_semid = NULL;
	}
#else
	if (_isCreate)
	{
		_isCreate = false;
		sem_destroy(&_semid);
	}
#endif

}

bool SemHelper::create(int initcount)
{
	if (initcount < 0)
	{
		initcount = 0;
	}
#ifdef WIN32
	if (initcount > 64)
	{
		return false;
	}
	_hSem = CreateSemaphore(NULL, initcount, 64, NULL);
	if (_hSem == NULL)
	{
		return false;
	}
#elif defined(__APPLE__)
	_semid = dispatch_semaphore_create(initcount);
	if (!_semid)
	{
		return false;
	}
#else
	if (sem_init(&_semid, 0, initcount) != 0)
	{
		return false;
	}
	_isCreate = true;
#endif

	return true;
}
bool SemHelper::wait(int timeout)
{
#ifdef WIN32
	if (timeout <= 0)
	{
		timeout = INFINITE;
	}
	if (WaitForSingleObject(_hSem, timeout) != WAIT_OBJECT_0)
	{
		return false;
	}
#elif defined(__APPLE__)
	if (dispatch_semaphore_wait(_semid, dispatch_time(DISPATCH_TIME_NOW, timeout * 1000)) != 0)
	{
		return false;
	}
#else
	if (timeout <= 0)
	{
		return (sem_wait(&_semid) == 0);
	}
	else
	{
		struct timeval tm;
		gettimeofday(&tm, NULL);
		long long endtime = tm.tv_sec * 1000 + tm.tv_usec / 1000 + timeout;
		do
		{
			sleepMillisecond(50);
			int ret = sem_trywait(&_semid);
			if (ret == 0)
			{
				return true;
			}
			struct timeval tv_cur;
			gettimeofday(&tv_cur, NULL);
			if (tv_cur.tv_sec * 1000 + tv_cur.tv_usec / 1000 > endtime)
			{
				return false;
			}

			if (ret == -1 && errno == EAGAIN)
			{
				continue;
			}
			else
			{
				return false;
			}
		} while (true);
		return false;
	}
#endif
	return true;
}

bool SemHelper::post()
{
#ifdef WIN32
	return ReleaseSemaphore(_hSem, 1, NULL) ? true : false;
#elif defined(__APPLE__)
	return dispatch_semaphore_signal(_semid) == 0;
#else
	return (sem_post(&_semid) == 0);
#endif

}

//////////////////////////////////////////////////////////////////////////
//! ThreadHelper
//////////////////////////////////////////////////////////////////////////
bool ThreadHelper::start()
{
#ifdef WIN32
	unsigned long long ret = _beginthreadex(NULL, 0, threadProc, (void *) this, 0, NULL);

	if (ret == -1 || ret == 0)
	{
		std::cout << "logr4z: create logr4z thread error! \r\n" << std::endl;
		return false;
	}
	_hThreadID = ret;
#else
	int ret = pthread_create(&_phtreadID, NULL, threadProc, (void*)this);
	if (ret != 0)
	{
		std::cout << "logr4z: create logr4z thread error! \r\n" << std::endl;
		return false;
	}
#endif
	return true;
}

bool ThreadHelper::wait()
{
#ifdef WIN32
	if (WaitForSingleObject((HANDLE)_hThreadID, INFINITE) != WAIT_OBJECT_0)
	{
		return false;
	}
#else
	if (pthread_join(_phtreadID, NULL) != 0)
	{
		return false;
	}
#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////
//! LogrerManager
//////////////////////////////////////////////////////////////////////////
LogrerManager::LogrerManager()
{
	_runing = false;
	_lastId = LOGR4Z_MAIN_LOGRGER_ID;
	_hotUpdateInterval = 0;

	_ullStatusTotalPushLogr = 0;
	_ullStatusTotalPopLogr = 0;
	_ullStatusTotalWriteFileCount = 0;
	_ullStatusTotalWriteFileBytes = 0;

	_pid = getProcessID();
	_proName = getProcessName();
	_logrgers[LOGR4Z_MAIN_LOGRGER_ID]._enable = true;
	_ids[LOGR4Z_MAIN_LOGRGER_KEY] = LOGR4Z_MAIN_LOGRGER_ID;
	_logrgers[LOGR4Z_MAIN_LOGRGER_ID]._key = LOGR4Z_MAIN_LOGRGER_KEY;
	_logrgers[LOGR4Z_MAIN_LOGRGER_ID]._name = LOGR4Z_MAIN_LOGRGER_KEY;

}
LogrerManager::~LogrerManager()
{
	stop();
}


LogrData * LogrerManager::makeLogrData(LogrgerId id, int level)
{
	LogrData * pLogr = NULL;
	if (true)
	{
		if (!_freeLogrDatas.empty())
		{
			AutoLock l(_logrLock);
			if (!_freeLogrDatas.empty())
			{
				pLogr = _freeLogrDatas.back();
				_freeLogrDatas.pop_back();
			}
		}
		if (pLogr == NULL)
		{
			pLogr = new LogrData();
		}
	}
	//append precise time to logr
	if (true)
	{
		pLogr->_id = id;
		pLogr->_level = level;
		pLogr->_type = LDT_GENERAL;
		pLogr->_typeval = 0;
		pLogr->_contentLen = 0;
#ifdef WIN32
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		unsigned long long now = ft.dwHighDateTime;
		now <<= 32;
		now |= ft.dwLowDateTime;
		now /= 10;
		now -= 11644473600000000ULL;
		now /= 1000;
		pLogr->_time = now / 1000;
		pLogr->_precise = (unsigned int)(now % 1000);
#else
		struct timeval tm;
		gettimeofday(&tm, NULL);
		pLogr->_time = tm.tv_sec;
		pLogr->_precise = tm.tv_usec / 1000;
#endif
	}

	//format logr
	if (true)
	{
		tm tt = timeToTm(pLogr->_time);

		pLogr->_contentLen = sprintf(pLogr->_content, "%d-%02d-%02d %02d:%02d:%02d.%03u %s ",
			tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec, pLogr->_precise,
			LOGR_STRING[pLogr->_level]);
		if (pLogr->_contentLen < 0)
		{
			pLogr->_contentLen = 0;
		}
	}
	return pLogr;
}
void LogrerManager::freeLogrData(LogrData * logr)
{

	if (_freeLogrDatas.size() < 200)
	{
		AutoLock l(_logrLock);
		_freeLogrDatas.push_back(logr);
	}
	else
	{
		delete logr;
	}
}

void LogrerManager::showColorText(const char *text, int level)
{

#if defined(WIN32) && defined(LOGR4Z_OEM_CONSOLE)
	char oem[LOGR4Z_LOGR_BUF_SIZE] = { 0 };
	CharToOemBuffA(text, oem, LOGR4Z_LOGR_BUF_SIZE);
#endif

	if (level <= LOGR_LEVEL_DEBUG || level > LOGR_LEVEL_FATAL)
	{
#if defined(WIN32) && defined(LOGR4Z_OEM_CONSOLE)
		printf("%s", oem);
#else
		printf("%s", text);
#endif
		return;
	}
#ifndef WIN32
	printf("%s%s\e[0m", LOGR_COLOR[level], text);
#else
	AutoLock l(_scLock);
	HANDLE hStd = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStd == INVALID_HANDLE_VALUE) return;
	CONSOLE_SCREEN_BUFFER_INFO oldInfo;
	if (!GetConsoleScreenBufferInfo(hStd, &oldInfo))
	{
		return;
	}
	else
	{
		SetConsoleTextAttribute(hStd, LOGR_COLOR[level]);
#ifdef LOGR4Z_OEM_CONSOLE
		printf("%s", oem);
#else
		printf("%s", text);
#endif
		SetConsoleTextAttribute(hStd, oldInfo.wAttributes);
	}
#endif
	return;
}

bool LogrerManager::configFromStringImpl(std::string content, bool isUpdate)
{
	unsigned int sum = 0;
	for (std::string::iterator iter = content.begin(); iter != content.end(); ++iter)
	{
		sum += (unsigned char)*iter;
	}
	if (sum == _checksum)
	{
		return true;
	}
	_checksum = sum;


	std::map<std::string, LogrgerInfo> logrgerMap;
	if (!parseConfigFromString(content, logrgerMap))
	{
		std::cout << " !!! !!! !!! !!!" << std::endl;
		std::cout << " !!! !!! logr4z load config file error" << std::endl;
		std::cout << " !!! !!! !!! !!!" << std::endl;
		return false;
	}
	for (std::map<std::string, LogrgerInfo>::iterator iter = logrgerMap.begin(); iter != logrgerMap.end(); ++iter)
	{
		LogrgerId id = LOGR4Z_INVALID_LOGRGER_ID;
		id = findLogrger(iter->second._key.c_str());
		if (id == LOGR4Z_INVALID_LOGRGER_ID)
		{
			if (isUpdate)
			{
				continue;
			}
			else
			{
				id = createLogrger(iter->second._key.c_str());
				if (id == LOGR4Z_INVALID_LOGRGER_ID)
				{
					continue;
				}
			}
		}
		enableLogrger(id, iter->second._enable);
		setLogrgerName(id, iter->second._name.c_str());
		setLogrgerPath(id, iter->second._path.c_str());
		setLogrgerLevel(id, iter->second._level);
		setLogrgerFileLine(id, iter->second._fileLine);
		setLogrgerDisplay(id, iter->second._display);
		setLogrgerOutFile(id, iter->second._outfile);
		setLogrgerLimitsize(id, iter->second._limitsize);
		setLogrgerMonthdir(id, iter->second._monthdir);
	}
	return true;
}

//! read configure and create with overwriting
bool LogrerManager::config(const char* configPath)
{
	if (!_configFile.empty())
	{
		std::cout << " !!! !!! !!! !!!" << std::endl;
		std::cout << " !!! !!! logr4z configure error: too many calls to Config. the old config file=" << _configFile << ", the new config file=" << configPath << " !!! !!! " << std::endl;
		std::cout << " !!! !!! !!! !!!" << std::endl;
		return false;
	}
	_configFile = configPath;

	Logr4zFileHandler f;
	f.open(_configFile.c_str(), "rb");
	if (!f.isOpen())
	{
		std::cout << " !!! !!! !!! !!!" << std::endl;
		std::cout << " !!! !!! logr4z load config file error. filename=" << configPath << " !!! !!! " << std::endl;
		std::cout << " !!! !!! !!! !!!" << std::endl;
		return false;
	}
	return configFromStringImpl(f.readContent().c_str(), false);
}

//! read configure and create with overwriting
bool LogrerManager::configFromString(const char* configContent)
{
	return configFromStringImpl(configContent, false);
}

//! create with overwriting
LogrgerId LogrerManager::createLogrger(const char* key)
{
	if (key == NULL)
	{
		return LOGR4Z_INVALID_LOGRGER_ID;
	}

	std::string copyKey = key;
	trimLogrConfig(copyKey);

	LogrgerId newID = LOGR4Z_INVALID_LOGRGER_ID;
	{
		std::map<std::string, LogrgerId>::iterator iter = _ids.find(copyKey);
		if (iter != _ids.end())
		{
			newID = iter->second;
		}
	}
	if (newID == LOGR4Z_INVALID_LOGRGER_ID)
	{
		if (_lastId + 1 >= LOGR4Z_LOGRGER_MAX)
		{
			showColorText("logr4z: CreateLogrger can not create|writeover, because logrgerid need < LOGRGER_MAX! \r\n", LOGR_LEVEL_FATAL);
			return LOGR4Z_INVALID_LOGRGER_ID;
		}
		newID = ++_lastId;
		_ids[copyKey] = newID;
		_logrgers[newID]._enable = true;
		_logrgers[newID]._key = copyKey;
		_logrgers[newID]._name = copyKey;
	}

	return newID;
}


bool LogrerManager::start()
{
	if (_runing)
	{
		showColorText("logr4z already start \r\n", LOGR_LEVEL_FATAL);
		return false;
	}
	_semaphore.create(0);
	bool ret = ThreadHelper::start();
	return ret && _semaphore.wait(3000);
}
bool LogrerManager::stop()
{
	if (_runing)
	{
		showColorText("logr4z stopping \r\n", LOGR_LEVEL_FATAL);
		_runing = false;
		wait();
		return true;
	}
	return false;
}
bool LogrerManager::prePushLogr(LogrgerId id, int level)
{
	if (id < 0 || id > _lastId || !_runing || !_logrgers[id]._enable)
	{
		return false;
	}
	if (level < _logrgers[id]._level)
	{
		return false;
	}
	return true;
}
bool LogrerManager::pushLogr(LogrData * pLogr, const char * file, int line)
{
	// discard logr
	if (pLogr->_id < 0 || pLogr->_id > _lastId || !_runing || !_logrgers[pLogr->_id]._enable)
	{
		freeLogrData(pLogr);
		return false;
	}

	//filter logr
	if (pLogr->_level < _logrgers[pLogr->_id]._level)
	{
		freeLogrData(pLogr);
		return false;
	}
	if (_logrgers[pLogr->_id]._fileLine && file)
	{
		const char * pNameBegin = file + strlen(file);
		do
		{
			if (*pNameBegin == '\\' || *pNameBegin == '/') { pNameBegin++; break; }
			if (pNameBegin == file) { break; }
			pNameBegin--;
		} while (true);
		zsummer_e::logr4z::Logr4zStream ss(pLogr->_content + pLogr->_contentLen, LOGR4Z_LOGR_BUF_SIZE - pLogr->_contentLen);
		ss << " " << pNameBegin << ":" << line;
		pLogr->_contentLen += ss.getCurrentLen();
	}
	if (pLogr->_contentLen < 3) pLogr->_contentLen = 3;
	if (pLogr->_contentLen + 3 <= LOGR4Z_LOGR_BUF_SIZE) pLogr->_contentLen += 3;

	pLogr->_content[pLogr->_contentLen - 1] = '\0';
	pLogr->_content[pLogr->_contentLen - 2] = '\n';
	pLogr->_content[pLogr->_contentLen - 3] = '\r';
	pLogr->_contentLen--; //clean '\0'


	if (_logrgers[pLogr->_id]._display && LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
	{
		showColorText(pLogr->_content, pLogr->_level);
	}

	if (LOGR4Z_ALL_DEBUGOUTPUT_DISPLAY && LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
	{
#ifdef WIN32
		OutputDebugStringA(pLogr->_content);
#endif
	}

	if (_logrgers[pLogr->_id]._outfile && LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
	{
		AutoLock l(_logrLock);
		if (openLogrger(pLogr))
		{
			_logrgers[pLogr->_id]._handle.write(pLogr->_content, pLogr->_contentLen);
			closeLogrger(pLogr->_id);
			_ullStatusTotalWriteFileCount++;
			_ullStatusTotalWriteFileBytes += pLogr->_contentLen;
		}
	}

	if (LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
	{
		freeLogrData(pLogr);
		return true;
	}

	AutoLock l(_logrLock);
	_logrs.push_back(pLogr);
	_ullStatusTotalPushLogr++;
	return true;
}

//! \B2\E9\D5\D2ID
LogrgerId LogrerManager::findLogrger(const char * key)
{
	std::map<std::string, LogrgerId>::iterator iter;
	iter = _ids.find(key);
	if (iter != _ids.end())
	{
		return iter->second;
	}
	return LOGR4Z_INVALID_LOGRGER_ID;
}

bool LogrerManager::hotChange(LogrgerId id, LogrDataType ldt, int num, const std::string & text)
{
	if (id <0 || id > _lastId) return false;
	if (text.length() >= LOGR4Z_LOGR_BUF_SIZE) return false;
	if (!_runing || LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
	{
		return onHotChange(id, ldt, num, text);
	}
	LogrData * pLogr = makeLogrData(id, LOGR4Z_DEFAULT_LEVEL);
	pLogr->_id = id;
	pLogr->_type = ldt;
	pLogr->_typeval = num;
	memcpy(pLogr->_content, text.c_str(), text.length());
	pLogr->_contentLen = (int)text.length();
	AutoLock l(_logrLock);
	_logrs.push_back(pLogr);
	return true;
}

bool LogrerManager::onHotChange(LogrgerId id, LogrDataType ldt, int num, const std::string & text)
{
	if (id < LOGR4Z_MAIN_LOGRGER_ID || id > _lastId)
	{
		return false;
	}
	LogrgerInfo & logrger = _logrgers[id];
	if (ldt == LDT_ENABLE_LOGRGER) logrger._enable = num != 0;
	else if (ldt == LDT_SET_LOGRGER_NAME) logrger._name = text;
	else if (ldt == LDT_SET_LOGRGER_PATH) logrger._path = text;
	else if (ldt == LDT_SET_LOGRGER_LEVEL) logrger._level = num;
	else if (ldt == LDT_SET_LOGRGER_FILELINE) logrger._fileLine = num != 0;
	else if (ldt == LDT_SET_LOGRGER_DISPLAY) logrger._display = num != 0;
	else if (ldt == LDT_SET_LOGRGER_OUTFILE) logrger._outfile = num != 0;
	else if (ldt == LDT_SET_LOGRGER_LIMITSIZE) logrger._limitsize = num;
	else if (ldt == LDT_SET_LOGRGER_MONTHDIR) logrger._monthdir = num != 0;
	return true;
}

bool LogrerManager::enableLogrger(LogrgerId id, bool enable)
{
	if (id < 0 || id > _lastId) return false;
	if (enable)
	{
		_logrgers[id]._enable = true;
		return true;
	}
	return hotChange(id, LDT_ENABLE_LOGRGER, false, "");
}
bool LogrerManager::setLogrgerLevel(LogrgerId id, int level)
{
	if (id < 0 || id > _lastId) return false;
	if (level <= _logrgers[id]._level)
	{
		_logrgers[id]._level = level;
		return true;
	}
	return hotChange(id, LDT_SET_LOGRGER_LEVEL, level, "");
}
bool LogrerManager::setLogrgerDisplay(LogrgerId id, bool enable) { return hotChange(id, LDT_SET_LOGRGER_DISPLAY, enable, ""); }
bool LogrerManager::setLogrgerOutFile(LogrgerId id, bool enable) { return hotChange(id, LDT_SET_LOGRGER_OUTFILE, enable, ""); }
bool LogrerManager::setLogrgerMonthdir(LogrgerId id, bool enable) { return hotChange(id, LDT_SET_LOGRGER_MONTHDIR, enable, ""); }
bool LogrerManager::setLogrgerFileLine(LogrgerId id, bool enable) { return hotChange(id, LDT_SET_LOGRGER_FILELINE, enable, ""); }

bool LogrerManager::setLogrgerLimitsize(LogrgerId id, unsigned int limitsize)
{
	if (limitsize == 0) { limitsize = (unsigned int)-1; }
	return hotChange(id, LDT_SET_LOGRGER_LIMITSIZE, limitsize, "");
}

bool LogrerManager::setLogrgerName(LogrgerId id, const char * name)
{
	if (id <0 || id > _lastId) return false;
	//the name by main logrger is the process name and it's can't change. 
	//    if (id == LOGR4Z_MAIN_LOGRGER_ID) return false; 

	if (name == NULL || strlen(name) == 0)
	{
		return false;
	}
	return hotChange(id, LDT_SET_LOGRGER_NAME, 0, name);
}

bool LogrerManager::setLogrgerPath(LogrgerId id, const char * path)
{
	if (id <0 || id > _lastId) return false;
	if (path == NULL || strlen(path) == 0)  return false;
	std::string copyPath = path;
	{
		char ch = copyPath.at(copyPath.length() - 1);
		if (ch != '\\' && ch != '/')
		{
			copyPath.append("/");
		}
	}
	return hotChange(id, LDT_SET_LOGRGER_PATH, 0, copyPath);
}
bool LogrerManager::setAutoUpdate(int interval)
{
	_hotUpdateInterval = interval;
	return true;
}
bool LogrerManager::updateConfig()
{
	if (_configFile.empty())
	{
		//LOGRW("logr4z update config file error. filename is empty.");
		return false;
	}
	Logr4zFileHandler f;
	f.open(_configFile.c_str(), "rb");
	if (!f.isOpen())
	{
		std::cout << " !!! !!! !!! !!!" << std::endl;
		std::cout << " !!! !!! logr4z load config file error. filename=" << _configFile << " !!! !!! " << std::endl;
		std::cout << " !!! !!! !!! !!!" << std::endl;
		return false;
	}
	return configFromStringImpl(f.readContent().c_str(), true);
}

bool LogrerManager::isLogrgerEnable(LogrgerId id)
{
	if (id <0 || id > _lastId) return false;
	return _logrgers[id]._enable;
}

unsigned int LogrerManager::getStatusActiveLogrgers()
{
	unsigned int actives = 0;
	for (int i = 0; i <= _lastId; i++)
	{
		if (_logrgers[i]._enable)
		{
			actives++;
		}
	}
	return actives;
}


bool LogrerManager::openLogrger(LogrData * pLogr)
{
	int id = pLogr->_id;
	if (id < 0 || id >_lastId)
	{
		showColorText("logr4z: openLogrger can not open, invalide logrger id! \r\n", LOGR_LEVEL_FATAL);
		return false;
	}

	LogrgerInfo * pLogrger = &_logrgers[id];
	if (!pLogrger->_enable || !pLogrger->_outfile || pLogr->_level < pLogrger->_level)
	{
		return false;
	}

	bool sameday = isSameDay(pLogr->_time, pLogrger->_curFileCreateTime);
	bool needChageFile = pLogrger->_curWriteLen > pLogrger->_limitsize * 1024 * 1024;
	if (!sameday || needChageFile)
	{
		if (!sameday)
		{
			pLogrger->_curFileIndex = 0;
		}
		else
		{
			pLogrger->_curFileIndex++;
		}
		if (pLogrger->_handle.isOpen())
		{
			pLogrger->_handle.close();
		}
	}
	if (!pLogrger->_handle.isOpen())
	{
		pLogrger->_curFileCreateTime = pLogr->_time;
		pLogrger->_curWriteLen = 0;

		tm t = timeToTm(pLogrger->_curFileCreateTime);
		std::string name;
		std::string path;
		_hotLock.lock();
		name = pLogrger->_name;
		path = pLogrger->_path;
		_hotLock.unLock();

		char buf[100] = { 0 };
		if (pLogrger->_monthdir)
		{
			sprintf(buf, "%04d_%02d/", t.tm_year + 1900, t.tm_mon + 1);
			path += buf;
		}

		if (!isDirectory(path))
		{
			createRecursionDir(path);
		}

		//sprintf(buf, "%s_%s_%04d%02d%02d%02d%02d_%s_%03u.logr",
		//    _proName.c_str(), name.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		//    t.tm_hour, t.tm_min, _pid.c_str(), pLogrger->_curFileIndex);
		sprintf(buf, "%s_%s_%03u.log", _proName.c_str(), name.c_str(), pLogrger->_curFileIndex);
		path += buf;
		pLogrger->_handle.open(path.c_str(), "ab");
		if (!pLogrger->_handle.isOpen())
		{
			std::stringstream ss;
			ss << "logr4z: can not open logr file " << path << " . \r\n";
			showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!! \r\n", LOGR_LEVEL_FATAL);
			showColorText(ss.str().c_str(), LOGR_LEVEL_FATAL);
			showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!! \r\n", LOGR_LEVEL_FATAL);
			pLogrger->_outfile = false;
			return false;
		}
		return true;
	}
	return true;
}
bool LogrerManager::closeLogrger(LogrgerId id)
{
	if (id < 0 || id >_lastId)
	{
		showColorText("logr4z: closeLogrger can not close, invalide logrger id! \r\n", LOGR_LEVEL_FATAL);
		return false;
	}
	LogrgerInfo * pLogrger = &_logrgers[id];
	if (pLogrger->_handle.isOpen())
	{
		pLogrger->_handle.close();
		return true;
	}
	return false;
}
bool LogrerManager::popLogr(LogrData *& logr)
{
	AutoLock l(_logrLock);
	if (_logrs.empty())
	{
		return false;
	}
	logr = _logrs.front();
	_logrs.pop_front();
	return true;
}

void LogrerManager::run()
{
	_runing = true;
	LOGA("-----------------  logr4z thread started!  ----------------------------");
	for (int i = 0; i <= _lastId; i++)
	{
		if (_logrgers[i]._enable)
		{
			LOGA("logrger id=" << i
				<< " key=" << _logrgers[i]._key
				<< " name=" << _logrgers[i]._name
				<< " path=" << _logrgers[i]._path
				<< " level=" << _logrgers[i]._level
				<< " display=" << _logrgers[i]._display);
		}
	}

	_semaphore.post();


	LogrData * pLogr = NULL;
	int needFlush[LOGR4Z_LOGRGER_MAX] = { 0 };
	time_t lastCheckUpdate = time(NULL);
	while (true)
	{
		while (popLogr(pLogr))
		{
			if (pLogr->_id <0 || pLogr->_id > _lastId)
			{
				freeLogrData(pLogr);
				continue;
			}
			LogrgerInfo & curLogrger = _logrgers[pLogr->_id];

			if (pLogr->_type != LDT_GENERAL)
			{
				onHotChange(pLogr->_id, (LogrDataType)pLogr->_type, pLogr->_typeval, std::string(pLogr->_content, pLogr->_contentLen));
				curLogrger._handle.close();
				freeLogrData(pLogr);
				continue;
			}

			//
			_ullStatusTotalPopLogr++;
			//discard

			if (!curLogrger._enable || pLogr->_level <curLogrger._level)
			{
				freeLogrData(pLogr);
				continue;
			}


			if (curLogrger._display && !LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
			{
				showColorText(pLogr->_content, pLogr->_level);
			}
			if (LOGR4Z_ALL_DEBUGOUTPUT_DISPLAY && !LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
			{
#ifdef WIN32
				OutputDebugStringA(pLogr->_content);
#endif
			}


			if (curLogrger._outfile && !LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
			{
				if (!openLogrger(pLogr))
				{
					freeLogrData(pLogr);
					continue;
				}

				curLogrger._handle.write(pLogr->_content, pLogr->_contentLen);
				curLogrger._curWriteLen += (unsigned int)pLogr->_contentLen;
				needFlush[pLogr->_id] ++;
				_ullStatusTotalWriteFileCount++;
				_ullStatusTotalWriteFileBytes += pLogr->_contentLen;
			}
			else if (!LOGR4Z_ALL_SYNCHRONOUS_OUTPUT)
			{
				_ullStatusTotalWriteFileCount++;
				_ullStatusTotalWriteFileBytes += pLogr->_contentLen;
			}

			freeLogrData(pLogr);
		}

		for (int i = 0; i <= _lastId; i++)
		{
			if (_logrgers[i]._enable && needFlush[i] > 0)
			{
				_logrgers[i]._handle.flush();
				needFlush[i] = 0;
			}
			if (!_logrgers[i]._enable && _logrgers[i]._handle.isOpen())
			{
				_logrgers[i]._handle.close();
			}
		}

		//! delay. 
		sleepMillisecond(100);

		//! quit
		if (!_runing && _logrs.empty())
		{
			break;
		}

		if (_hotUpdateInterval != 0 && time(NULL) - lastCheckUpdate > _hotUpdateInterval)
		{
			updateConfig();
			lastCheckUpdate = time(NULL);
		}



	}

	for (int i = 0; i <= _lastId; i++)
	{
		if (_logrgers[i]._enable)
		{
			_logrgers[i]._enable = false;
			closeLogrger(i);
		}
	}

}

//////////////////////////////////////////////////////////////////////////
//ILogr4zManager::getInstance
//////////////////////////////////////////////////////////////////////////
ILogr4zManager * ILogr4zManager::getInstance()
{
	static LogrerManager m;
	return &m;
}



_ZSUMMER_E_LOGR4Z_END
_ZSUMMER_E_END
