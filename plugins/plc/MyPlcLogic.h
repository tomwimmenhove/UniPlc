/*
 * PlcLogic.h
 *
 *  Created on: Apr 11, 2018
 *      Author: tom
 */

#ifndef MYPLCLOGIC_H_
#define MYPLCLOGIC_H_

#include <cerrno>
#include <cstring>
#include <vector>

#include "ModbusServer.h"
#include "IIODevice.h"
#include "IPlcLogic.h"

using namespace std;

class MyPlcLogic : public IPlcLogic
{
public:
	MyPlcLogic(ModbusServer* mbServer, std::vector<IIODevice*>& ioDevices);
	~MyPlcLogic() {}

	/* Modbus */
	void requestReadDiscreteInput(int address);
	void requestReadCoil(int address);
	void requestReadInputRegister(int address);
	void requestReadHoldingRegister(int address);
	void writtenCoil(int address);
	void writtenHoldingRegister(int address);

	/* IODevice */
	void analogInputChanged(IIODevice* ioDevice, int inputIndex);
	void digitalInputChanged(IIODevice* ioDevice, int inputIndex);
	void devicePreUpdate(IIODevice* ioDevice);
	void deviceUpdated(IIODevice* ioDevice);

	/* UniPLC */
	void allUpdated();

public:
	static Logger::logger_t logger;

private:
	ModbusServer* mbServer;
	std::vector<IIODevice*>& ioDevices;
};

#endif /* MYPLCLOGIC_H_ */
