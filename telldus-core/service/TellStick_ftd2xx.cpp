//
// C++ Implementation: TellStick
//
// Description: 
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "TellStick.h"
#include "Mutex.h"
#include "../client/telldus-core.h"
#include <string.h>

#include "ftd2xx.h"

class TellStick::PrivateData {
public:
	bool open, running;
	int vid, pid, fwVersion;
	std::string serial, message;
	FT_HANDLE ftHandle;
	TelldusCore::Mutex mutex;

	HANDLE eh;

};

TellStick::TellStick(Event *event, const TellStickDescriptor &td ) 
	:Controller(event)
{
	d = new PrivateData;
	d->eh = CreateEvent( NULL, false, false, NULL );
	d->open = false;
	d->running = false;
	d->vid = td.vid;
	d->pid = td.pid;
	d->fwVersion = 0;
	d->serial = td.serial;

	char *tempSerial = new char[td.serial.size()+1];
#ifdef _WINDOWS
	strcpy_s(tempSerial, td.serial.size()+1, td.serial.c_str());
#else
	strcpy(tempSerial, td.serial.c_str());
	FT_SetVIDPID(td.vid, td.pid);
#endif
	FT_STATUS ftStatus = FT_OpenEx(tempSerial, FT_OPEN_BY_SERIAL_NUMBER, &d->ftHandle);
	delete tempSerial;
	if (ftStatus == FT_OK) {
		d->open = true;
		FT_SetFlowControl(d->ftHandle, FT_FLOW_NONE, 0, 0);
		FT_SetTimeouts(d->ftHandle,5000,0);
	}
	
	if (d->open) {
		if (td.pid == 0x0C31) {
			setBaud(9600);
		} else {
 			setBaud(4800);
		}
		this->start();
	}
}

TellStick::~TellStick() {
	if (d->running) {
		TelldusCore::MutexLocker locker(&d->mutex);
		d->running = false;
		SetEvent(d->eh);
	}
	this->wait();
	if (d->open) {
	}
	delete d;
}

void TellStick::setBaud( int baud ) {
	FT_SetBaudRate(d->ftHandle, baud);
}

int TellStick::firmwareVersion() {
	return d->fwVersion;
}

bool TellStick::isOpen() const {
	return d->open;
}

bool TellStick::isSameAsDescriptor(const TellStickDescriptor &td) const {
	if (td.vid != d->vid) {
		return false;
	}
	if (td.pid != d->pid) {
		return false;
	}
	if (td.serial != d->serial) {
		return false;
	}
	return true;
}

void TellStick::processData( const std::string &data ) {
	for (unsigned int i = 0; i < data.length(); ++i) {
		if (data[i] == 13) { // Skip \r
			continue;
		} else if (data[i] == 10) { // \n found
			if (d->message.substr(0,2).compare("+V") == 0) {
				//TODO save the firmware version
			} else if (d->message.substr(0,2).compare("+R") == 0) {
				this->publishData(d->message);
			}
			d->message.clear();
		} else { // Append the character
			d->message.append( 1, data[i] );
		}
	}
}

void TellStick::run() {
	d->running = true;
	DWORD dwBytesInQueue = 0;
	DWORD dwBytesRead = 0;
	char *buf = 0;

	while(1) {
		FT_SetEventNotification(d->ftHandle, FT_EVENT_RXCHAR, d->eh);
		WaitForSingleObject(d->eh,INFINITE);

		TelldusCore::MutexLocker locker(&d->mutex);
		if (!d->running) {
			break;
		}
		FT_GetQueueStatus(d->ftHandle, &dwBytesInQueue);
		if (dwBytesInQueue < 1) {
			d->mutex.unlock();
			continue;
		}
		buf = (char*)malloc(sizeof(buf) * (dwBytesInQueue+1));
		memset(buf, 0, dwBytesInQueue+1);
		FT_Read(d->ftHandle, buf, dwBytesInQueue, &dwBytesRead);
		processData( buf );
		free(buf);
	}
}

