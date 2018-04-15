/*
 * logger.cpp
 *
 *  Created on: Apr 14, 2018
 *      Author: tom
 */

#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>

#include "logger.h"

Logger::loggerFunc_t Logger::loggerFunc = Logger::consolelogger;

void Logger::consolelogger(int priority, const char *format, va_list args)
{
	FILE* f = (priority <= LOG_WARNING) ? stderr : stdout;
	vfprintf(f, format, args);
	fflush(f);
}

void Logger::sysloglogger(int priority, const char *format, va_list args)
{
	vsyslog(priority, format, args);
}

void Logger::logger(int priority, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	loggerFunc(priority, format, args);
	va_end(args);
}

void Logger::setLogger(loggerFunc_t f)
{
	loggerFunc = f;
}
