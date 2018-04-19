/*
 * PlcLogic.cpp
 *
 *  Created on: Apr 11, 2018
 *      Author: tom
 */

#include <math.h>
#include <time.h>
#include <chrono>
#include <sys/time.h>

#include "MyPlcLogic.h"

Logger::logger_t MyPlcLogic::logger;

using namespace std;
using namespace libconfig;

/* Finger savers */
#define MB_COILS						(mbServer->mb_mapping->tab_bits)
#define MB_INPUT						(mbServer->mb_mapping->tab_input_bits)
#define MB_REGS_INPUT					(mbServer->mb_mapping->tab_input_registers)
#define MB_REGS_HOLD					(mbServer->mb_mapping->tab_registers)

/* Pin definitions */
#define PIN_RESERVOIR_BOTTOM_FLOAT		0
#define PIN_RESERVOIR_TOP_FLOAT			1
#define PIN_RESERVOIR_PUMP				4
#define PIN_LAMP						5
#define PIN_FAN							6
#define PIN_NUTRIENT_TEST				7

#define MB_INPUT_FLOAT_ERROR			100

/* Coil definitions */
#define MB_COIL_TRIGGER_PUMP			100
#define MB_COIL_CANCEL_PUMP				101
#define MB_COIL_TRIGGER_NUTRIENT_TEST	102
#define MB_COIL_LAMP_OVERRIDE			201
#define MB_COIL_FAN_OVERRIDE			203
#define MB_COIL_RESERVOIR_PUMP_OVERRIDE	204

/* Holding register definitions */
#define MB_HOLD_LAMP_STARTMINUTE		100
#define MB_HOLD_LAMP_STOPMINUTE			101
#define MB_HOLD_FAN_STARTMINUTE			102
#define MB_HOLD_FAN_STOPMINUTE			103
#define MB_HOLD_NUTRIENT_TEST_MS		104

/* Other random constants */
#define SECSINDAY (3600ull * 24ull)
#define MSECSINDAY (SECSINDAY * 1000ull)
#define USECSINDAY (SECSINDAY * 1000000ull)

#define MSEC2USEC(x) ((x) * 1000ull)
#define SEC2USUEC(x) ((x) * 1000000ull)
//#define MIN2USEC(x) ((x) * 60000000ull)
#define REG2USEC(x) ((x) * 2000000ull) // 2 Seconds/count because 16 bit is not enough for the number of seconds in a day. 2 second precision should be plenty.

/* constructor */
MyPlcLogic::MyPlcLogic(ModbusServer* mbServer, vector<IIODevice*>& ioDevices, Setting* settings)
: settings(settings), mbServer(mbServer), ioDevices(ioDevices)
{
	this->logger = logger;

	ioDevices[0]->digitalIODirectionOutput[PIN_RESERVOIR_PUMP] = true;
	ioDevices[0]->digitalIO[PIN_RESERVOIR_PUMP] = false;

	ioDevices[0]->digitalIODirectionOutput[PIN_LAMP] = true;
	ioDevices[0]->digitalIO[PIN_LAMP] = false;

	ioDevices[0]->digitalIODirectionOutput[PIN_FAN] = true;
	ioDevices[0]->digitalIO[PIN_FAN] = false;

	ioDevices[0]->digitalIODirectionOutput[PIN_NUTRIENT_TEST] = true;
	ioDevices[0]->digitalIO[PIN_NUTRIENT_TEST] = false;

	if (settings == NULL || !settings->exists("saveConfigState"))
	{
		logger(LOG_CRIT, "MyPlcLogic: No \"saveConfigState\" specified in options\n");
		throw 0;
	}

	saveConfigState = (*settings)["saveConfigState"];

	loadState();
}

MyPlcLogic::~MyPlcLogic()
{
	saveState();
}

