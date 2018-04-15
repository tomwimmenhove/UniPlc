/*
 * UniPLC.h
 *
 *  Created on: Apr 12, 2018
 *      Author: tom
 */

#ifndef UNIPLC_H_
#define UNIPLC_H_

#include <vector>
#include <syslog.h>

#include "ModbusServer.h"
#include "logger.h"

class IPlcLogic;
class IIODevice;

class UniPLC
{
public:
	UniPLC(int argc, char **argv);
	virtual ~UniPLC();

	bool loadPlugin(const char* path, void** constructor, void** destructor);

	int run();

	int getLastSignal();

private:
	static void signalHandler(int signum);

private:
	IPlcLogic* plcLogic;
	void* destroyPlcLogic;

	std::vector<IIODevice*> ioDevices;
	std::vector<void*> ioDevicesDestructors;

	ModbusServer* modbusServer;

	bool daemonize;

	static int sig;
};

#endif /* UNIPLC_H_ */
