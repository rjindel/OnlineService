#include "stdafx.h"

#include "QosSocket.h"

uint32_t QosSocket::instanceCounter = 0;

QosSocket::QosSocket() : instanceId(instanceCounter++), 
	exit(false),
	packetsLost(0),
	packetsSent(0)
{
	memset(buffer, 0xff, MAX_PAYLOADSIZE);
}

void QosSocket::StartMeasuringQos()
{
	QueryPerformanceFrequency(&frequency);

	socketInfo.thisPtr = this;
	memset(&socketInfo.overlapped, 0, sizeof(WSAOVERLAPPED));
	socketInfo.overlapped.hEvent = WSACreateEvent();
	WSAResetEvent(socketInfo.overlapped.hEvent);

	QosPacket* packet = (QosPacket*)buffer;		//We could use a union instead here
	packet->instanceId = instanceId;

	thread = CreateThread(nullptr, 0, &Measure, this, 0, nullptr);
}

void QosSocket::StopMeasuring()
{
	exit = true;
	WaitForSingleObject(thread, INFINITE);
	WSACloseEvent(socketInfo.overlapped.hEvent);
	CloseHandle(thread);
}

DWORD QosSocket::Measure(LPVOID threadParameter)
{ 
	QosSocket *thisPtr = (QosSocket*)threadParameter;

	while (!thisPtr->exit)
	{
		thisPtr->MeasureLoop();
	}

	return 0;
}

void QosSocket::MeasureLoop()
{
	DWORD timeOut = 5000;
	DWORD recvBytes = 0, flags = 0;
	QosPacket* packet = (QosPacket*)buffer;		//We could use a union instead here
	packet->packetId = packetsSent;

	WSABUF wsaBuffer = { MAX_PAYLOADSIZE, &buffer[0] };

	QueryPerformanceCounter(&startTime);
	//auto err = send(connectedSocket, buffer, MAX_PAYLOADSIZE , 0);
	WSASend(connectedSocket, &wsaBuffer, 1, nullptr, 0, &socketInfo.overlapped, 0);
	auto err = WSAGetLastError();
	PrintError("Sending packet: ");
	packetsSent++;

	DWORD result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, false);
	//WSAGetOverlappedResult(connectedSocket, &socketInfo.overlapped, &recvBytes, true, &flags);
	WSAResetEvent(socketInfo.overlapped.hEvent);

	//if (result == WSA_WAIT_IO_COMPLETION)
	if (result != WSA_WAIT_TIMEOUT)
	{
		//err = WSARecv(connectedSocket, &wsaBuffer, 0, nullptr, 0, nullptr, RecvCallback);
		do {
			WSARecv(connectedSocket, &wsaBuffer, 1, &recvBytes, &flags, &socketInfo.overlapped, RecvCallback);
			err = WSAGetLastError();
		} while (err == WSA_IO_PENDING);
		PrintError("Receiving packet: ");
	
		result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, true);
		//PrintError("Receiving packet: ");
		WSAResetEvent(socketInfo.overlapped.hEvent);
	}
	averagePing = accumulator.QuadPart * 1000 / frequency.QuadPart;
}

void QosSocket::RecvCallback(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	auto err = WSAGetLastError();
	if (dwError == WSA_WAIT_IO_COMPLETION || dwError == 0)
	{
		SocketInfo *socketInfo = (SocketInfo*)overlapped;
		QueryPerformanceCounter(&socketInfo->thisPtr->endTime);

		socketInfo->thisPtr->accumulator.QuadPart += socketInfo->thisPtr->endTime.QuadPart - socketInfo->thisPtr->startTime.QuadPart;
	}
}