void MyPlcLogic::saveState()
{
	Config cfg;

	logger(LOG_INFO, "Saving state to \"%s\"\n", saveConfigState);

	Setting& root = cfg.getRoot();

	Setting& holdingRegisters = root.add("HoldingRegisters", Setting::TypeArray);
	for(int i = 0; i < (int) mbServer->mb_mapping->nb_registers; i++)
	{
		holdingRegisters.add(Setting::TypeInt) = MB_REGS_HOLD[i];
	}

	try
	{
		cfg.writeFile(saveConfigState);
	}
	catch(const FileIOException &fioex)
	{
		logger(LOG_CRIT, "I/O error while writing file \"%s\": %s\n", saveConfigState, strerror(errno));
	}

	dirtyState = false;
}

void MyPlcLogic::loadState()
{
	Config cfg;

	try
	{
		cfg.readFile(saveConfigState);
	}
	catch(const FileIOException &fioex)
	{
		logger(LOG_INFO, "MyPlcLogic: Not loading state from \"%s\": %s\n", saveConfigState, strerror(errno));
		return;
	}
	catch(const ParseException &pex)
	{
		Logger::logger(LOG_WARNING, "MyPlcLogic: Parse error at %s: %d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}
	catch(...)
	{
		logger(LOG_INFO, "MyPlcLogic: Not loading state from \"%s\"\n", saveConfigState);
		return;
	}

	logger(LOG_INFO, "Loading state from \"%s\"\n", saveConfigState);

	Setting& root = cfg.getRoot();

	Setting &holdingRegisters = root["HoldingRegisters"];

	if (holdingRegisters.getType() != Setting::TypeArray)
	{
		Logger::logger(LOG_WARNING, "MyPlcLogic: \"HoldingRegisters\" should be an array\n");
		return;
	}
	int holdingCount = holdingRegisters.getLength();
	for (int i = 0; i < holdingCount; i++)
	{
		int regValue = holdingRegisters[i];
		MB_REGS_HOLD[i] = regValue;
	}

	dirtyState = false;
}

/* Modbus */
void MyPlcLogic::requestReadDiscreteInput(int address)
{
	//logger(LOG_INFO, "Request to read discrete input %d\n", address);
}

void MyPlcLogic::requestReadCoil(int address)
{
	//logger(LOG_INFO, "Request to read coil %d\n", address);
}

void MyPlcLogic::requestReadInputRegister(int address)
{
	//logger(LOG_INFO, "Request to read input register %d\n", address);
}

void MyPlcLogic::requestReadHoldingRegister(int address)
{
	//logger(LOG_INFO, "Request to read input holding register %d\n", address);
}

void MyPlcLogic::writtenCoil(int address)
{
	//logger(LOG_INFO, "Written to coil %d\n", address);

	bool value = MB_COILS[address];

	/* Overrides */
	switch (address)
	{
	case MB_COIL_LAMP_OVERRIDE:
		logger(LOG_INFO, "%s override on lamp\n", value ? "Enabled" : "Disabled");
		break;
	case MB_COIL_FAN_OVERRIDE:
		logger(LOG_INFO, "%s override on fan\n", value ? "Enabled" : "Disabled");
		break;
	case MB_COIL_RESERVOIR_PUMP_OVERRIDE:
		logger(LOG_INFO, "%s override on reservoir\n", value ? "Enabled" : "Disabled");
		break;
	}
}

void MyPlcLogic::writtenHoldingRegister(int address)
{
	logger(LOG_INFO, "Written %d to holding register %d\n", MB_REGS_HOLD[address] ,address);

	/* Set the timer to check for periodic state-saving */
	dirtyState = true;
	periodicSaveStateTimer = milliSeconds();
}

/* IODevice */
void MyPlcLogic::analogInputChanged(IIODevice* ioDevice, int inputIndex)
{
	//cerr << "Analog input " << index << " changed" << endl;
};

const char* MyPlcLogic::digitalInputName(int index)
{
	static char buf[32];

	switch (index)
	{
	case PIN_RESERVOIR_BOTTOM_FLOAT:
		return "Reservoir Bottom Float Switch";
	case PIN_RESERVOIR_TOP_FLOAT:
		return "Reservoir Top Float Switch";
	}

	snprintf(buf, sizeof(buf), "%d", index);

	return buf;
}

void MyPlcLogic::digitalInputChanged(IIODevice* ioDevice, int inputIndex)
{
	if (ioDevice->getDeviceIndex() != 0)
	{
		return;
	}

	bool value = ioDevice->digitalIO[inputIndex];
	logger(LOG_INFO, "Digital input \"%s\" turned %s\n", digitalInputName(inputIndex), value ? "on" : "off");
};

uint64_t MyPlcLogic::milliSeconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto epoch = now_ms.time_since_epoch();
	auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
	return value.count();
}

bool MyPlcLogic::isBetweenMod(unsigned long long start, unsigned long long stop, unsigned long long x, unsigned long long mod)
{
	stop += mod - 1; // exclude stop
	start %= mod;
	stop %= mod;
	x %= mod;

	if (start > stop)
	{
		return (x >= start && x < mod) ||
				(x >= 0 && x <= stop);
	}

	return (x >= start) && (x <= stop);
}

void MyPlcLogic::checkTriggerTimer(IIODevice* ioDevice, uint8_t& coil, uint8_t& pin, uint64_t& started_ms, uint64_t time_ms, const char* name)
{
	if (coil) // Trigger
	{
		coil = 0;
		if (name != NULL)
		{
			logger(LOG_INFO, "TriggerTimer: %s start\n", name);
		}
		started_ms = milliSeconds();
		pin = true;
	}
	else // Turn off
	{
		if (started_ms > 0)
		{
			if ((started_ms + time_ms) <= milliSeconds())
			{
				if (name != NULL)
				{
					logger(LOG_INFO, "TriggerTimer: %s stop\n", name);
				}
				pin = false;
				started_ms = 0;
			}
		}
	}
}

unsigned long long MyPlcLogic::getUtc_us()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long) tv.tv_sec * 1000000llu + (unsigned long long) tv.tv_usec;
}

