#pragma once 

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")

#define TCP_MODE SOCK_STREAM
#define UDP_MODE SOCK_DGRAM

typedef char*(__cdecl* ResponseFunction)(char*);

class TCPServer
{
public:
	TCPServer()
	{
		RequestBuffer = (char*)malloc(max_client_buffer_size);
		timeout.tv_sec = 180;
		timeout.tv_usec = 0;
	}
	~TCPServer()
	{
		closesocket(listen_socket);
		WSACleanup();
	}

	void start_server(const char* address, const char* start_port, LPCSTR* requestpatterns, void* functionreponses, int size)
	{
		patterns = requestpatterns;
		responses = reinterpret_cast<ResponseFunction*>(functionreponses);
		sizeofdata = size;
			
		if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		{
			WSAStartup(MAKEWORD(wsaData.wVersion, wsaData.wHighVersion), &wsaData);
		}

		ZeroMemory(&connection, sizeof(addrinfo));

		connection.ai_family = AF_INET;
		connection.ai_socktype = TCP_MODE;

		int result = getaddrinfo(address, start_port, &connection, &connection_ptr);

		if (result != 0)
		{
			std::string error_string = "getaddrinfo()->failed\nError Code: " + std::to_string(result);
			std::cout << error_string << std::endl;
			this->~TCPServer();
			return;
		}

		listen_socket = socket(connection_ptr->ai_family, connection_ptr->ai_socktype, 0);

		if (listen_socket == INVALID_SOCKET)
		{
			std::string error_string = "listen_socket()->invalid_socket\nError Code: " + std::to_string(WSAGetLastError());
			std::cout << error_string << std::endl; 
			this->~TCPServer();
			return;
		}

		result = bind(listen_socket, connection_ptr->ai_addr, connection_ptr->ai_addrlen);

		if (WSAGetLastError() == 0x271d)
		{
			this->~TCPServer();
			int current_port = atoi(start_port);
			std::string current_port_str = std::to_string(++current_port);
			this->start_server(address, current_port_str.c_str(), requestpatterns, functionreponses, size);
		}

		if (result == SOCKET_ERROR)
		{
			std::string error_string = "bind()->socket_error\nError Code: " + std::to_string(WSAGetLastError());
			std::cout << error_string << std::endl; 
			this->~TCPServer();
			return;
		}

		result = listen(listen_socket, SOMAXCONN);
		
		if (result == SOCKET_ERROR)
		{
			std::string error_string = "listen()->socket_error\nError Code: " + std::to_string(WSAGetLastError());
			std::cout << error_string << std::endl; 
			this->~TCPServer();
			return;
		}

		// non-blocking sockets
		bool value = true;
		ioctlsocket(listen_socket, FIONBIO, (unsigned long*)&value);
		FD_ZERO(&master_set);
		FD_SET(listen_socket, &master_set);
		max_socket = listen_socket;
		//

		std::cout << "TCPServer is launched on port " << start_port << std::endl << std::endl;

		this->HandlerMain();
	}
private:
	void HandlerMain()
	{
		while (true)
		{
			memcpy(&working_set, &master_set, sizeof(master_set));

			int descriptors = select(NULL, &working_set, NULL, NULL, &timeout);

			for (int i = 0; i <= max_socket && descriptors != 0; ++i)
			{
				if (FD_ISSET(i, &working_set))
				{
					// if listening socket accepting connections else reading sending
					if (i == listen_socket)
					{
						while (true)
						{
							int new_socket = accept(listen_socket, NULL, NULL);

							if (new_socket == -1)
							{
								break;
							}

							FD_SET(new_socket, &master_set);

							if (new_socket > max_socket)
							{
								max_socket = new_socket;
							}
						}
					}
					else
					{
						ZeroMemory(RequestBuffer, max_client_buffer_size);

						std::string full_request;

						int recieved_bytes = 0;

						do
						{
							recieved_bytes = recv(i, RequestBuffer, max_client_buffer_size, 0);
							std::string data_part(RequestBuffer);
							full_request += data_part;
							ZeroMemory(RequestBuffer, max_client_buffer_size);
						} while (recieved_bytes > 0);

						std::cout << "TCP connection accepted." << std::endl
							<< std::endl << "Request data: " << full_request << std::endl << std::endl;

						char* datapointer = ResponseData((char*)full_request.c_str());

						std::cout << "Response data: " << datapointer << std::endl << std::endl;

						send(i, datapointer, strlen(datapointer), 0);

						Sleep(500);

						closesocket(i);

						FD_CLR(i, &master_set);

						if (i == max_socket)
						{
							while (!FD_ISSET(--max_socket, &master_set));
						}
					}
				}
			}
		}
	}

	char* ResponseData(char* request)
	{
		for (int i = 0; i < sizeofdata; i++)
		{
			if (strstr(request, patterns[i]))
			{
				return responses[i](request);
			}
		}
		return (char*)"Unknown request!";
	}

	WSADATA wsaData;
	fd_set master_set, working_set;
	struct timeval timeout;
	addrinfo connection;
	addrinfo* connection_ptr = NULL;
	LPCSTR* patterns = NULL;
	ResponseFunction* responses = NULL;
	int listen_socket = 0, sizeofdata = 0, max_socket = 0;
	char* RequestBuffer = NULL;
	const int max_client_buffer_size = 1024;
};