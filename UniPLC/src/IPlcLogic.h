/*
 * IPlcLogic.h
 *
 *  Created on: Apr 11, 2018
 *      Author: tom
 */

#ifndef IPLCLOGIC_H_
#define IPLCLOGIC_H_

#include "IIODevice.h"
#include "UniPLC.h"
#include "logger.h"

class IPlcLogic : public IModbusServerEvents, public IIODeviceEvents
{
public:
	virtual void allUpdated() = 0;
};

typedef IPlcLogic* constructPlcLogic_t(Logger::logger_t logger, ModbusServer*, std::vector<IIODevice*>&);
typedef void destroyPlcLogic_t(IPlcLogic* );

#endif /* IPLCLOGIC_H_ */
