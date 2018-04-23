all: uniplc

uniplc: logger.o  main.o  ModbusServer.o  UniPLC.o
	g++ -o uniplc logger.o  main.o  ModbusServer.o  UniPLC.o -lmodbus -ldl -lconfig++

logger.o: logger.cpp logger.h
	g++ -O2 -Wall -c logger.cpp

UniPLC.o: UniPLC.cpp UniPLC.h IPlcLogic.h UniPLC.h
	g++ -O2 -Wall -c UniPLC.cpp

ModbusServer.o: ModbusServer.cpp ModbusServer.h IModbusServerEvents.h
	g++ -O2 -Wall -c ModbusServer.cpp

main.o: main.cpp
	g++ -O2 -Wall -c main.cpp

clean:
	rm -f *.o uniplc

install: uniplc
	cp uniplc /usr/bin/

