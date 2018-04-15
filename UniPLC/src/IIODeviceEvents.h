/*
 * IIODeviceEvents.h
 *
 *  Created on: Apr 11, 2018
 *      Author: tom
 */

#ifndef IIODEVICEEVENTS_H_
#define IIODEVICEEVENTS_H_

class IIODevice;

class IIODeviceEvents
{
public:
	IIODeviceEvents() {}
	virtual ~IIODeviceEvents() {}

	virtual void analogInputChanged(IIODevice* ioDevice, int inputIndex) = 0;
	virtual void digitalInputChanged(IIODevice* ioDevice, int inputIndex) = 0;

	virtual void devicePreUpdate(IIODevice* ioDevice) = 0;
	virtual void deviceUpdated(IIODevice* ioDevice) = 0;
};

#endif /* IIODEVICEEVENTS_H_ */
