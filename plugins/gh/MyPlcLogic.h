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

#include <libconfig.h++>

class MyPlcLogic : public IPlcLogic
{
public:
	MyPlcLogic(ModbusServer* mbServer, std::vector<IIODevice*>& ioDevices, libconfig::Setting* settings);
	~MyPlcLogic();

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

private:
	/* Timing */
	static bool isBetweenMod(unsigned long long start, unsigned long long stop, unsigned long long x, unsigned long long mod);
	static uint64_t milliSeconds();
	void checkTriggerTimer(IIODevice* ioDevice, uint8_t& coil, uint8_t& pin, uint64_t& started_ms, uint64_t time_ms, const char* name = NULL);
	unsigned long long getUtc_us();

	/* Voltage range */
	static float range(float in, float inMin, float inMax, float outMin, float outMax);
	static float intToVoltRange(uint16_t x);
	static uint16_t voltToIntRange(float v);

	/* Save holding regs */
	void saveState();
	void loadState();

	/* Helpers */
	static const char *digitalInputName(int index);

public:
	static Logger::logger_t logger;

private:
	libconfig::Setting* settings;
	ModbusServer* mbServer;
	std::vector<IIODevice*>& ioDevices;

	/* Nutrients */
	uint64_t nutrientsTestStart = 0;

	/* Save state */
	const char* saveConfigState = NULL;
	uint64_t periodicSaveStateTimer = 0;
	uint8_t dirtyState = 0;

	/* local states */
	bool lamp = false;
	bool reservoirPump = false;
	bool fan = false;
};

#endif /* MYPLCLOGIC_H_ */
