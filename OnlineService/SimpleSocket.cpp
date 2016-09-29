#include "stdafx.h"
#include "SimpleSocket.h"

SimpleSocket::~SimpleSocket()
{
	closesocket(connectedSocket);
}

// Pass the port as string, so that getddrinfo can handle the conversion to network byte ordering
bool SimpleSocket::CreateConnection(std::string url, std::string port, bool tcpip)
{
	if (tcpip)
	{
		addr.ai_socktype = SOCK_STREAM;
		addr.ai_protocol = IPPROTO_TCP;
	}
	else
	{
		addr.ai_socktype = SOCK_DGRAM;
		addr.ai_protocol = IPPROTO_UDP;
	}

	addr.ai_flags = AI_CANONNAME;

	struct addrinfo *resultPtr;
	auto err = getaddrinfo(url.c_str(), port.c_str(), &addr, &resultPtr);
	std::shared_ptr<addrinfo> resultWrapper(resultPtr, freeaddrinfo);

	if (err != 0)
	{
		PrintError("Error getting address info");
		return false;
	}

	connectedSocket = socket(resultPtr->ai_family, resultPtr->ai_socktype, resultPtr->ai_protocol);
	if (connectedSocket == INVALID_SOCKET)
	{
		PrintError("Error connecting socket");
		return false;
	}
	
	//if (tcpip)
	{
		err = connect(connectedSocket, resultPtr->ai_addr, resultPtr->ai_addrlen);

		if (err != 0)
		{
			PrintError("Error connecting");
			return false;
		}
		printf("Connected to %s\n", resultPtr->ai_canonname);
	}

	return true;
}

bool SimpleSocket::Send(uint16_t msgType, msgpack::sbuffer& buffer)
{
	SimpleHeader authHeader;
	authHeader.msgType = msgType;
	authHeader.payloadSize = buffer.size();

	//ERRor Check
	auto err = send(connectedSocket, (char*)&authHeader, sizeof(SimpleHeader), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending header: ");
	}	
	err = send(connectedSocket, buffer.data(), buffer.size(), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending data: ");
	}

	return true;
}

bool SimpleSocket::Send(uint16_t msgType, char* buffer, uint32_t size)
{
	SimpleHeader authHeader;
	authHeader.msgType = msgType;
	authHeader.payloadSize = size;

	//ERRor Check
	auto err = send(connectedSocket, (char*)&authHeader, sizeof(SimpleHeader), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending header: ");
	}

	err = send(connectedSocket, buffer, size, 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending data: ");
	}

	return true;;
}

bool SimpleSocket::Receive(uint16_t& msgType, char* buffer, uint32_t& bufferSize)
{
	SimpleHeader header;
	auto err = recv(connectedSocket, (char*)&header, sizeof(header), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error receiving header: ");
	}
	msgType = header.msgType;

	if (header.payloadSize > bufferSize)
	{
		return false;
	}
	
	bufferSize = recv(connectedSocket, buffer, bufferSize, 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error receiving data: ");
	}
	
	if (bufferSize < 0)
	{
		return false;
	}

	return true;
}

void SimpleSocket::SetNonBlockingMode()
{
	u_long mode = 1;
	if (ioctlsocket(connectedSocket, FIONBIO, &mode))
	{
		PrintError("Error setting non blocking mode: %s");
	}
}

void SimpleSocket::PrintError(const char * msg)
{
		const int MAX_STRING_SIZE = 256;
		char buffer[MAX_STRING_SIZE];
		auto err = WSAGetLastError();
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
		printf("%s: %s", msg, buffer);
}