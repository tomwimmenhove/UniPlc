/*
 * PlcLogic.cpp
 *
 *  Created on: Apr 11, 2018
 *      Author: tom
 */

#include "MyPlcLogic.h"

Logger::logger_t MyPlcLogic::logger;

using namespace std;

/* constructor */
MyPlcLogic::MyPlcLogic(ModbusServer* mbServer, vector<IIODevice*>& ioDevices)
	: mbServer(mbServer), ioDevices(ioDevices)
{
	this->logger = logger;

	ioDevices[0]->analogOutputs[0] = 1.25;
	ioDevices[0]->analogOutputs[1] = 3.21;

	ioDevices[0]->gains[1] = 2.0f;

	ioDevices[0]->digitalIODirectionOutput[2] = true;
	ioDevices[0]->digitalIO[2] = false;

	ioDevices[0]->digitalIODirectionOutput[3] = true;
	ioDevices[0]->digitalIO[3] = false;

	ioDevices[0]->digitalIODirectionOutput[4] = true;
	ioDevices[0]->digitalIO[4] = true;

	for (int i = 0; i < 8; i++)
	{
		ioDevices[0]->digitalIODirectionOutput[i+4] = true;
		ioDevices[0]->digitalIO[i+4] = true;
	}
}

/* Modbus */
void MyPlcLogic::requestReadDiscreteInput(int address)
{
	logger(LOG_INFO, "Request to read discrete input %d\n", address);
}

void MyPlcLogic::requestReadCoil(int address)
{
	logger(LOG_INFO, "Request to read coil %d\n", address);
}

void MyPlcLogic::requestReadInputRegister(int address)
{
	logger(LOG_INFO, "Request to read input register %d\n", address);
}

void MyPlcLogic::requestReadHoldingRegister(int address)
{
	logger(LOG_INFO, "Request to read input holding register %d\n", address);
}

void MyPlcLogic::writtenCoil(int address)
{
	logger(LOG_INFO, "Written to coil %d\n", address);
}

void MyPlcLogic::writtenHoldingRegister(int address)
{
	logger(LOG_INFO, "Written to holding register %d\n", address);
}

/* IODevice */
void MyPlcLogic::analogInputChanged(IIODevice* ioDevice, int inputIndex)
{
	//cerr << "Analog input " << index << " changed" << endl;
};

void MyPlcLogic::digitalInputChanged(IIODevice* ioDevice, int inputIndex)
{
	bool value = ioDevice->digitalIO[inputIndex];

	logger(LOG_INFO, "Digital input %d on device %d changed to %d\n", inputIndex, ioDevice->getDeviceIndex(), value);
};

void MyPlcLogic::devicePreUpdate(IIODevice* ioDevice)
{

}

void MyPlcLogic::deviceUpdated(IIODevice* ioDevice)
{
	ioDevice->digitalIO[4] = !ioDevice->digitalIO[0];

	//cout << "tick tock" << endl;
}

void MyPlcLogic::allUpdated()
{
	//logger(LOG_INFO, "tick tock\n");
}

/* Functions for contruction and destruction */
extern "C" MyPlcLogic* contruct(Logger::logger_t logger, ModbusServer* mbServer, std::vector<IIODevice*>& ioDevices)
{
	MyPlcLogic::logger = logger;
	return new MyPlcLogic(mbServer, ioDevices);
}

extern "C" void destroy(MyPlcLogic* plcLogic)
{
	delete plcLogic;
}
