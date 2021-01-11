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
void cleanbuffer(char buffer[]);
char* cutbuffer(char *buffer);
DWORD WINAPI connection(LPVOID ipParameter);

class Client{
public:
//some information about the client
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
    //wait for the receive thread
    while(waiting_for_reply == 1);
    //the user-interactive
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
    char a[BUFFER_LENGTH] = {0};
    //link
    if(current_service == 0&&islink ==0){
        //link 
        WORD sockVersion = MAKEWORD(2,2);
        WSADATA wsaData;
        if(WSAStartup(sockVersion, &wsaData) != 0){
            cout<<"WSAStartup fail!"<<endl;
            return 0;
        }
        //input the ip and the port
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
            //try to connect
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
    //get the time
    else if(current_service == 1&&islink==1){
        //get time 
        a[0] = GET_TIME;
        // for(int i=0;i<100;i++){
            send(mysocket,a,1,0);
            // sleep(100);
        // }//this is for the test, get 100 mysocket.       
        cleanbuffer(a);
    }
    //get the name of the server
    else if(current_service == 2&&islink==1){
        //get server name
        a[0] = GET_SERVER_NAME;
        send(mysocket,a,1,0);
        cleanbuffer(a);
    }
    //send the third type of the message : get the list of the clients
    else if(current_service == 3&&islink==1){
        //get client list
        a[0] = GET_CLIENT_LIST;
        send(mysocket,a,1,0);
        cleanbuffer(a);
    }
    //send the forth type of message : send message to another client
    else if(current_service == 4&&islink==1){
        //get client message
        cleanbuffer(a);]
        a[0] = GET_CLIENT_MESSAGE;
        char send_addr[10] = {0};
        char send_port[10] = {0};
        char send_message[BUFFER_LENGTH] = {0};
        printf("Please input the ip : \n");
        scanf("%s",send_addr);
        printf("Please input the port : \n");
        scanf("%s",send_port);
        printf("Please input the message : \n");
        // scanf("%s",send_message);
        getchar();//get the '\n'
        cin.getline(send_message,BUFFER_LENGTH);

        strcat(a, send_addr);
        strcat(a, "#");
        strcat(a, send_port);
        strcat(a, "$");
        strcat(a, send_message);
        strcat(a, "\n\0");
        send(mysocket,a,strlen(a),0);
        cleanbuffer(a);
        waiting_for_reply = 0;
    }
    //exit
    else if(current_service == 5){
        printf("exit the client\n");
        a[0] = 0;
        if(islink==1)send(mysocket,a,1,0);
        cleanbuffer(a);
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
    cleanbuffer(read_buffer);
    int iResult;
    //when you first connect the server, receive a message from the server.
    iResult = recv(socket, read_buffer, BUFFER_LENGTH, 0);
    if (iResult <= 0){
        printf("Connection has been closed\n");
        closesocket(socket);
    }
    else printf("FROM SERVER : %s\n",read_buffer);
    cleanbuffer(read_buffer);
    waiting_for_reply = 0;
    while(true){
        //listen to the server continuously.
        iResult = recv(socket, read_buffer, BUFFER_LENGTH, 0);
        //whether the connection is closed.
        if (iResult <= 0)
        {
            printf("Connection has been closed\n");
            closesocket(socket);
            break;
        }
        else {
            //check the type.
            char type = read_buffer[0];
            //receive the time
            if(type == 1){
                printf("FROM SERVER : %s\n",read_buffer+1);
            }
            //receive the name of the server
            else if(type == 2){
                printf("FROM SERVER : %s\n",read_buffer+1);
            }
            //receive the list of the clients
            else if(type == 3){
                int n = 0;
                printf("FROM SERVER : %s\n",read_buffer);
                char* cut_read_buffer = cutbuffer(read_buffer);//shorter
                printf("No.%d, client address : ",n);
                n++;
                //parse the packet
                for(int i = 0; i < strlen(cut_read_buffer); i++){
                    if(cut_read_buffer[i] == '#'){
                        printf(", client port : ");
                    }
                    else if(cut_read_buffer[i] == '$'){
                        if((cut_read_buffer[i+1] >= '0'&&cut_read_buffer[i+1] <='9')||(cut_read_buffer[i+1] == '.')){
                            printf("\nNo.%d, client address : ",n);
                            n++;
                        }
                        else{
                            printf("\n");
                            break;
                        }
                    }
                    else{
                        printf("%c",cut_read_buffer[i]);
                    }
                }
            }
            //receive a message from another client
            else if(type == 4){
                int message_start = -1;
                char* cut_read_buffer = cutbuffer(read_buffer);
                
                for(int i = 0; i < strlen(cut_read_buffer); i++){
                    if(cut_read_buffer[i] == '@'){
                        printf("FROM SERVER : ");
                    }
                    else if(cut_read_buffer[i] == '#'){
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
                if(message_start != -1)printf("MESSAGE : %s\n",cut_read_buffer+message_start);
            }
            cleanbuffer(read_buffer);
        }
        waiting_for_reply = 0;
    }
    
    return 0;
}

//this function is used to clean the buffer to all zero.
void cleanbuffer(char buffer[]){
    for(int i = 0; i < BUFFER_LENGTH; i++){
        buffer[i] = 0;
    }
}

//this function is used to cut the buffer to a shorter one.
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
    Client thisclient;//let the client begin
    while(thisclient.start() > 0);
    return 0;
}
