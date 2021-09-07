#pragma once
#include <iostream>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
//[时间 info/warning/error/fatal/debug 文件 行号] 具体的错误信息

const char* Level[] =
{
  "INFO",
  "WARNING",
  "ERROR",
  "FATAL",
  "DEBUG"
};

enum LogLevel
{
  INFO = 0,
  WARNING,
  ERROR,
  FATAL,
  DEBUG
};

class LogTime
{
  public:
      static void GetTimeStamp(std::string& timestamp)
      {
        //返回年月日 时分秒
        time_t SysTime;
        time(&SysTime);

        struct tm* ST = localtime(&SysTime);
        //格式化字符串[YYYY-MM-DD HH-MM-ss]
        char TimeNow[23] = {'\0'};
        snprintf(TimeNow, sizeof(TimeNow) - 1, "%04d-%02d-%02d %02d:%02d:%02d", ST->tm_year + 1900, ST ->tm_mon + 1, ST->tm_mday, ST->tm_hour, ST->tm_min, ST->tm_sec);
        timestamp.assign(TimeNow, strlen(TimeNow));
      }
};

inline std::ostream& Log(LogLevel lev, const char* file, int line, const std::string& logmsg)
{
  std::string level_info = Level[lev];
  std::string timer_stamp;

  LogTime::GetTimeStamp(timer_stamp);

  std::cout << "[" << timer_stamp << " " << level_info << " " << file << ":" << line << "]" << logmsg;

  return std::cout;
}

#define LOG(lev, msg) Log(lev, __FILE__, __LINE__, msg)
