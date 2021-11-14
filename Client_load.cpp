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
    int slen = sizeof(server);

    server.sin_family = AF_INET;  
    server.sin_port = htons(69); // server的监听端口   
    server.sin_addr.s_addr = inet_addr("192.168.152.128"); //server的地址   
    
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);  
    DWORD read;
    char buf[PACKET];
    char sendData[1024];
    char recvData[1024];
    char ackData[5];

    memset(buf, 0, sizeof(buf));
    memset(sendData, 0, sizeof(sendData));
    memset(recvData, 0, sizeof(recvData));
    memset(ackData, 0, sizeof(ackData));
    
    buf[1] = WRQ;
    ackData[1] = ACK;
    sendData[1] = DATA;
    strcpy(buf+2, "1.docx");       // fileName
    strcpy(buf+9, BIN);       // transMode

    if (sendto(socket1, buf, sizeof(buf), 0, (struct sockaddr*)&server, slen) != SOCKET_ERROR)//发送信息给服务器，发送完进入等待，代表服务器在客户端启动前必须是等待状态  
    {  
        printf("Send a request, wait for the server to accept and send data...\n");  
    }
    struct sockaddr_in from;
    DWORD written;
    int num = 1;
    int from_len = sizeof(from);
    HANDLE hFile = CreateFileA("1.docx", GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    int fileSize = GetFileSize(hFile, nullptr);
    // WriteFile(hFile, str.c_str(), str.size(), &written, NULL);
    // freopen("test.txt", "w", stdout);  
    while(true) {
        int len = recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
        if(len == 4 && recvData[1] == 4 && recvData[3] == 0)
            break;
    }
    int len = fileSize;
    while ( len > 0 ) {
        int rlen = len > PACKET ? PACKET : len;
        ReadFile(hFile, sendData+4, rlen, &read, NULL);
        sendData[2] = (num >> 8) & 0xff;
        sendData[3] = num & 0xff;
        sendto(socket1, sendData, PACKET+4, 0, (struct sockaddr*)&from, from_len);
        while (true) {
            recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
            if(recvData[1] == ACK && recvData[3] == num) {
                break;
            }
        }
        memset(sendData+2, 0, sizeof(sendData)-2);
        num++;
        len-=PACKET;
    }  
    closesocket(socket1);  
    WSACleanup(); 
}