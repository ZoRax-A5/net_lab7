// g++ client.cpp -o server -lwsock32 -lws2_32
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

using namespace std;
// for the client, the server receive the message from the client,and create a thread, i should first let it create a thread.

//whether client is open or closed

int sendMessage();

class Client{
public:
    SOCKET mysocket;
    char addr[20] = {0};
    int port = -1;
    int current_service;
    string message;
    string inputmess;
    Client();
    ~Client(){
        closesocket(mysocket);
        // WSACleanup(); // I don't know if client should clean the WSA, may be not.
        cout<<"Client has been closed."<<endl;
    }
    int start();
};

Client::Client()
{
    mysocket = INVALID_SOCKET;
}

int Client::start()
{
    cout<<"Please input your operation"<<endl;
    cout<<"-------------------------------"<<endl;
    cout<<"0 exit"<<endl<<"1 get time"<<endl<<"2 get server time"
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA wsaData;
    if(WSAStartup(sockVersion, &wsaData) != 0){
        cout<<"WSAStartup fail!"<<endl;
        return 0;
    }
    cout<<"Please input the address of the server that you want to connect"<<endl;
    cin>>addr>>port;
    cout<<"Your input address : "<<addr<<endl;
    cout<<"Your input port : "<<port<<endl;
    cout<<"Whether log in the server? [Y/N]"<<endl;
    cin>>inputmess;
    if(inputmess==(string)"Y"||inputmess == (string)"y"){
        mysocket = socket(PF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serAddr;
        serAddr.sin_family = AF_INET;
        serAddr.sin_port = htons(port);
        serAddr.sin_addr.S_un.S_addr = inet_addr(addr);
        memset(&(serAddr.sin_zero), '\0', 8);
        
        if(connect(mysocket, (struct sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR){
            cout<<"connect error!"<<endl;
            shutdown(mysocket,SD_BOTH);
            return -1;
        }
        cout<<"connect successfully!"<<endl;
        return 1;
    }
    else if(inputmess==(string)"N"||inputmess == (string)"n"){
        return 0;
    }
    return 0;
}

int main(){
    Client thisclient;
    if(thisclient.start()){
        cout<<"linked...127.0.0.1"
    }
    while(1){
        if(sendMessage() == 0)break;

    }
    return 0;
}

int sendMessage(){

    return 1;
}
/*
client to server a bit
一个单位数据包，一个buffer，第一位八个字节足够。
默认 : char是一个byte
发给客户端的信息，第一位是代表回应请求（请求的类型），位指的都是byte，
后面是char* write buffer + 1，后面全0，
可能长度甚至少于0.
基本都是编号+信息。
第一byte都是编号。
信息里面不带换行符。
#$是特殊符号，要所有用户的list，#前面是用户ip，#后面是用户端口，然后反复。
ip#port$...
最后是定向内容，
我应该怎么发给服务器，一个类型，ip#port$消息。

*/