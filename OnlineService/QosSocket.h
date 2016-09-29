#pragma once
#include "stdafx.h"
#include "SimpleSocket.h"

class QosSocket;

struct SocketInfo
{
	WSAOVERLAPPED overlapped;
	QosSocket* thisPtr;
};

struct QosPacket
{
	char header[2] = { (char)0xff };
	uint32_t instanceId;
	uint32_t packetId;
	//Timestamp
};

class QosSocket : public SimpleSocket
{
	uint32_t	packetsSent;
	uint32_t	packetsLost;
	uint32_t	averagePing;

	HANDLE thread;

	LARGE_INTEGER startTime, endTime, frequency;
	LARGE_INTEGER accumulator = { 0 };
	static uint32_t instanceCounter;
	uint32_t instanceId;

	SocketInfo socketInfo;

	static const int MAX_PAYLOADSIZE = 256;
	char buffer[MAX_PAYLOADSIZE];

	bool exit;
public:
	QosSocket();

	void StartMeasuringQos();
	void StopMeasuring();

	static DWORD CALLBACK Measure(LPVOID threadParameter);
	void MeasureLoop();
	static void CALLBACK RecvCallback(DWORD , DWORD, LPWSAOVERLAPPED, DWORD);
};
