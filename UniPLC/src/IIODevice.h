/*
 * IIODevice.h
 *
 *  Created on: Apr 10, 2018
 *      Author: tom
 */

#ifndef IIODEVICE_H_
#define IIODEVICE_H_

#include <vector>
#include <libconfig.h++>

#include "IIODeviceEvents.h"
#include "UniPLC.h"
#include "logger.h"

typedef IIODevice* contructIODevice_t(Logger::logger_t logger, int deviceIndex, libconfig::Setting* setting);
typedef void destroyIODevice_t(IIODevice*);

class IIODevice {
public:
	virtual ~IIODevice() {}

	virtual void attachIODeviceEvents(IIODeviceEvents* events) = 0;

	virtual void update(int deviceIndex) = 0;

	virtual int getDeviceIndex() = 0;

public:
	std::vector<bool> digitalIODirectionOutput;
	std::vector<bool> digitalIO;
	std::vector<float> analogInputs;
	std::vector<float> gains;
	std::vector<float> supportedGains;
	std::vector<float> analogOutputs;
	std::vector<unsigned int> counters;
};

#endif /* IIODEVICE_H_ */
