#include "simpleTFTP.h"
#include <WinSock2.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <time.h>
#include <Windows.h>

using namespace std;

string  fileName = "tftpd32.chm";
string  dstIP    = "192.168.152.128";
int     dstPort  = 69;
int     TimeOut  = 3000;
string  mode     = BIN;
string  writeRQ;
int     tsize, rsize;
SOCKET  socket1;
int     throughput;
clock_t tstart, tend;
double  tt;  
struct sockaddr_in server;
struct sockaddr_in from;  
int slen = sizeof(server);
int from_len = sizeof(from);

void init_RRQ()
{
    writeRQ.clear();
    writeRQ += ZEROCHAR;
    writeRQ += (char)RRQ;
    writeRQ += fileName;
    writeRQ += ZEROCHAR;
    writeRQ += mode;
    writeRQ += ZEROCHAR;
    writeRQ += "tsize";
    writeRQ += ZEROCHAR;
    writeRQ += '0';
    writeRQ += ZEROCHAR;
}


void DownLoad()
{

    struct sockaddr_in server; 
    struct sockaddr_in from; 
    int len = sizeof(server);
    int from_len = sizeof(from);
    
    
    DWORD written;
    int   num = 1;
    char  recvData[1024];
    memset(recvData, 0, sizeof(recvData));
    
    server.sin_family = AF_INET;  
    server.sin_port = htons(dstPort); // server listen port   
    server.sin_addr.s_addr = inet_addr(dstIP.c_str()); //server ip
    

    
    printf("Start Creating SOCKET.\n");     
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);  
    init_RRQ();

    // request for tsize
    if (sendto(socket1, writeRQ.c_str(), writeRQ.size(), 0, (struct sockaddr*)&server, len) != SOCKET_ERROR) {  
        printf("Send a request, wait for the client to accept and send data...\n");
    }

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    // WriteFile(hFile, str.c_str(), str.size(), &written, NULL);
    // freopen("test.txt", "w", stdout);
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
            sendto(socket1, writeRQ.c_str(), writeRQ.size(), 0, (struct sockaddr*)&server, len);
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
    WSADATA wsaData;
    // HideCursor();
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {  
        printf("Winsock init failed!\n");  
        WSACleanup();  
        return 0;  
    }
    DownLoad();
    char recvData[1024];
    int tt = 1000;
    // setsockopt(socket1, SOL_SOCKET, SO_RCVTIMEO, (char*)&tt, sizeof(int));
    while(1) {
        cout << "start" << endl;
        int datalen = recvfrom(socket1, recvData, sizeof(recvData), 0, (struct sockaddr*)&from, &from_len);
        cout << "test " << datalen << endl;
    }
    closesocket(socket1);  
    WSACleanup(); 
}