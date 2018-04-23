all: myplclogic

myplclogic: MyPlcLogic.o
	g++ -shared -o "myplclogic.so"  ./MyPlcLogic.o -lm

MyPlcLogic.o: MyPlcLogic.cpp MyPlcLogic.h ../../UniPLC/src/IIODeviceEvents.h ../../UniPLC/src/IModbusServerEvents.h ../../UniPLC/src/logger.h ../../UniPLC/src/IIODevice.h ../../UniPLC/src/IPlcLogic.h ../../UniPLC/src/ModbusServer.h
	g++ -fPIC -MMD -MP -c MyPlcLogic.cpp -I../../UniPLC/src/ -std=c++11

clean:
	rm -f *.o *.so

install: myplclogic
	mkdir -p /usr/share/uniplc/plugins/plc/ && cp myplclogic.so /usr/share/uniplc/plugins/plc/

