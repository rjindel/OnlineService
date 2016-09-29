// TestServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <thread>

void PrintError(DWORD err)
{
	const int MAX_STRING_SIZE = 256;
	char buffer[MAX_STRING_SIZE];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
	printf("Error: %s", buffer);
}

void PrintWSAError()
{
	auto err = WSAGetLastError();
	PrintError(err);
}

void ListenFunction(const int threadId)
{
	addrinfo addressInfo = {};
	addressInfo.ai_flags = AI_CANONNAME;
	addressInfo.ai_family = AF_INET; // AF_UNSPEC;
	addressInfo.ai_socktype = SOCK_DGRAM;
	addressInfo.ai_protocol = IPPROTO_UDP;

	const uint32_t MAX_PACKETSIZE = 256;
	char buffer[MAX_PACKETSIZE];

	int port = 27015 + threadId;
	int addrSize = sizeof(sockaddr);

	addrinfo* resultPtr = nullptr;
	sockaddr_in incomingAddress = {0};
	int incomingAddressLength = sizeof(incomingAddress);

	auto err = getaddrinfo("0.0.0.0", std::to_string(port).c_str(), &addressInfo, &resultPtr);
	PrintError(err);

	SOCKET listenSocket = socket(resultPtr->ai_family, resultPtr->ai_socktype, resultPtr->ai_protocol);

	err = bind(listenSocket, resultPtr->ai_addr, resultPtr->ai_addrlen);
	printf("Binding to %s\n", resultPtr->ai_canonname);

	freeaddrinfo(resultPtr);
	while (true)
	{
		int bytesRead = recvfrom(listenSocket, &buffer[0], MAX_PACKETSIZE, 0, (sockaddr*)&incomingAddress, &incomingAddressLength);
		if (bytesRead == SOCKET_ERROR)
		{
			const int MAX_STRING_SIZE = 256;
			char buffer[MAX_STRING_SIZE];
			auto err = WSAGetLastError();
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
			printf("Error from recv: %s", buffer);
			WSACleanup();
			return;
		}
		printf("%i bytes Received from %s\n", bytesRead, inet_ntoa(incomingAddress.sin_addr));
		Sleep(800);
		int bytesSent = sendto(listenSocket, buffer, bytesRead, 0, (sockaddr*) &incomingAddress, incomingAddressLength);
		if (bytesSent == SOCKET_ERROR)
		{
			const int MAX_STRING_SIZE = 256;
			char buffer[MAX_STRING_SIZE];
			auto err = WSAGetLastError();
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
			printf("Error from Sendto: %s", buffer);
			WSACleanup();
			return;
		}
		printf("%i bytes Sent to %s \n", bytesSent, inet_ntoa(incomingAddress.sin_addr));
	}
}

int main()
{
	WSADATA wsaData = {};

	auto err = WSAStartup(MAKEWORD(2, 2), &wsaData);

	const int numConnections = 3;
	std::thread threads[numConnections];

	for (int i = 0; i < numConnections; ++i)
	{
		threads[i] = std::thread(ListenFunction, i);
	}

//	WaitForMultipleObjects(numConnections, threads, true, INFINITE);
//
	for (int i = 0; i < numConnections; ++i)
	{
		threads[i].join();
	}

	WSACleanup();

    return 0;
}

