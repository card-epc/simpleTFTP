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
int     TimeOut  = 3000;
int     maxTout  = 5;
string  mode     = BIN;
string  writeRQ;
int     tsize, rsize;
int     retranblk;
SOCKET  socket1;
char    errtm[30];
SYSTEMTIME systm;
FILE*   msgfp;
char    data[1024];

struct sockaddr_in server;
struct sockaddr_in from;  
int slen = sizeof(server);
int from_len = sizeof(from);
DWORD written;
DWORD read;
clock_t tstart, tend;
double  tt;  

void init_RQ(int RQsign)
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

int wrapped_recvfrom(char* data, int rcsize, int sdsize, int id, int sendid) 
{
    int n = 0;
    while (1) {
        int len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);
        if(len == SOCKET_ERROR) {
            sendto(socket1, data, sdsize, 0, (struct sockaddr*)&from, from_len);
            retranblk++;
            Sleep(300);
        }
        else if(id == data[1]) {
            // logTime();
            // fprintf(error, "Recv ")
            if(id == 6) {
                int idx = 0, a = 2;
                for( ; a; idx++) {
                    if(data[idx] == 0) a--;
                }
                tsize = atoi(data+idx);
            }
            return len;
        }
        else if(data[1] == 5) {
            logTime();
            fprintf(msgfp, "Error: ");
            fprintf(msgfp, data+4);
            fprintf(msgfp, "\n");
            return 0;
        }  
        // if(n == maxTout){
        //     logTime();
        //     string s = "Receive timeout and retransmit";
        //     s += n+'0';
        //     s += " times\n";
        //     fprintf(msgfp, s.c_str());
        //     return 0;
        // }
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
    char  recvData[1024];
    memset(recvData, 0, sizeof(recvData));
    
    init_Server();
    printf("Start Creating SOCKET.\n");       
    init_RQ(RRQ);

    // request for tsize
    memcpy(recvData, writeRQ.c_str(), writeRQ.size());
    if (sendto(socket1, writeRQ.c_str(), writeRQ.size(), 0, (struct sockaddr*)&server, slen) != SOCKET_ERROR) {  
        printf("Send a request, wait for the client to accept and send data...\n");
    }

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    int outTime = 0;
    // tstart = clock();
    while(1) {

        int ret = recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
        if(recvData[1] == 6) {
            int idx = 0, a = 2;
            for( ; a; idx++) {
                if(recvData[idx] == 0) a--;
            }
            tsize = atoi(recvData+idx);
            recvData[1] = ACK;
            recvData[2] = recvData[3] = 0;
            sendto(socket1, recvData, 4, 0, (struct sockaddr*)&from, from_len);
            break;
        }
        if(ret == SOCKET_ERROR) {
            sendto(socket1, writeRQ.c_str(), writeRQ.size(), 0, (struct sockaddr*)&server, slen);
            outTime++;
            Sleep(TimeOut);
        }
    }

    // initProgressBar();

    while(true)  
    {   
        int datalen = recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
        // & -> (automatically convert to unsigned int)
        int datanum = (recvData[3]&0xff) + ((recvData[2]&0xff)<<8);
        rsize += (datalen-4);
        // tend = clock(); tt = (double)(tend-tstart)/CLOCKS_PER_SEC;
        // ProgressBar(tt, rsize, tsize);
        if( datalen != SOCKET_ERROR && recvData[1] == 3 && datanum == num)
        {   
            if(!WriteFile(hFile, recvData+4, datalen-4, &written, NULL)) {
                printf("WRITE ERROR\n");
                break;
            }
            recvData[1] = ACK;
            sendto(socket1, recvData, 4, 0, (struct sockaddr*)&from, from_len);
            num++;
            if(datalen < 516)   break;
        }      
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