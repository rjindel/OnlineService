// OnlineService.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "SimpleSocket.h"
#include "QosSocket.h"

char CharToHex(char character)
{
	character = tolower(character);
	if (!isalnum(character))
	{
		throw "invalid string";
	}

	char temp = 0;
	if (isdigit(character))
	{
		temp = character - '0';
	}
	else
	{
		temp = character - 'a' + 10;
	}

	return temp;
}

void StringToHex(const char* str, BYTE* hex)
{
	if (!str || !hex)
	{
		throw "invalid parameters";
	}

	while (*str != 0 && *str + 1 != 0)// && *hex != 0)
	{
		if (*str == '-')
		{
			//Handle uuid's. Ideally this belongs in a higher level function, but as we have simple use case, it's easier to handle here.
			//As the message headers are little-endian, we don't need to endian conversion
			str++;
			continue;
		}

		char hiNibble = CharToHex(*str);
		str++;

		char loNibble = CharToHex(*str);

		*hex = (hiNibble << 4) + loNibble;
		hex++;
		str++;
	}
}

int main()
{
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//TODO: better error handling. Minimally output the type of error we got from err
	if (err != 0)
	{
		printf("Error creating socket");
		return -1;
	}

	if (true)
	{
		int port = 27016;
		const uint32_t numConnections = 1;
		QosSocket socketsArray[numConnections];
		for (uint32_t i = 0; i < numConnections; i++)
		{
			QosSocket& socket = socketsArray[i];
			socket.CreateConnection("127.0.0.1", std::to_string(port + i), false);
			socket.SetNonBlockingMode();
			socket.StartMeasuringQos();
		}
		getchar();
		//Waitforthreads;
		for (auto socket : socketsArray)
		{
			socket.StopMeasuring();
		}

		WSACleanup();
		return 0;
	}
	
	int authPort = 12704;
	char* authUrl = "auth.rcr.experiments.fireteam.net";

	SimpleSocket authSocket;
	if ( !authSocket.CreateConnection(authUrl, std::to_string(authPort)) )
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}

	//Convert string to binary data
	char * clientID = "45f67935-9286-4ba0-8b3b-70228e727ca2";
	char * clientSecret = "1eba4dac53c33ae135fe7b2a839eb30aec964f75";
	
	const int UUID_LENGTH_IN_BYTES = 16;
	const int MAX_PAYLOADSIZE = 256;
	
	BYTE clientUUID[UUID_LENGTH_IN_BYTES];
	constexpr int maxSecretLength = MAX_PAYLOADSIZE - UUID_LENGTH_IN_BYTES;
	BYTE clientSecretBinary[maxSecretLength];
	memset(clientSecretBinary, 0, maxSecretLength);

	StringToHex(clientID, &clientUUID[0]);
	StringToHex(clientSecret, &clientSecretBinary[0]);
	int secretLength = 0;
	while (clientSecretBinary[secretLength] != 0) secretLength++;

	// Create Header
	enum class AuthMessageType : uint16_t
	{
		Error = 0,
		AuthTicketRequest = 1, 
		AuthTicketResponse = 2
	};

	//MsgPack client id and client secret
	msgpack::sbuffer streamBuffer;
	msgpack::packer<msgpack::sbuffer> packer(&streamBuffer);
	packer.pack_bin(UUID_LENGTH_IN_BYTES + secretLength);
	packer.pack_bin_body((const char *)clientUUID, UUID_LENGTH_IN_BYTES);
	packer.pack_bin_body((const char *)clientSecretBinary, secretLength);

	authSocket.Send(static_cast<uint16_t>(AuthMessageType::AuthTicketRequest), streamBuffer);

	uint16_t messageType = 0;
	uint32_t tokenLength = MAX_PAYLOADSIZE;
	char authToken[MAX_PAYLOADSIZE];
	if(!authSocket.Receive(messageType, authToken, tokenLength))
	{
		printf("Error response expected");
		return -1;
	}

	if ((AuthMessageType) messageType != AuthMessageType::AuthTicketResponse)
	{
		printf("Error expected Auth Token");
		return -1;
	}
	
	int apiPort = 12705;
	char* apiUrl = "api.rcr.experiments.fireteam.net";

	SimpleSocket apiSocket;
	if (!apiSocket.CreateConnection(apiUrl, std::to_string(apiPort)))
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}


	// Create Header
	enum class APIMessageType : UINT16
	{
		Error = 0,
		Authorize = 1, 
		GetServers = 2,
		GetServerResponse = 3,
	};

	if (!apiSocket.Send(static_cast<uint16_t>(APIMessageType::Authorize), authToken, tokenLength))
	{
		return -1;
	}

	if (!apiSocket.Send(static_cast<uint16_t>(APIMessageType::GetServers), nullptr, 0))
	{
		return -1;
	}
	
	uint32_t payloadSize = MAX_PAYLOADSIZE;
	char payload[MAX_PAYLOADSIZE];
	if (!apiSocket.Receive(messageType, payload, payloadSize))
	{
		return -1;
	}

	if ((APIMessageType)messageType != APIMessageType::GetServerResponse)
	{
		printf("expected server reponse");
		return -1;
	}

	msgpack::object_handle objectHandle = msgpack::unpack((const char*)&payload[0], payloadSize);
	msgpack::object serverObjects = objectHandle.get();
	
	std::vector<QosSocket> qosServers(serverObjects.via.array.size);

	for(uint32_t i = 0; i < serverObjects.via.array.size; i++)
	{

		msgpack::type::tuple<std::string, int> serverData;
		serverObjects.via.array.ptr[i].convert(serverData);

		std::string servername = serverData.get<0>();
		int port = serverData.get<1>();
		printf("Address %s : %i \n", servername.c_str(), port);

		QosSocket qosSocket;
		qosSocket.CreateConnection(servername, std::to_string(port), false);
		qosSocket.SetNonBlockingMode();
		qosSocket.StartMeasuringQos();
	}
	
	WSACleanup();
    return 0;
}

