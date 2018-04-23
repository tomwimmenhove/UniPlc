all: u12iodevice

u12iodevice: U12IODevice.o
	g++ -shared -o "u12iodevice.so"  ./U12IODevice.o -lm

U12IODevice.o: U12IODevice.cpp U12IODevice.h ../../UniPLC/src/IIODeviceEvents.h ../../UniPLC/src/IModbusServerEvents.h ../../UniPLC/src/logger.h ../../UniPLC/src/IIODevice.h ../../UniPLC/src/IPlcLogic.h ../../UniPLC/src/ModbusServer.h
	g++ -fPIC -MMD -MP -c U12IODevice.cpp -I../../UniPLC/src/ -std=c++11

clean:
	rm -f *.o *.so

install: u12iodevice
	mkdir -p /usr/share/uniplc/plugins/iodevices/ && cp u12iodevice.so /usr/share/uniplc/plugins/iodevices/

