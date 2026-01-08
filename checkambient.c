/*
 * $Id: checkambient.c,v 1.4 2006/04/12 14:01:53 fab Exp $
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>


int main(void)
{
	APTR port;

	port = FindPort("Ambient IPC");

	if (port)
	{
		return (0);
	}
	else
	{
		Printf("Ambient is not running\n");
		return (1);
	}
}
