/*
 * ModbusServer.h
 *
 *  Created on: Apr 10, 2018
 *      Author: Tom Wimmenhove
 */

#ifndef MODBUS_H_
#define MODBUS_H_

#include "IModbusServerEvents.h"

extern "C"
{
	#include <modbus/modbus.h>

	/* According to POSIX.1-2001, POSIX.1-2008 */
	#include <sys/select.h>

	/* According to earlier standards */
	#include <sys/time.h>
	#include <sys/types.h>
	#include <unistd.h>

	#include <sys/select.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
};

class ModbusServer
{
public:
	ModbusServer(const char *ip_address, int port, int maxConnections);
	virtual ~ModbusServer();

	void attachServerEvents(IModbusServerEvents* events) { this->events = events; }

	bool processEvents(struct timeval *timeout);

private:
	void preReply(int length);
	void postReply(int length);

public:
	modbus_mapping_t *mb_mapping;

private:
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    fd_set refset;
    fd_set rdset;
    int fdmax;
    int header_length;
    modbus_t *ctx = NULL;
    int server_socket;
    IModbusServerEvents* events;
};

#endif /* MODBUS_H_ */
