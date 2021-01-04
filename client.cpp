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

int waiting_for_reply = 0;
char* cleanbuffer(char *buffer);
char* cutbuffer(char *buffer);
DWORD WINAPI connection(LPVOID ipParameter);

class Client{
public:
    SOCKET mysocket;
    char addr[20] = {0};
    int islink = 0;
    int port = -1;
    int current_service;//what kind of service the client needs
    string inputmess;//other input information
    Client();
    ~Client(){
        closesocket(mysocket);
        // WSACleanup(); // I don't know if client should clean the WSA, may be not.
        cout<<"Client has been closed."<<endl;
    }
    int start();
    int listen();
};

Client::Client()
{
    mysocket = INVALID_SOCKET;
}

int Client::start()
{
    while(waiting_for_reply == 1);
    if(islink == 0){
        printf("Please input your operation\n");
        printf("-------------------------------\n");
        printf("0 link\n(cannot be used now)1 get time\n(cannot be used now)2 get server time\n(cannot be used now)3 get client list\n(cannot be used now)4 get client message\n5 exit\n");
        printf("-------------------------------\n");
    }
    else if(islink == 1){
        printf("Please input your operation\n");
        printf("-------------------------------\n");
        printf("(cannot be used now)0 link\n1 get time\n2 get server time\n3 get client list\n4 get client message\n5 exit\n");
        printf("-------------------------------\n");
    }
    scanf("%d",&(this->current_service));
    printf("Waiting for the server to reply...\n");
    waiting_for_reply = 1;
    char* a = (char*)malloc(BUFFER_LENGTH*sizeof(char));
    if(current_service == 0&&islink ==0){
        //link 
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
            this->islink = 1;
            this->listen();
            return 1;
        }
        else if(inputmess==(string)"N"||inputmess == (string)"n"){
            return 0;
        }
    }
    else if(current_service == 1&&islink==1){
        //get time 
        a[0] = 1;
        send(mysocket,a,1,0);//strlen((char*)"1") == 1
        a = cleanbuffer(a);
    }
    else if(current_service == 2&&islink==1){
        //get server name
        a[0] = 2;
        send(mysocket,a,1,0);
        a = cleanbuffer(a);
    }
    else if(current_service == 3&&islink==1){
        //get client list
        a[0] = 3;
        send(mysocket,a,1,0);
        a = cleanbuffer(a);
    }
    else if(current_service == 4&&islink==1){
        //get client message
        a = cleanbuffer(a);
        a[0] = 4;
        printf("init a :  %s\n",a);
        char ip_[10] = {0};
        char port_[10] = {0};
        char message_[BUFFER_LENGTH] = {0};
        printf("Input the destination ip address : \n");
        scanf("%s",ip_);
        printf("Input the destination port : \n");
        scanf("%s",port_);
        scanf("%s",message_);
        
        strcat(a+1,ip_);
        strcat(a+1,"#");
        strcat(a+1,port_);
        strcat(a+1,"$");
        strcat(a+1,message_);
        a = cutbuffer(a);
        printf("now the package : %s",a);
        send(mysocket,a ,strlen(a),0);
        a = cleanbuffer(a);
    }//现在的问题，三的输出多了一个，四的输入第一个byte写不进去。
    else if(current_service == 5){
        printf("exit the client\n");
        a[0] = 0;
        if(islink==1)send(mysocket,a,1,0);
        a = cleanbuffer(a);
        islink = 0;
        return 0;
    }
    else {
        printf("Unknown service, please input again.\n");
    }
    return 1;
}

int Client::listen()
{
    if(islink == 1)
    {
        DWORD thread;
        // create a thread for server and listen for its message
        HANDLE hThread = CreateThread(NULL, 0, connection, &mysocket, 0, &thread);
        if (hThread == NULL)
        {
            closesocket(mysocket);
            WSACleanup();
            throw "create thread failed.\n";
        }
        CloseHandle(hThread);
    }
    else{
        printf("Not link\n");
    }
    return 0;
}


DWORD WINAPI connection(LPVOID ipParameter)
{
    SOCKET socket = *((SOCKET *)ipParameter);
    char* read_buffer = (char*)malloc(BUFFER_LENGTH*sizeof(char));
    read_buffer = cleanbuffer(read_buffer);
    int iResult;
    iResult = recv(socket, read_buffer, BUFFER_LENGTH, 0);
    if (iResult <= 0){
        printf("Connection has been closed\n");
        closesocket(socket);
    }
    else printf("FROM SERVER : %s\n",read_buffer);
    read_buffer = cleanbuffer(read_buffer);
    waiting_for_reply = 0;
    while(true){
        iResult = recv(socket, read_buffer, BUFFER_LENGTH, 0);
        if (iResult <= 0)
        {
            printf("Connection has been closed\n");
            closesocket(socket);
            break;
        }
        else {
            char type = read_buffer[0];
            if(type == 1){
                printf("FROM SERVER : %s\n",read_buffer+1);
            }
            else if(type == 2){
                printf("FROM SERVER : %s\n",read_buffer+1);
            }
            else if(type == 3){
                int n = 0;
                printf("FROM SERVER : %s\n",read_buffer);
                char* cut_read_buffer = cutbuffer(read_buffer);//shorter
                printf("No.%d, client address : ",n);
                n++;
                for(int i = 0; i < strlen(cut_read_buffer); i++){
                    if(cut_read_buffer[i] == '#'){
                        printf(", client port : ");
                    }
                    else if(cut_read_buffer[i] == '$'){
                        if(cut_read_buffer[i+1] != 0){
                            printf("\nNo.%d, client address : ",n);
                            n++;
                        }
                        else{
                            printf("\n");
                        }
                    }
                    else{
                        printf("%c",cut_read_buffer[i]);
                    }
                }
            }
            else if(type == 4){
                int message_start = 1;
                char* cut_read_buffer = cutbuffer(read_buffer);
                printf("FROM SERVER : ");
                for(int i = 0; i < strlen(cut_read_buffer); i++){
                    if(cut_read_buffer[i] == '#'){
                        printf(", PORT : ");
                    }
                    else if(cut_read_buffer[i] == '$'){
                        message_start = i+1;
                        break;
                    }
                    else{
                        printf("%c",cut_read_buffer[i]);
                    }
                }
                printf("MESSAGE : %s\n",cut_read_buffer+message_start);
            }
            read_buffer = cleanbuffer(read_buffer);
        }
        waiting_for_reply = 0;
    }
    
    return 0;
}

char* cleanbuffer(char *buffer){
    for(int i = 0; i < BUFFER_LENGTH; i++){
        buffer[i] = 0;
    }
    return buffer;
}

char* cutbuffer(char* buffer){
    int i = 0;
    for(i=BUFFER_LENGTH-1;i>=0;i--) {
        if (buffer[i] != 0) {
            break;
        }
    }
    // printf("cutbuffer : %d",i+1);
    char* b2 = (char*)malloc((i+1)*sizeof(char));
    for (int j = 0; j <i+1 ; j++) {
        b2[j] = buffer[j];
    }
    return b2;
}


int main(){
    Client thisclient;
    while(thisclient.start() > 0);
    return 0;
}
