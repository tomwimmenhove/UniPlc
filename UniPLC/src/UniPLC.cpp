/*
 * UniPLC.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: tom
 */

#include <iostream>
#include <cerrno>
#include <cstring>
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "IPlcLogic.h"
#include "UniPLC.h"

using namespace std;
using namespace libconfig;

int UniPLC::sig = 0;

UniPLC::UniPLC(int argc, char **argv)
	: daemonize(false)
{
	Logger::logger(LOG_INFO, "UniPLC Starting\n");

	const char* configFile = "/etc/uniplc/uniplc.conf";
	pidFile = "/var/run/uniplc/uniplc.pid";
	int c;

	optind = 1;

	while (1)
	{
		static struct option long_options[] =
		{
				/* These options set a flag. */
				{"help", 	no_argument,       0, 'h'},
				{"config",	required_argument, 0, 'f'},
				{"daemon",	no_argument,       0, 'd'},
				{"pidfile",	required_argument, 0, 'p'},
				{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hf:dp:",
				long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
		case 'h':
			printf("Usage %s <options>\n", argv[0]);
			printf("\t--help   -h               : This\n");
			printf("\t--config -f <config file> : Specify config file\n");
			printf("\t--daemon -d               : Daemonize\n");
			printf("\t--pidfile -p <pid file>   : Specify pid file\n");
			throw 0;
			break;

		case 'f':
			configFile = optarg;
			break;

		case 'd':
			daemonize = true;
			break;

		case 'p':
			pidFile = optarg;
			break;

		default:
			throw 1;
		}
	}

	try
	{
		Logger::logger(LOG_INFO, "Using configuration file %s\n", configFile);
		cfg.readFile(configFile);
	}
	catch(const FileIOException &fioex)
	{
		Logger::logger(LOG_CRIT, "I/O error while reading configuration file: %s\n", strerror(errno));
		throw 1;
	}
	catch(const ParseException &pex)
	{
		Logger::logger(LOG_CRIT, "Parse error at %s: %d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
		throw 1;
	}
	catch(...)
	{
		Logger::logger(LOG_CRIT, "Error while reading configuration file.\n");
	}

	Setting &root = cfg.getRoot();

	/* Setup Modbus server */
	if (!root.exists("Modbus"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"Modbus\" entry\n");
		throw 1;
	}
	Setting &modbusSettings = root["Modbus"];

	if (!modbusSettings.exists("ip"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"Modbus/ip\" entry\n");
		throw 1;
	}
	const char* ip = modbusSettings["ip"].c_str();

	if (!modbusSettings.exists("port"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"Modbus/port\" entry\n");
		throw 1;
	}
	int port = modbusSettings["port"];

	if (!modbusSettings.exists("maxconnections"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"Modbus/maxconnections\" entry\n");
		throw 1;
	}
	int maxconnections = modbusSettings["maxconnections"];

	try
	{
		modbusServer = new ModbusServer(ip, port, maxconnections);
	}
	catch (int err)
	{
		Logger::logger(LOG_CRIT, "Failed to start modbus server: %s\n", strerror(errno));
		throw 1;
	}

	/* Setup IO Devices */
	if (!root.exists("IODevices"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"IODevices\" entry\n");
		throw 1;
	}
	Setting &iODevices = root["IODevices"];

	if (!iODevices.exists("DeviceList"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"DeviceList\" entry in \"IODevices\" found\n");
		throw 1;
	}
	Setting &deviceList = iODevices["DeviceList"];

	if (deviceList.getType() != Setting::TypeArray)
	{
		Logger::logger(LOG_CRIT, "Configuration: \"DeviceList\" should be an array\n");
		throw 1;
	}
	int deviceListCount = deviceList.getLength();
	for (int i = 0; i < deviceListCount; i++)
	{
		const char* deviceName = deviceList[i].c_str();

		Logger::logger(LOG_INFO, "Found device entry: \"%s\"\n", deviceName);

		if (!iODevices.exists(deviceName))
		{
			Logger::logger(LOG_CRIT, "Configuration: No \"%s\" entry in \"IODevices\" found\n", deviceName);
			throw 1;
		}

		Setting& device = iODevices[deviceName];
		if (!device.exists("plugin"))
		{
			Logger::logger(LOG_CRIT, "Configuration: No \"plugin\" entry in \"IODevices/%s\" found\n", deviceName);
			throw 1;
		}
		const char* iODevicesPluginPath = device["plugin"].c_str();

		Logger::logger(LOG_INFO, "Loading IODevice \"%s\" plugin: %s\n", deviceName, iODevicesPluginPath);
		contructIODevice_t* constructIODevice;
		destroyIODevice_t* destroyIODevice;
		void* handle;
		if ((handle = loadPlugin(iODevicesPluginPath, (void**) &constructIODevice, (void**) &destroyIODevice)) == NULL)
		{
			throw 1;
		}

		Setting* options = NULL;
		if (device.exists("options"))
		{
			options = &device["options"];
		}

		Logger::logger(LOG_INFO, "Loading IODevice \"%s\"\n", deviceName);
		IIODevice* ioDevice = constructIODevice(Logger::logger, i, options);
		if (ioDevice == NULL)
		{
			Logger::logger(LOG_CRIT, "Failed to load IODevice \"%s\"\n", deviceName);
		}
		ioDeviceHandles.push_back(handle);
		ioDevices.push_back(ioDevice);
		ioDevicesDestructors.push_back((void*) destroyIODevice);
	}

	/* Setup PLC Logic */
	if (!root.exists("PLCLogic"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"PLCLogic\" entry\n");
		throw 1;
	}
	Setting &plcLogicSetting = root["PLCLogic"];
	if (!plcLogicSetting.exists("plugin"))
	{
		Logger::logger(LOG_CRIT, "Configuration: No \"plugin\" entry in \"IODevices\" found\n");
		throw 1;
	}
	plcLogicPluginPath = plcLogicSetting["plugin"].c_str();

	plcLogicOptions = NULL;
	if (plcLogicSetting.exists("options"))
	{
		plcLogicOptions = &plcLogicSetting["options"];
	}

	initPLCLogic();
}

UniPLC::~UniPLC()
{
	/* Destroy PLC logic */
	killPLCLogic();

	/* Destroy IO Devices */
	for(size_t i = 0; i < ioDevices.size(); i++)
	{
		destroyIODevice_t* destructor = (destroyIODevice_t*) ioDevicesDestructors[i];
		destructor(ioDevices[i]);
		dlclose(ioDeviceHandles[i]);
	}

	/* Destroy the server */
	delete modbusServer;
}

void UniPLC::initPLCLogic()
{
	Logger::logger(LOG_INFO, "Loading PLCLogic plugin: %s\n", plcLogicPluginPath);
	constructPlcLogic_t* constructPlcLogic;
	if ((plcLogicHandle = loadPlugin(plcLogicPluginPath, (void**) &constructPlcLogic, (void**) &destroyPlcLogic)) == NULL)
	{
		throw 1;
	}

	Logger::logger(LOG_INFO, "Loading PLCLogic\n");
	plcLogic = constructPlcLogic(Logger::logger, modbusServer, ioDevices, plcLogicOptions);
	if (plcLogic == NULL)
	{
		Logger::logger(LOG_CRIT, "Failed to load PLCLogic\n");
		throw 1;
	}

	/* Attach PLC Logic to modbus server */
	modbusServer->attachServerEvents(plcLogic);

	/* Attach PLC Logic to IO Devices */
	for (size_t i = 0; i < ioDevices.size(); i++)
	{
		ioDevices[i]->attachIODeviceEvents(plcLogic);
	}
}

void UniPLC::killPLCLogic()
{
	destroyPlcLogic_t* destructor = (destroyPlcLogic_t*) destroyPlcLogic;
	destructor(plcLogic);
	dlclose(plcLogicHandle);
}

void UniPLC::reloadPLCLogic()
{
	killPLCLogic();
	initPLCLogic();
}

void* UniPLC::loadPlugin(const char* path, void** constructor, void** destructor)
{
	void* handle = dlopen(path, RTLD_LAZY);
	if (handle == NULL)
	{
		Logger::logger(LOG_CRIT, "Cannot load library: %s\n", dlerror());
		return NULL;
	}
	dlerror();

	// load the symbols
	*constructor = dlsym(handle, "contruct");
	const char* dlsym_error = dlerror();
	if (dlsym_error)
	{
		Logger::logger(LOG_CRIT, "Cannot load symbol \"contruct\": %s\n", dlsym_error);
		dlclose(handle);
		return NULL;
	}

	*destructor = dlsym(handle, "destroy");
	dlsym_error = dlerror();
	if (dlsym_error)
	{
		Logger::logger(LOG_CRIT, "Cannot load symbol \"destroy\": %s\n", dlsym_error);
		dlclose(handle);
		return NULL;
	}

	return handle;
}

int UniPLC::getLastSignal()
{
	int ret = sig;
	sig = 0;
	return ret;
}

void UniPLC::signalHandler(int signum)
{
	Logger::logger(LOG_NOTICE, "Received signal %d\n", signum);
	sig = signum;
}

int UniPLC::run()
{
	if (daemonize)
	{
		if (daemon(1, 0) == -1)
		{
			Logger::logger(LOG_CRIT, "Failed to daemonize\n");
			return -1;
		}

		Logger::setLogger(Logger::sysloglogger);
		openlog("UniPLC", LOG_PID, LOG_DAEMON);

		FILE *fPid = fopen(pidFile, "w+");
		if (fPid == NULL)
		{
			Logger::logger(LOG_CRIT, "Could not write pid file %s: %s\n", pidFile, strerror(errno));
		}
		else
		{
			fprintf(fPid, "%d", getpid());
			fclose(fPid);
		}

		daemonize = false;
	}

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = signalHandler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);

    Logger::logger(LOG_INFO, "Daemon running\n");

	/* The main loop */
	for (;;)
	{
		/* Loop while there's shit to do for the server */
		for (;;)
		{
			struct timeval tv = { 0, 0, };
			if (!modbusServer->processEvents(&tv))
			{
				break;
			}
		}

		/* Update the PLC in- and outputs */
		for(size_t i = 0; i < ioDevices.size(); i++)
		{
			ioDevices[i]->update(i);
		}

		plcLogic->allUpdated();

		if (sig)
		{
			if (sig == SIGUSR1)
			{
				sig = 0;
				reloadPLCLogic();
			}
			else
			{
				break;
			}
		}
	}

	return 0;
}
