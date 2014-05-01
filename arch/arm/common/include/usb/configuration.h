/******************************************************************************
*	configuration.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	configuration.h contains definitions for all optional components
*	The makefile defines three main categories of definitions:
*		CONFIG: Whether or not this is a DEBUG driver
*		TARGET: The target system
*		TYPE:	What sort of driver to compile (e.g. standalone)
*****************************************************************************/

#define DEBUG 1
#define TARGET_RPI

#define ARM
#define ARM_V6
#define ENDIAN_LITTLE
#define BROADCOM_2835
#define HCD_DESIGNWARE_20
#define HCD_DESIGNWARE_BASE ((void*)0x90980000)
//#define MEM_INTERNAL_MANAGER
#define MEM_NO_RESERVE
#define LIB_BCM2835
#define LIB_DWC
#define LIB_HID
#define LIB_HUB
#define LIB_KBD
#define LIB_MOUSE
