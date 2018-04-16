/*
 * U12IODevice.h
 *
 *  Created on: Apr 10, 2018
 *      Author: tom
 */

#ifndef U12IODEVICE_H_
#define U12IODEVICE_H_

#include <libconfig.h++>

#include "IIODevice.h"

extern "C"
{
	#include "ljacklm.h"
};

class U12IODevice : public IIODevice
{
public:
	U12IODevice(int deviceIndex, libconfig::Setting* setting);
	virtual ~U12IODevice();

	void attachIODeviceEvents(IIODeviceEvents* events) { this->events = events; }

	void update(int deviceIndex);

	int getDeviceIndex() { return deviceIndex; }

private:
//	static long setBitsFromVector(std::vector<bool>& v, int num, int start);
//	void setAndCompareBitsInVector(int deviceIndex, std::vector<bool>& v, int num, int start, long bits);
	static long setBitsFromVector(std::vector<uint8_t>& v, int num, int start);
	void setAndCompareBitsInVector(int deviceIndex, std::vector<uint8_t>& v, int num, int start, long bits);
	static long gainToLong(long gain);

public:
	static Logger::logger_t logger;

private:
	long idNum;
	long demo;
	IIODeviceEvents* events;
	int deviceIndex;
};

#endif /* U12IODEVICE_H_ */
