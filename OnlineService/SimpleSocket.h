#pragma once
#include "stdafx.h"

class SimpleSocket
{
protected:
	SOCKET connectedSocket;
	struct addrinfo addr;


	struct SimpleHeader
	{
		uint32_t payloadSize;
		uint16_t msgType;
	};
	
public:
	SimpleSocket() : connectedSocket(INVALID_SOCKET) 
	{
		memset(&addr, 0, sizeof(addr));
		addr.ai_family = AF_UNSPEC; //Allow IPv4 or IPv6
	};
	~SimpleSocket();

	bool CreateConnection(std::string url, std::string port, bool tcpip = true);

	bool Send(uint16_t msgType, msgpack::sbuffer& buffer);
	bool Send(uint16_t msgType, char* buffer, uint32_t size);
	bool Receive(uint16_t& msgType, char* buffer, uint32_t& bufferSize);

	void SetNonBlockingMode();

	void PrintError(const char * msg);
};