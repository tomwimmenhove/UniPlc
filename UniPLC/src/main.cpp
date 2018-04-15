#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UniPLC.h"

int main(int argc, char **argv)
{
	UniPLC* plc;
	int sig;
	int ret;

	do
	{
		try
		{
			plc = new UniPLC(argc, argv);
		}
		catch(int err)
		{
			return err;
		}
		catch(...)
		{
			fprintf(stderr, "Unknown fatal error.\n");
			return 1;
		}

		/* Run the PLC */
		ret = plc->run();
		if (ret != 0)
		{
			break;
		}

		/* Check for signal */
		sig = plc->getLastSignal();

		delete plc;
	} while (sig == SIGHUP);

	return ret;
}
