/*
 * logger.h
 *
 *  Created on: Apr 14, 2018
 *      Author: tom
 */

#ifndef LOGGER_H_
#define LOGGER_H_

class Logger
{
public:
	typedef void (*logger_t)(int priority, const char *format, ...);
	typedef void (*loggerFunc_t)(int priority, const char *format, va_list args);

	static void consolelogger(int priority, const char *format, va_list args);
	static void sysloglogger(int priority, const char *format, va_list args);
	static void logger(int priority, const char *format, ...);

	static void setLogger(loggerFunc_t f);

private:
	static loggerFunc_t loggerFunc;
};



#endif /* LOGGER_H_ */
