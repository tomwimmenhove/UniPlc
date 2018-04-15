/*
 * ModbusServer.cpp
 *
 *  Created on: Apr 10, 2018
 *      Author: Tom Wimmenhove
 */

#include <iostream>
#include <cerrno>
#include <cstring>

#include "ModbusServer.h"
#include "logger.h"

using namespace std;

ModbusServer::ModbusServer(const char *ip_address, int port, int maxConnections)
: events(NULL)
{
	ctx = modbus_new_tcp(ip_address, port);

	header_length = modbus_get_header_length(ctx);

	mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, MODBUS_MAX_WRITE_BITS, MODBUS_MAX_READ_REGISTERS, MODBUS_MAX_WRITE_REGISTERS);
	if (mb_mapping == NULL)
	{
		int err = errno;
		modbus_free(ctx);

		throw err;
	}

	mb_mapping->tab_input_registers[0] = 42;
	mb_mapping->tab_input_registers[1] = 43;
	mb_mapping->tab_registers[0] = 44;
	mb_mapping->tab_registers[1] = 45;

	server_socket = modbus_tcp_listen(ctx, maxConnections);
	if (server_socket == -1)
	{
		int err = errno;

		modbus_free(ctx);
		throw err;
	}

	/* Clear the reference set of socket */
	FD_ZERO(&refset);
	/* Add the server socket */
	FD_SET(server_socket, &refset);

	/* Keep track of the max file descriptor */
	fdmax = server_socket;
}

ModbusServer::~ModbusServer()
{
	modbus_free(ctx);
	close(server_socket);
}

void ModbusServer::preReply(int length)
{
	if (length < header_length + 4)
	{
		return;
	}

	int functionCode = query[header_length];
	int address = MODBUS_GET_INT16_FROM_INT8(query, header_length + 1);
	int num = MODBUS_GET_INT16_FROM_INT8(query, header_length + 3);

	/* In pre-reply we care about values to be read */
	switch (functionCode)
	{
	case 2: // Read Discrete Inputs
		for (int i = address; i < address + num; i++)
		{
			events->requestReadDiscreteInput(i);
		}
		break;
	case 1: // Read Coils
		for (int i = address; i < address + num; i++)
		{
			events->requestReadCoil(i);
		}
		break;
	case 4: // Read Input Registers
		for (int i = address; i < address + num; i++)
		{
			events->requestReadInputRegister(i);
		}
		break;
	case 3: // Read Multiple Holding Registers
		for (int i = address; i < address + num; i++)
		{
			events->requestReadHoldingRegister(i);
		}
		break;
	}
}

void ModbusServer::postReply(int length)
{
	if (length < header_length + 4)
	{
		return;
	}

	int functionCode = query[header_length];
	int address = MODBUS_GET_INT16_FROM_INT8(query, header_length + 1);
	int num;

	/* In pre-reply we care about values that have been written */
	switch (functionCode)
	{
	case 5: // Write Single Coil
		events->writtenCoil(address);
		break;
	case 15: // Write Multiple Coils
		num = MODBUS_GET_INT16_FROM_INT8(query, header_length + 3);
		for (int i = address; i < address + num; i++)
		{
			events->writtenCoil(i);
		}
		break;
	case 6: // Write Single Holding Register;
		events->writtenHoldingRegister(address);
		break;
	case 16: // Write Multiple Holding Registers
		num = MODBUS_GET_INT16_FROM_INT8(query, header_length + 3);
		for (int i = address; i < address + num; i++)
		{
			events->writtenHoldingRegister(i);
		}
		break;
	}
}

bool ModbusServer::processEvents(struct timeval *timeout)
{
	rdset = refset;
	int ret = select(fdmax+1, &rdset, NULL, NULL, timeout);
	if (ret == -1)
	{
		int err = errno;
		throw err;
	}

	if (ret == 0)
	{
		return false;
	}

	for (int fd = 0; fd <= fdmax; fd++)
	{
		if (!FD_ISSET(fd, &rdset))
		{
			continue;
		}

		if (fd == server_socket)
		{
			/* A client is asking a new connection */
			socklen_t addrlen;
			struct sockaddr_in clientaddr;
			int newfd;

			/* Handle new connections */
			addrlen = sizeof(clientaddr);
			memset(&clientaddr, 0, sizeof(clientaddr));
			newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
			if (newfd == -1)
			{
				cerr << "Server accept() error: " << strerror(errno) << endl;
				continue;
			}
			else
			{
				FD_SET(newfd, &refset);

				if (newfd > fdmax)
				{
					/* Keep track of the maximum */
					fdmax = newfd;
				}
				Logger::logger(LOG_INFO, "New connection from %s:%d on socket %d\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
			}
		}
		else
		{
			modbus_set_socket(ctx, fd);
			int rc = modbus_receive(ctx, query);
			if (rc > 0)
			{
				if (events != NULL)
				{
					preReply(rc);
				}
				modbus_reply(ctx, query, rc, mb_mapping);
				if (events != NULL)
				{
					postReply(rc);
				}
			}
			else if (rc == -1)
			{
				/* This example server in ended on connection closing or
				 * any errors. */
				Logger::logger(LOG_INFO, "Connection closed on socket %d\n", fd);
				close(fd);

				/* Remove from reference set */
				FD_CLR(fd, &refset);

				if (fd == fdmax)
				{
					fdmax--;
				}
			}
		}
	}

	return true;
}

