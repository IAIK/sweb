#ifndef MTOOLS_FLOPPYDIO_H
#define MTOOLS_FLOPPYDIO_H

#ifdef USE_FLOPPYD

#include "stream.h"

/*extern int ConnectToFloppyd(const char* name, Class_t** ioclass);*/
Stream_t *FloppydOpen(struct device *dev, struct device *dev2,
					  char *name, int mode, char *errmsg,
					  int mode2, int locked);

#define FLOPPYD_DEFAULT_PORT 5703

#define FLOPPYD_PROTOCOL_VERSION_OLD 10
#define FLOPPYD_PROTOCOL_VERSION 11

#define FLOPPYD_CAP_EXPLICIT_OPEN 1 /* explicit open. Useful for 
				     * clean signalling of readonly disks */
#define FLOPPYD_CAP_LARGE_SEEK 2    /* large seeks */

enum FloppydOpcodes {
	OP_READ,
	OP_WRITE,
	OP_SEEK,
	OP_FLUSH,
	OP_CLOSE,
	OP_IOCTL,
	OP_OPRO,
	OP_OPRW
};

enum AuthErrorsEnum {
	AUTH_SUCCESS,
	AUTH_PACKETOVERSIZE,
	AUTH_AUTHFAILED,
	AUTH_WRONGVERSION,
	AUTH_DEVLOCKED,
	AUTH_BADPACKET
};

typedef unsigned long IPaddr_t;

#endif
#endif
