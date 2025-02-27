/*
* server.cpp
* name: Zong Weixu
* id:   3180102776
* compile: g++ server.cpp -o server -lwsock32 -lws2_32
*/

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <process.h>

#include "package.h"
#include "INIReader.h"

#pragma comment(lib, "Ws2_32")

unsigned __stdcall connection(void *ipParameter);
sockaddr_in removeClient(SOCKET client_addr);

struct ClientSocket
{
    SOCKET client_socket;
    sockaddr_in client_addr;

    ClientSocket();
    ClientSocket(SOCKET socket, sockaddr_in addr)
    {
        client_socket = socket;
        client_addr = addr;
    }
};

/*
* server's basic infor default as below
* config server's ip, port, concurrency and name in config.ini
*/

const char *server_name = "myserver";
const char *server_addr = "127.0.0.1";
int MAX_CONNECTION = 20;
const char *port = "2776";
std::vector<ClientSocket> clients_queue;

HANDLE h_mutex = CreateMutex(NULL, FALSE, NULL);

class Server
{
private:
    SOCKET server_socket;

public:
    Server();
    ~Server();
    void start();
};

Server::Server()
{
}

Server::~Server()
{
    closesocket(server_socket);
    CloseHandle(h_mutex);
    WSACleanup();
    printf("Server has been closed.\n");
}

void Server::start()
{
    const char *temp = server_addr;
    WSADATA wsaData;
    server_socket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    hints;
    /*
    * Initialize Winsock
    * MAKEWORD(2, 2): use version winsock2
    */
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
        throw "WSAStartup failed.\n";
    /* 
    * initial server's socket
    * socket(domain, type, protocol)
    * AF_INET       :   IPv4 Internet protocols
    * SOCKET_STREAM :   Provides sequenced, reliable, two-way, connection-based byte streams.
    * IPPROTO_TCP   :   TCP/IP protocols
    */
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(server_addr, port, &hints, &result);
    if (iResult != 0)
    {
        WSACleanup();
        throw "getaddrinfo failed.\n";
    }

    // Create a SOCKET for connecting to server
    server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (server_socket == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        WSACleanup();
        throw "socket failed.\n";
    }

    // bind with given Inet4address

    iResult = bind(server_socket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        freeaddrinfo(result);
        closesocket(server_socket);
        WSACleanup();
        throw "bind failed.\n";
    }

    freeaddrinfo(result);

    // start listen for client
    printf("Server has started. Waiting for client's connection...\n");
    iResult = listen(server_socket, MAX_CONNECTION);

    if (iResult == SOCKET_ERROR)
    {
        closesocket(server_socket);
        WSACleanup();
        throw "listen failed.\n";
    }

    while (true)
    {
        SOCKET client_socket;
        sockaddr_in client_addr;
        int caddr_size = sizeof(client_addr);
        // Accept a client socket
        client_socket = accept(server_socket, (sockaddr *)(&client_addr), &caddr_size);

        if (client_socket == INVALID_SOCKET)
        {
            closesocket(server_socket);
            WSACleanup();
            throw "accept failed.\n";
        }

        // print out client address information
        char *caddr = inet_ntoa(client_addr.sin_addr);
        printf("Connection from %s:%d\n", caddr, client_addr.sin_port);

        // insert client socket in socket message queue
        clients_queue.push_back(ClientSocket(client_socket, client_addr));
        // create a thread for client and listen for its message
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, connection, &client_socket, 0, NULL);
        if (hThread == NULL)
        {
            closesocket(server_socket);
            WSACleanup();
            throw "create thread failed.\n";
        }
        CloseHandle(hThread);
    }
}

