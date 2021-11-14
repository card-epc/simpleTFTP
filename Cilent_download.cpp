#include "simpleTFTP.h"
#include <WinSock2.h>
#include <cstdio>
#include <string>
#include <Windows.h>

using namespace std;

int main()
{
    SOCKET socket1;  
    WSADATA wsaData;  
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) //初始化  
    {  
        printf("Winsock无法初始化!\n");  
        WSACleanup();  
        return 0;  
    }  
    printf("客户端开始创建SOCKET。\n");  
    struct sockaddr_in server;  
    int len = sizeof(server);

    server.sin_family = AF_INET;  
    server.sin_port = htons(69); // server的监听端口   
    server.sin_addr.s_addr = inet_addr("192.168.152.128"); //server的地址   
    
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);  
    char buffer[32];
    char recvData[1024];
    char ackData[5];

    memset(buffer, 0, sizeof(buffer));
    memset(recvData, 0, sizeof(recvData));
    memset(ackData, 0, sizeof(ackData));
    
    buffer[1] = RRQ;
    ackData[1] = ACK;
    strcpy(buffer+2, "test.txt");       // fileName
    strcpy(buffer+11, "netascii");       // transMode

    if (sendto(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&server, len) != SOCKET_ERROR)//发送信息给服务器，发送完进入等待，代表服务器在客户端启动前必须是等待状态  
    {  
        printf("Send a request, wait for the client to accept and send data...\n");  
    }
    struct sockaddr_in from;
    DWORD written;
    int num = 1;
    int from_len = sizeof(from);
    HANDLE hFile = CreateFileA("test.txt", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    // WriteFile(hFile, str.c_str(), str.size(), &written, NULL);
    // freopen("test.txt", "w", stdout);  
    while(true)  
    {    
        int datalen = recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
        if( datalen != SOCKET_ERROR && recvData[1] == 3 && recvData[3] == num)
        {   
            // printf("%s", recvData+4);
            if(!WriteFile(hFile, recvData+4, datalen-4, &written, NULL)) {
                printf("WRITE ERROR\n");
                break;
            }
            ackData[2] = recvData[2];
            ackData[3] = recvData[3];
            sendto(socket1, ackData, sizeof(ackData), 0, (struct sockaddr*)&from, from_len);
            num++;
            if(datalen < 516)   break;
        }  
        
    }  
    closesocket(socket1);  
    WSACleanup(); 
}