void MyPlcLogic::devicePreUpdate(IIODevice* ioDevice)
{
	if (ioDevice->getDeviceIndex() != 0)
	{
		return;
	}

	if (MB_COILS[MB_COIL_TRIGGER_PUMP]) // Trigger pump
	{
		logger(LOG_INFO, "Manual remote trigger pump\n");
		MB_COILS[MB_COIL_TRIGGER_PUMP] = 0;

		reservoirPump = true; // Pump on
	}

	if (MB_COILS[MB_COIL_CANCEL_PUMP]) // Cancel pump
	{
		logger(LOG_INFO, "Manual remote cancel pump\n");
		MB_COILS[MB_COIL_CANCEL_PUMP] = 0;

		reservoirPump = false; // Pump on
	}

	/* Nutrients timer */
	checkTriggerTimer(ioDevice,
			MB_COILS[MB_COIL_TRIGGER_NUTRIENT_TEST],
			ioDevice->digitalIO[PIN_NUTRIENT_TEST],
			nutrientsTestStart, MB_REGS_HOLD[MB_HOLD_NUTRIENT_TEST_MS] * 1000, "NutrientTest");

	/* Get UTC time */
	unsigned long long utc_us = getUtc_us() % USECSINDAY;

	/* Lamp Timer */
	bool tmpLamp = isBetweenMod(
			REG2USEC(MB_REGS_HOLD[MB_HOLD_LAMP_STARTMINUTE]),
			REG2USEC(MB_REGS_HOLD[MB_HOLD_LAMP_STOPMINUTE]),
			utc_us,
			USECSINDAY);
	bool tmpFan = isBetweenMod(
			REG2USEC(MB_REGS_HOLD[MB_HOLD_FAN_STARTMINUTE]),
			REG2USEC(MB_REGS_HOLD[MB_HOLD_FAN_STOPMINUTE]),
			utc_us,
			USECSINDAY);

	if (tmpLamp && !lamp)
	{
		logger(LOG_INFO, "Lamp on timer triggered\n");
	}
	if (!tmpLamp && lamp)
	{
		logger(LOG_INFO, "Lamp off timer triggered\n");
	}

	if (tmpFan && !fan)
	{
		logger(LOG_INFO, "Fan on timer triggered\n");
	}
	if (!tmpFan && fan)
	{
		logger(LOG_INFO, "Fan off timer triggered\n");
	}

	lamp = tmpLamp;
	fan = tmpFan;

	ioDevice->digitalIO[PIN_LAMP] 			= lamp 			|| MB_COILS[MB_COIL_LAMP_OVERRIDE];
	ioDevice->digitalIO[PIN_FAN] 			= fan	 		|| MB_COILS[MB_COIL_FAN_OVERRIDE];
	ioDevice->digitalIO[PIN_RESERVOIR_PUMP]	= reservoirPump	|| MB_COILS[MB_COIL_RESERVOIR_PUMP_OVERRIDE];
}

