/*
 * U12IODevice.cpp
 *
 *  Created on: Apr 10, 2018
 *      Author: tom
 */

#include <cstdlib>
#include <libconfig.h++>

#include "U12IODevice.h"

using namespace std;
using namespace libconfig;

Logger::logger_t U12IODevice::logger;

U12IODevice::U12IODevice(int deviceIndex, Setting* setting)
	: events(NULL),
	  deviceIndex(deviceIndex)
{
	this->logger = logger;

	static const float gainArray[] = { 1.0f, 2.0f, 4.0f, 5.0f, 8.0f, 10.0f, 16.0f, 20.0f };
	idNum = -1;
	demo = 0;

	logger(LOG_INFO, "U12IODevice: Initializing device index %d\n", deviceIndex);

	if (setting != NULL)
	{
		if (setting->exists("idnum"))
		{
			idNum = (int) (*setting)["idnum"];
			logger(LOG_INFO, "U12IODevice: IdNum: %d\n", idNum);
		}
		if (setting->exists("demo"))
		{
			demo = (int) (*setting)["demo"];
			if (demo)
			{
				logger(LOG_INFO, "U12IODevice: Demo mode\n");
			}
		}
	}

	float version = 0.0;

	version = GetDriverVersion();
	logger(LOG_INFO, "U12IODevice: U12 Driver version %f\n", version);

	version = GetFirmwareVersion(&idNum);
	logger(LOG_INFO, "U12IODevice: U12 Firmware version %f\n", version);

	/* The 4 IO's and 16 D's are combined together, respectively. */

	digitalIODirectionOutput.resize(20, false);		// Set all digital lines to be inputs
	digitalIO.resize(20, 0);
	gains.resize(4, 1.0f);
	supportedGains.assign(gainArray, gainArray + 8);
	analogInputs.resize(12, 0.0f); // 8 single-ended inputs, 4 differential inputs
	analogOutputs.resize(2, 0.0f);
	counters.resize(1, 0);

	statusuLed = true;
}

U12IODevice::~U12IODevice()
{
}

//long U12IODevice::setBitsFromVector(std::vector<bool>& v, int num, int start)
long U12IODevice::setBitsFromVector(std::vector<uint8_t>& v, int num, int start)
{
	long bits = 0;

	for (int i = 0; i < num; i++)
	{
		if (v[i + start])
		{
			bits |= 1 << i;
		}
	}

	return bits;
}

//void U12IODevice::setAndCompareBitsInVector(int deviceIndex, std::vector<bool>& v, int num, int start, long bits)
void U12IODevice::setAndCompareBitsInVector(int deviceIndex, std::vector<uint8_t>& v, int num, int start, long bits)
{
	for (int i = 0; i < num; i++)
	{
		int inputIndex = i + start;

		bool value = ( (bits & (1 << i)) != 0);
		if (v[inputIndex] != value)
		{
			v[inputIndex] = value;
			if (events != NULL)
			{
				events->digitalInputChanged(this, inputIndex);
			}
		}
	}
}

long U12IODevice:: gainToLong(long gain)
{
	switch(gain)
	{
	case 1:  return 0;
	case 2:  return 1;
	case 4:  return 2;
	case 8:  return 3;
	case 10: return 4;
	case 16: return 5;
	case 20: return 6;
	}

	return -1;
}

void U12IODevice::update(int deviceIndex)
{
	char errorString[50];
	long res;

	if (events != NULL)
	{
		events->devicePreUpdate(this);
	}

	/* Setup data for  AOUpdate() */

	/* The 4 IO's and 16 D's are combined together, respectively. */
	long trisIO = setBitsFromVector(digitalIODirectionOutput, 4, 0);
	long stateIO = setBitsFromVector(digitalIO, 4, 0);
	long trisD = setBitsFromVector(digitalIODirectionOutput, 16, 4);
	long stateD = setBitsFromVector(digitalIO, 16, 4);

	long updateDigital = 1;
	long resetCounter = 0;
	unsigned long count;
	float analogOut0 = analogOutputs[0];
	float analogOut1 = analogOutputs[1];

	res = AOUpdate (&idNum,
	                demo,
	                trisD,
	                trisIO,
	                &stateD,
	                &stateIO,
	                updateDigital,
	                resetCounter,
	                &count,
	                analogOut0,
	                analogOut1);

	if (res != 0)
	{
		GetErrorString(res, errorString);
		logger(LOG_CRIT, "U12IODevice: LabJack error: %s\n", errorString);
		return;
	}

	/* Store the results */
	setAndCompareBitsInVector(deviceIndex, digitalIO, 4, 0, stateIO);
	setAndCompareBitsInVector(deviceIndex, digitalIO, 16, 4, stateD);
	counters[0] = count;

	/* Setup data for AISample() */
	long channels[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, };
	long lgains[12] = { 0, 0, 0, 0, 0, 0, 0, 0,  /* Diff */ 0, 0, 0, 0 };

	long overVoltage;
	float voltages[12] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, };

	for (int i = 0; i < 4; i++)
	{
		long g = gainToLong(gains[i]);
		if (g == -1)
		{
			logger(LOG_CRIT, "U12IODevice: Invalid gain for differential pair: %d\n", i);
			continue;
		}
		lgains[i+8] = g;
	}

	for (int i = 0; i < 12; i +=4)
	{
		res = AISample (&idNum,
				demo,
				&stateIO,
				0,
				statusuLed,
				4,
				&channels[i],
				&lgains[i],
				0,
				&overVoltage,
				&voltages[i]);
		if (res != 0)
		{
			GetErrorString(res, errorString);
			logger(LOG_CRIT, "U12IODevice: LabJack error: %s\n", errorString);
			return;
		}

		if (overVoltage)
		{
			logger(LOG_CRIT, "U12IODevice: LabJack overvoltage on analog inputs %d-%d\n", i, i+3);
		}
	}

	/* Store the results */
	for (int i = 0; i < 12; i++)
	{
		if (analogInputs[i] != voltages[i])
		{
			analogInputs[i] = voltages[i];
			if (events != NULL)
			{
				events->analogInputChanged(this, i);
			}
		}
	}

	if (events != NULL)
	{
		events->deviceUpdated(this);
	}

	statusuLed = !statusuLed;
}

extern "C" IIODevice* contruct(Logger::logger_t logger, int deviceIndex, Setting* setting)
{
	U12IODevice::logger = logger;
	return new U12IODevice(deviceIndex, setting);
}

extern "C" void destroy(IIODevice* ioDevice)
{
	delete (U12IODevice*) ioDevice;
}
