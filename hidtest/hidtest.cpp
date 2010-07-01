/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"


int main(int argc, char* argv[])
{
	int res;
	unsigned char buf[65];
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	int handle;
	int i;

#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif

	struct hid_device *devs, *cur_dev;
	
	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("\n");
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);

	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;
	

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x4d8, 0x3f, NULL);
	if (handle < 0) {
		printf("unable to open device\n");
		wprintf(L"Error: %s\n", hid_error(handle));
	}

	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	wprintf(L"Manufacturer String: %s\n", wstr);

	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	wprintf(L"Product String: %s\n", wstr);

	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	wprintf(L"Serial Number String: (%d) %s\n", wstr[0], wstr);

	// Read Indexed String 1
	wstr[0] = 0x0000;
	res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read indexed string 1\n");
	wprintf(L"Indexed String 1: %s\n", wstr);

	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);
	
	// Try to read from the device. There shoud be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);

	// Toggle LED (cmd 0x80). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 17);
	if (res < 0) {
		printf("Unable to write()\n");
		printf("Error: %s\n", hid_error(handle));
	}
	

	// Request state (cmd 0x81). The first byte is the report number (0x1).
	buf[0] = 0x1;
	buf[1] = 0x81;
	hid_write(handle, buf, 17);
	if (res < 0)
		printf("Unable to write() (2)\n");

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	while (res == 0) {
		res = hid_read(handle, buf, 17);
		if (res == 0)
			printf("waiting...\n");
		if (res < 0)
			printf("Unable to read()\n");
	}

	// Print out the returned buffer.
	for (i = 0; i < res; i++)
		printf("%02hhx ", buf[i]);
	printf("\n");

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
