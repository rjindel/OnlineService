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

/*DWORD CALLBACK*/
void ListenFunction(const int threadId)
{
	addrinfo addressInfo = {};
	addressInfo.ai_family = AF_INET; // AF_UNSPEC;
	addressInfo.ai_socktype = SOCK_DGRAM;
	addressInfo.ai_protocol = IPPROTO_UDP;
	
	const uint32_t MAX_PACKETSIZE = 256;
	char buffer[MAX_PACKETSIZE];

	int port = 27015 + threadId;
	int addrSize = sizeof(sockaddr);
	
	addrinfo* resultPtr = nullptr;
	//sockaddr_in incomingConnection = {0};

	//incomingConnection.sin_family = AF_INET;
	//incomingConnection.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//incomingConnection.sin_port = htons(port);	
	//
	//sockaddr incomingConnection = {0};
	//incomingConnection.sa_family = AF_INET;
	//incomingConnection.sa_data

	auto err = getaddrinfo("0.0.0.0", std::to_string(port).c_str(), &addressInfo, &resultPtr);
	PrintError(err);

	//SOCKET listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	SOCKET listenSocket = socket(resultPtr->ai_family, resultPtr->ai_socktype, resultPtr->ai_protocol);

	//err = bind(listenSocket, (sockaddr*)&incomingConnection, sizeof(incomingConnection));
	err = bind(listenSocket, resultPtr->ai_addr, resultPtr->ai_addrlen);

	freeaddrinfo(resultPtr);
	while (true)
	{
		//int bytesRead = recvfrom(listenSocket, &buffer[0], MAX_PACKETSIZE, 0, (sockaddr*)&incomingConnection, &addrSize);
		int bytesRead = recvfrom(listenSocket, &buffer[0], MAX_PACKETSIZE, 0, resultPtr->ai_addr,  (int*)&resultPtr->ai_addrlen);
		if (bytesRead == SOCKET_ERROR)
		{
			const int MAX_STRING_SIZE = 256;
			char buffer[MAX_STRING_SIZE];
			auto err = WSAGetLastError();
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
			printf("Error from recv: %s", buffer);
			WSACleanup();
		}
		printf("%i bytes Received\n", bytesRead);
		Sleep(800);
		int bytesSent = sendto(listenSocket, buffer, bytesRead, 0, (sockaddr*)  resultPtr->ai_addr, resultPtr->ai_addrlen);
		//int bytesSent = sendto(listenSocket, buffer, bytesRead, 0, (sockaddr*) &incomingConnection, addrSize);
		if (bytesSent == SOCKET_ERROR)
		{
			const int MAX_STRING_SIZE = 256;
			char buffer[MAX_STRING_SIZE];
			auto err = WSAGetLastError();
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
			printf("Error from Sendto: %s", buffer);
			WSACleanup();
		}
		printf("%i bytes Sent\n", bytesSent);
	}
}

int main()
{
	WSADATA wsaData = {};

	auto err = WSAStartup(MAKEWORD(2, 2), &wsaData);

	const int numConnections = 1;
	std::thread threads[numConnections];

	for (int i = 0; i < numConnections; ++i)
	{
		//threads[i] = CreateThread(nullptr, 0, ListenFunction, &i, 0, nullptr);
		threads[i] = std::thread(ListenFunction, i);
	}

//	WaitForMultipleObjects(numConnections, threads, true, INFINITE);
//
//	for (int i = 0; i < numConnections; ++i)
//	{
//		CloseHandle(threads[i]);
//	}

	WSACleanup();

    return 0;
}

