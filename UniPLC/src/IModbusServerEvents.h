/*
 * IModbusServerEvents.h
 *
 *  Created on: Apr 10, 2018
 *      Author: tom
 */

#ifndef IMODBUSSERVEREVENTS_H_
#define IMODBUSSERVEREVENTS_H_

class ModbusServer;

class IModbusServerEvents
{
public:
	IModbusServerEvents() {}
	virtual ~IModbusServerEvents() {}

	virtual void requestReadDiscreteInput(int address) = 0;
	virtual void requestReadCoil(int address) = 0;
	virtual void requestReadInputRegister(int address) = 0;
	virtual void requestReadHoldingRegister(int address) = 0;

	virtual void writtenCoil(int address) = 0;
	virtual void writtenHoldingRegister(int address) = 0;
};

#endif /* IMODBUSSERVEREVENTS_H_ */
