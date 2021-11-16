#include "simpleTFTP.h"
#include <WinSock2.h>
#include <cstdio>
#include <string>
#include <time.h>
#include <iostream>
#include <Windows.h>

using namespace std;

string  fileName = "FLEX.pdf";
string  dstIP    = "192.168.152.128";
int     dstPort  = 69;
int     TimeOut  = 100;
int     maxTout  = 5;
string  mode     = BIN;
string  writeRQ;
int     tsize, rsize;
int     retranblk;
SOCKET  socket1;
char    errtm[30];
SYSTEMTIME systm;
FILE*   msgfp;
char    Data[1024];

struct sockaddr_in server;
struct sockaddr_in from;  
int slen = sizeof(server);
int from_len = sizeof(from);
DWORD written;
DWORD read;
clock_t tstart, tend;
double  tt;  

int init_RQ(int RQsign)
{
    writeRQ.clear();
    writeRQ += ZEROCHAR;
    writeRQ += (char)RQsign;
    writeRQ += fileName;
    writeRQ += ZEROCHAR;
    writeRQ += mode;
    writeRQ += ZEROCHAR;
    if ( RQsign == RRQ ) {
        writeRQ += "tsize";
        writeRQ += ZEROCHAR;
        writeRQ += '0';
        writeRQ += ZEROCHAR;
    }
    memcpy(Data, writeRQ.c_str(), writeRQ.size());
    return writeRQ.size();
}

inline void init_Server() {
    server.sin_family = AF_INET;  
    server.sin_port = htons(dstPort);    
    server.sin_addr.s_addr = inet_addr(dstIP.c_str());
}

inline void logTime() {
    GetLocalTime(&systm);
    sprintf(errtm, "[%4d/%02d/%02d %02d:%02d:%02d] ", systm.wYear, systm.wMonth, systm.wDay, systm.wHour, systm.wMinute, systm.wSecond);
    fprintf(msgfp, errtm);
}

int wrapped_recvfrom(char* data, int rcsize, int sdsize, int opcode, int num) 
{
    while (1) {
        int len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);
        if(len == SOCKET_ERROR) {
            sendto(socket1, data, sdsize, 0, (struct sockaddr*)&from, from_len);
            retranblk++;
            Sleep(TimeOut);
        }
        else if(data[1] == TERROR) {
            logTime();
            fprintf(msgfp, "Error: ");
            fprintf(msgfp, data+4);
            fprintf(msgfp, "\n");
            return 0;
        }
        else if(opcode == data[1]) {
            int datanum = (Data[3]&0xff) + ((Data[2]&0xff)<<8);
            if(num == datanum) {
                return len;
            } else {
                retranblk++;
                sendto(socket1, data, sdsize, 0, (struct sockaddr*)&from, from_len);
            }
        }
    }
}

void UpLoad()
{

    init_Server();
    int num = 1;
    printf("Client Create SOCKET。\n");
    char sendData[1024];
    memset(sendData, 0, sizeof(sendData));
    
    sendData[1] = DATA;
    init_RQ(WRQ);

    if (sendto(socket1, writeRQ.c_str(), writeRQ.size(), 0, (struct sockaddr*)&server, slen) != SOCKET_ERROR) {  
        printf("Send a request, wait for the server to accept and send data...\n");  
    }

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    int fileSize = GetFileSize(hFile, nullptr);
    while (true) {
        int len = recvfrom(socket1, sendData, sizeof(sendData), 0, (struct sockaddr*)&from, &from_len);
        if(sendData[1] == ACK && len == 4)
            break;
    }
    int len = fileSize;
    while ( len >= 0 ) {
        int rlen = len > PACKET ? PACKET : len;

        memset(sendData, 0, sizeof(sendData));
        sendData[1] = DATA;
        sendData[2] = (num >> 8) & 0xff;
        sendData[3] = num & 0xff;
        ReadFile(hFile, sendData+4, rlen, &read, NULL);
        

        sendto(socket1, sendData, rlen+4, 0, (struct sockaddr*)&from, from_len);
        while (true) {
            recvfrom(socket1, sendData, sizeof(sendData), 0, (struct sockaddr*)&from, &from_len);
            int acknum = (sendData[3]&0xff) + ((sendData[2]&0xff)<<8);
            if(sendData[1] == ACK && acknum == num) {
                break;
            }
        }
        num++;
        len-=PACKET;
    }  
}

void DownLoad()
{      
    int   num = 1;
    memset(Data, 0, sizeof(Data));
    
    init_Server();
    printf("Start Creating SOCKET.\n");       
    int ssize = init_RQ(RRQ);

    // request for tsize
    if (sendto(socket1, Data, ssize, 0, (struct sockaddr*)&server, slen) != SOCKET_ERROR) {  
        printf("Send a request, wait for the client to accept and send data...\n");
    }

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    // tstart = clock();
    while(1) {

        int ret = recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
        if(Data[1] == 6) {
            int idx = 0, a = 2;
            for( ; a; idx++) {
                if(Data[idx] == 0) a--;
            }
            tsize = atoi(Data+idx);
            Data[1] = ACK;
            Data[2] = Data[3] = 0;
            sendto(socket1, Data, 4, 0, (struct sockaddr*)&from, from_len);
            break;
        }
        if(ret == SOCKET_ERROR) {
            sendto(socket1, Data, ssize, 0, (struct sockaddr*)&server, slen);
            Sleep(TimeOut);
        }
    }

    // initProgressBar();

    while(true)  
    {   
        // int datalen = recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
        int datalen = wrapped_recvfrom(Data, sizeof(Data), 4, DATA, num);
        // & -> (automatically convert to unsigned int)
        int datanum = (Data[3]&0xff) + ((Data[2]&0xff)<<8);
        rsize += (datalen-4);
        if(!WriteFile(hFile, Data+4, datalen-4, &written, NULL)) {
                printf("WRITE ERROR\n");
                break;
            }
        Data[1] = ACK;
        sendto(socket1, Data, 4, 0, (struct sockaddr*)&from, from_len);
        num++;
        if(datalen < 516)   break;   
    }  
    putchar('\n');
}

int main()
{ 
    msgfp = fopen("Msg.log", "a+");
    WSADATA wsaData;  
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {  
        printf("Winsock无法初始化!\n"); 
        WSACleanup();  
        return 0;  
    }
    socket1 = socket(AF_INET, SOCK_DGRAM, 0); 
    // UpLoad();
    DownLoad();
    fclose(msgfp);
    closesocket(socket1);  
    WSACleanup(); 
}