void MyPlcLogic::deviceUpdated(IIODevice* ioDevice)
{
	if (ioDevice->getDeviceIndex() != 0)
	{
		return;
	}

	bool floatSwitchFault = ioDevice->digitalIO[PIN_RESERVOIR_TOP_FLOAT] && (!ioDevice->digitalIO[PIN_RESERVOIR_BOTTOM_FLOAT]);

	if (floatSwitchFault && !MB_INPUT[MB_INPUT_FLOAT_ERROR])
	{
		logger(LOG_CRIT, "Float switch fault\n");
	}

	if (!floatSwitchFault && MB_INPUT[MB_INPUT_FLOAT_ERROR])
	{
		logger(LOG_CRIT, "Float switch fault possibly corrected\n");
	}

	if (!floatSwitchFault)
	{
		if (ioDevice->digitalIO[PIN_RESERVOIR_TOP_FLOAT])
		{
			if (!reservoirPump)
			{
				logger(LOG_INFO, "Turning on pump\n");
				reservoirPump = true;
			}
		}
		if (!ioDevice->digitalIO[PIN_RESERVOIR_BOTTOM_FLOAT])
		{
			if (reservoirPump)
			{
				logger(LOG_INFO, "Turning off pump\n");
				reservoirPump = false;
			}
		}
	}

	/* Indicates a dicky float switch */
	MB_INPUT[MB_INPUT_FLOAT_ERROR] = floatSwitchFault;

	/* Reflect the analog inputs at the input registers */
	for (size_t i = 0; i < ioDevice->analogInputs.size(); i++)
	{
		MB_REGS_INPUT[i] = voltToIntRange(ioDevice->analogInputs[i]);
	}

	/* Make the first entries in tab_bits always reflect the actual relays-status */
	for (size_t i = 0; i < ioDevice->digitalIO.size() ; i++)
	{
		MB_INPUT[i] = ioDevice->digitalIO[i];
	}

}

void MyPlcLogic::allUpdated()
{
	if (settings != NULL && settings->exists("autoSaveTime_ms"))
	{
		/* Check for periodic saving */
		int autoSaveTime_ms = (*settings)["autoSaveTime_ms"];
		if (dirtyState && milliSeconds() > periodicSaveStateTimer + autoSaveTime_ms)
		{
			saveState();
		}
	}
}

float MyPlcLogic::range(float in, float inMin, float inMax, float outMin, float outMax)
{
	return outMin + (outMax - outMin) * ((in - inMin) / (inMax - inMin));
}

float MyPlcLogic::intToVoltRange(uint16_t x)
{
	return range(x, 0, 65536, -6.0f, 6.0f);
}

uint16_t MyPlcLogic::voltToIntRange(float v)
{
	return round(range(v, -6.0f, 6.0f, 0, 65536));
}

/* Functions for contruction and destruction */
extern "C" MyPlcLogic* contruct(Logger::logger_t logger, ModbusServer* mbServer, std::vector<IIODevice*>& ioDevices, Setting* settings)
{
	MyPlcLogic::logger = logger;
	return new MyPlcLogic(mbServer, ioDevices, settings);
}

extern "C" void destroy(MyPlcLogic* plcLogic)
{
	delete plcLogic;
}