int TellStick::send( const std::string &strMessage ) {
	if (!d->open) {
		return TELLSTICK_ERROR_NOT_FOUND;
	}
	bool c = true;

	//This lock does two things
	// 1 Prevents two calls from different threads to this function
	// 2 Prevents our running thread from receiving the data we are interested in here
	TelldusCore::MutexLocker(&d->mutex);

	char *tempMessage = (char *)malloc(sizeof(char) * (strMessage.size()+1));
#ifdef _WINDOWS
	strcpy_s(tempMessage, strMessage.size()+1, strMessage.c_str());
#else
	strcpy(tempMessage, strMessage.c_str());
#endif

	ULONG bytesWritten, bytesRead;
	char in;
	FT_STATUS ftStatus;
	ftStatus = FT_Write(d->ftHandle, tempMessage, (DWORD)strMessage.length(), &bytesWritten);
	free(tempMessage);

	while(c) {
		ftStatus = FT_Read(d->ftHandle,&in,1,&bytesRead);
		if (ftStatus == FT_OK) {
			if (bytesRead == 1) {
				if (in == '\n') {
					break;
				}
			} else { //Timeout
				c = false;
			}
		} else { //Error
			c = false;
		}
	}

	if (!c) {
		return TELLSTICK_ERROR_COMMUNICATION;
	}
	return TELLSTICK_SUCCESS;
}

bool TellStick::stillConnected() const {
	FT_STATUS ftStatus;
	DWORD numDevs;
	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus != FT_OK) {
		return false;
	}
	if (numDevs <= 0) {
		return false;
	}
	for (int i = 0; i < (int)numDevs; i++) {
		FT_HANDLE ftHandleTemp;
		DWORD flags;
		DWORD id;
		DWORD type;
		DWORD locId;
		char serialNumber[16];
		char description[64];
		// get information for device i
		ftStatus = FT_GetDeviceInfoDetail(i, &flags, &type, &id, &locId, serialNumber, description, &ftHandleTemp);
		if (ftStatus != FT_OK) {
			continue;
		}
		if (d->serial.compare(serialNumber) == 0) {
			return true;
		}
	}
	return false;
}

std::list<TellStickDescriptor> TellStick::findAll() {
	std::list<TellStickDescriptor> tellstick = findAllByVIDPID(0x1781, 0x0C30);

	std::list<TellStickDescriptor> duo = findAllByVIDPID(0x1781, 0x0C31);
	for(std::list<TellStickDescriptor>::const_iterator it = duo.begin(); it != duo.end(); ++it) {
		tellstick.push_back(*it);
	}

	return tellstick;
	
}

std::list<TellStickDescriptor> TellStick::findAllByVIDPID( int vid, int pid ) {
	std::list<TellStickDescriptor> retval;
	
	FT_HANDLE fthHandle = 0;
	FT_STATUS ftStatus = FT_OK;
	DWORD dwNumberOfDevices = 0;

#ifndef _WINDOWS
	FT_SetVIDPID(vid, pid);
#endif

	ftStatus = FT_CreateDeviceInfoList(&dwNumberOfDevices);
	if (ftStatus != FT_OK) {
		return retval;
	}
	for (int i = 0; i < (int)dwNumberOfDevices; i++) { 
		FT_PROGRAM_DATA pData;
		char ManufacturerBuf[32]; 
		char ManufacturerIdBuf[16]; 
		char DescriptionBuf[64]; 
		char SerialNumberBuf[16]; 

		pData.Signature1 = 0x00000000;
		pData.Signature2 = 0xffffffff;
		pData.Version = 0x00000002;      // EEPROM structure with FT232R extensions
		pData.Manufacturer = ManufacturerBuf; 
		pData.ManufacturerId = ManufacturerIdBuf;
		pData.Description = DescriptionBuf; 
		pData.SerialNumber = SerialNumberBuf; 

		ftStatus = FT_Open(i, &fthHandle);
		ftStatus = FT_EE_Read(fthHandle, &pData);
		ftStatus = FT_Close(fthHandle);
		if (ftStatus != FT_OK) {
			continue;
		}
		if (pData.VendorId == vid && pData.ProductId == pid) {
			TellStickDescriptor td;
			td.vid = vid;
			td.pid = pid;
			td.serial = pData.SerialNumber;
			retval.push_back(td);
		}
	}
	return retval;
}