unsigned __stdcall connection(LPVOID ipParameter)
{
    SOCKET socket = *((SOCKET *)ipParameter);
    char read_buffer[BUFFER_LENGTH];
    char write_buffer[BUFFER_LENGTH];
    int iResult;
    // optional
    {
        // send hello
        const char *greet = "hello! Connection established successfully.\n";
        send(socket, greet, strlen(greet), 0);
    }

    // keep on listening client's request
    while (true)
    {
        // init read/write buffer
        memset(read_buffer, 0, sizeof(read_buffer));
        memset(write_buffer, 0, sizeof(write_buffer));

        // receive package from client
        iResult = recv(socket, read_buffer, BUFFER_LENGTH, 0);
        WaitForSingleObject(h_mutex, INFINITE);

        // socket closed by client forcibly
        if (iResult <= 0)
        {
            sockaddr_in del_addr = removeClient(socket);
            printf("Client %s:%d disconnected.\n", inet_ntoa(del_addr.sin_addr), del_addr.sin_port);
            closesocket(socket);
            ReleaseMutex(h_mutex);
            break;
        }
        // deal&send package to client
        else
        {
            std::string s, ip_addr, port, message;
            SOCKET target = INVALID_SOCKET;
            sockaddr_in del_addr, src_addr;
            // get request type of package
            unsigned char type = read_buffer[0];
            // TEST FOR RECEIVE READ BUFFER
            // {
            //     for (int i = 0; i < strlen(read_buffer); i++) {
            //         printf("%c",read_buffer[i]);
            //     }
            // }
            switch (type)
            {
            case EXIT:
                del_addr = removeClient(socket);
                printf("Client %s:%d disconnected.\n", inet_ntoa(del_addr.sin_addr), del_addr.sin_port);
                closesocket(socket);
                ReleaseMutex(h_mutex);
                return 0;
            case GET_TIME:
                // [1 byte type] [time]
                write_buffer[0] = SEND_TIME;
                time_t t;
                struct tm *lt;
                time(&t);
                lt = localtime(&t);
                sprintf(write_buffer + strlen(write_buffer), "%d/%02d/%02d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
                send(socket, write_buffer, strlen(write_buffer) /*BUFFER_LEN*/, 0);
                break;
            case GET_SERVER_NAME:
                // [1 byte type] [server name]
                write_buffer[0] = SEND_SERVER_NAME;
                strcpy(write_buffer + strlen(write_buffer), server_name);
                send(socket, write_buffer, strlen(write_buffer), 0);
                break;
            case GET_CLIENT_LIST:
                // [1 byte type] [ip#port$]*
                write_buffer[0] = SEND_CLIENT_LIST;
                for (auto iter = clients_queue.begin(); iter != clients_queue.end(); ++iter)
                {
                    sprintf(write_buffer + strlen(write_buffer), "%s#%d$", inet_ntoa(iter->client_addr.sin_addr), iter->client_addr.sin_port);
                }
                //ip#port$ip#port$
                send(socket, write_buffer, strlen(write_buffer), 0);
                break;
            case GET_CLIENT_MESSAGE:
                // [1 byte type] [ip#port$] (type(io)) [message]
                // [message] -> @ip#port$content
                write_buffer[0] = SEND_CLIENT_MESSAGE;
                s = std::string(read_buffer + 1);
                ip_addr = s.substr(0, s.find("#"));
                port = s.substr(s.find("#") + 1, s.find("$") - s.find("#") - 1);
                message = s.substr(s.find("$") + 1);

                // find target socket from client list
                for (auto iter = clients_queue.begin(); iter != clients_queue.end(); ++iter)
                {
                    char *caddr = inet_ntoa(iter->client_addr.sin_addr);
                    if (!strcmp(caddr, ip_addr.c_str()) && iter->client_addr.sin_port == std::stoi(port))
                        target = iter->client_socket;
                    if (iter->client_socket == socket)
                        src_addr = iter->client_addr;
                }
                if (target == INVALID_SOCKET)
                {
                    sprintf(write_buffer + strlen(write_buffer), "fail to find target client.");
                    send(socket, write_buffer, strlen(write_buffer), 0);
                }
                else
                {
                    sprintf(write_buffer + strlen(write_buffer), "@%s#%d$%s", inet_ntoa(src_addr.sin_addr), src_addr.sin_port, message.c_str());
                    send(target, write_buffer, strlen(write_buffer), 0);
                }
                break;
            default:
                // [unrecognizable type]
                printf("failed to parses the request type of the packet.\n");
                break;
            }
            
            // print response package info into server.log
            FILE *fp = fopen("server.log", "a+");
            for (auto iter = clients_queue.begin(); iter != clients_queue.end(); ++iter)
            {
                if (iter->client_socket == socket)
                {
                    src_addr = iter->client_addr;
                    break;
                }
            }
            fprintf(fp, "%s:%d: ", inet_ntoa(src_addr.sin_addr), src_addr.sin_port);
            fprintf(fp, "%d ", write_buffer[0]);
            for (int i = 1; i < strlen(write_buffer); i++)
            {
                fputc(write_buffer[i], fp);
            }
            fputc('\n', fp);
            fclose(fp);

            ReleaseMutex(h_mutex);
        }
    }
    return 0;
}

sockaddr_in removeClient(SOCKET socket)
{
    sockaddr_in addr;
    for (auto iter = clients_queue.begin(); iter != clients_queue.end(); ++iter)
    {
        if (iter->client_socket == socket)
        {
            addr = iter->client_addr;
            iter = clients_queue.erase(iter);
            break;
        }
    }
    return addr;
}

int main(int argc, char **argv)
{
    try
    {
        // Read server's information from config.ini
        INIReader reader("config.ini");
        if (reader.ParseError() != 0)
            throw "Load config.ini failed.\n";
        server_name = reader.Get("server", "server_name", "UNKNOWN").c_str();
        server_addr = reader.Get("server", "ip", "UNKNOWN").c_str();
        port = reader.Get("server", "port", "UNKNOWN").c_str();
        MAX_CONNECTION = reader.GetInteger("server", "concurrency", -1);

        Server server;
        server.start();
    }
    catch (const char *message)
    {
        std::cerr << message << std::endl;
    }
}