#include "simpleTFTP.h"
#include <WinSock2.h>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <Windows.h>

using namespace std;

const double time_Interval = 0.1;

string  writeRQ;
size_t  tsize, rsize, prsize;
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
clock_t tstart, tend, tmid;
double  tt; 

inline void archive() {
    fclose(msgfp);
    msgfp = fopen("Msg.log", "a+");
} 

inline string convertBase(int size) {
    double ss;
    char num[30];
    if (size > 1000000) {
        sprintf(num, "%.2lf", round(size/10000.0)/100.0);
        return string(num)+"M";
    } else if (size > 1000) {
        sprintf(num, "%.2lf", round(size/10.0)/100.0);
        return string(num)+"K";
    } else {
        return to_string(size);
    }
}

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

void recv_flush()
{
    char data[1024];
    struct sockaddr_in temp;
    while(1) {
        int x = recvfrom(socket1, data, sizeof(data), 0, (struct sockaddr*)&temp, &from_len);
        if(x == -1)
            break;
    }
}

int wrapped_recvfrom(char* data, int rcsize, int sdsize, int opcode, int num, bool ff) 
{
    char *oldData = new char[sdsize+3];
    // struct sockaddr_in temp = (ff == 1) ? server : from;
    memcpy(oldData, data, sdsize);
    if(ff == 1)
    {
        int len;
        for(int i = 1; i<10; i++) {
            len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);
            if(len != -1)
                break;
        }
        if(len != -1)
            return len;
        else {
            fprintf(msgfp, "Request Rrror\n");
            archive();
            return 0;
        }
    }
    clock_t x, y;
    x = clock();
    while (1) {
        int len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);
        // cout << "No Packet..."  << " " << recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len) << endl;
        if(len == SOCKET_ERROR) {
            sendto(socket1, oldData, sdsize, 0, (struct sockaddr*)&from, from_len);
            retranblk++;
            // cout << "Sock_ERROR" << endl;
            y = clock();
            Sleep(500);
            if ( (double)(y-x)/CLOCKS_PER_SEC > 20.0) {
                logTime();
                fprintf(msgfp, "Maybe unreachable port\n");
                printf("\nthere is something wrong.\n");
                archive();
                return 0;
            }
            continue;
            // Sleep(TimeOut);
        } else if(data[1] == TERROR) {
            logTime();
            fprintf(msgfp, "Error: ");
            fprintf(msgfp, data+4);
            fprintf(msgfp, "\n");
            archive();
            return 0;
        } else if (opcode == data[1]) {

            int datanum = (data[3]&0xff) + ((data[2]&0xff)<<8);
            // cout << "ACK Blk " << datanum << endl;
            if(num == datanum) {
                // cout << "Get " << num << endl;
                // int len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);

                return len;
            } else {
                // cout << "OldData " << (int)oldData[0] << (int)oldData[1] << endl;
                // cout << "OldData " << (int)oldData[2] << (int)oldData[3] << endl;
                // cout << num << " " << datanum << endl;
                retranblk++;
                sendto(socket1, oldData, sdsize, 0, (struct sockaddr*)&from, from_len);
                continue;
            }
        }
        sendto(socket1, oldData, sdsize, 0, (struct sockaddr*)&from, from_len);
        retranblk++;
        Sleep(TimeOut);
    }
    free(oldData);
}

void UpLoad()
{
    init_Server();
    int num = 1;

    memset(Data, 0, sizeof(Data));
    
    Data[1] = DATA;
    int ssize = init_RQ(WRQ);
    printf("Start Upload.\n");       

    sendto(socket1, Data, ssize, 0, (struct sockaddr*)&server, slen);

    // HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    FILE* fp = fopen(fileName.c_str(), "rb+");
    // if(hFile == INVALID_HANDLE_VALUE) {
    if(fp == nullptr) {
        logTime();
        fprintf(msgfp, "ReadFile failed.\n");
        archive();
        return;
    }
    
    // tsize = GetFileSize(hFile, NULL);
    
    fseek(fp, 0, SEEK_END);
    tsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // cout << tsize << endl;
    // system("pause");
    int x = wrapped_recvfrom(Data, sizeof(Data), ssize, ACK, 0, 1);
    // int len = wrapped_recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
    if (x == 0) {
        return;
    }
    // ProgressBar pb(tsize);
    tstart = clock();
    int len = tsize;
    while ( len >= 0 ) {

        int rlen = len > PACKET ? PACKET : len;
        memset(Data, 0, sizeof(Data));
        Data[1] = DATA;
        Data[2] = (num >> 8) & 0xff;
        Data[3] = num & 0xff;
        // ReadFile(hFile, Data+4, rlen, &read, NULL);
        fread(Data+4, 1, rlen, fp);
        

        sendto(socket1, Data, rlen+4, 0, (struct sockaddr*)&from, from_len);
        rsize += rlen;
        // cout << "send " << num << endl;
        if (wrapped_recvfrom(Data, sizeof(Data), rlen+4, ACK, num, 0) == 0) {
            return;
        }
        tend = clock();
        tt = (double)(tend-tmid)/CLOCKS_PER_SEC;
        if(tt > time_Interval) {
            printRate(rsize, tsize, tt, (rsize-prsize)/tt);
            tmid = tend;
            prsize = rsize;
        }
        num = (num+1)%65536;
        len-=PACKET;
    }  
    tend = clock();
    printRate(tsize, tsize, tt, 0);
    tt = (double)(tend-tstart)/CLOCKS_PER_SEC;
    cout << endl << "Throughput: " << convertBase(tsize*8/tt) << "bps" << endl;
    logTime();
    fprintf(msgfp, fileName.c_str());
    fprintf(msgfp, " upload succeed!\n");
    string retran = "Resend " + to_string(retranblk) + " blks. Total size: " + byteConvert(tsize) + "\n";
    fprintf(msgfp, retran.c_str());
    archive();
    cout << "Finished.\n" << retran;
    fclose(fp);
    retranblk = rsize = tsize = prsize = 0;
    recv_flush();
}

void DownLoad()
{      
    int   num = 1;
    memset(Data, 0, sizeof(Data));
    
    init_Server();
    printf("Start Download.\n");       
    int ssize = init_RQ(RRQ);

    // request for tsize
    sendto(socket1, Data, ssize, 0, (struct sockaddr*)&server, slen);

    // HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    FILE* fp = fopen(fileName.c_str(), "wb+");

    // int ret = recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
    int ret = wrapped_recvfrom(Data, sizeof(Data), ssize, 6, 0x7473, 1);
    if(!ret)    return;
    int idx = 0, a = 2;
    for( ; a; idx++) {
        if(Data[idx] == 0) a--;
    }
    tsize = atoi(Data+idx);
    Data[1] = ACK;
    Data[2] = Data[3] = 0;
    sendto(socket1, Data, 4, 0, (struct sockaddr*)&from, from_len);

    // initProgressBar();
    tstart = clock();
    while(true)  
    {   
        // int datalen = recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
        int datalen = wrapped_recvfrom(Data, sizeof(Data), 4, DATA, num, 0);
        // & -> (automatically convert to unsigned int)
        if(!datalen)    return;
        int datanum = (Data[3]&0xff) + ((Data[2]&0xff)<<8);
        rsize += (datalen-4);
        // cout << "rsize: " << rsize << endl;
        // if(!WriteFile(hFile, Data+4, datalen-4, &written, NULL)) {
        if( fwrite(Data+4, 1, datalen-4, fp) != (datalen-4) ) {
                logTime();
                fprintf(msgfp, "WriteFile Error.\n");
                archive();
                break;
        }
        Data[1] = ACK;
        sendto(socket1, Data, 4, 0, (struct sockaddr*)&from, from_len);
        tend = clock();
        tt = (double)(tend-tmid)/CLOCKS_PER_SEC;
        if(tt > time_Interval) {
            printRate(rsize, tsize, tt, (rsize-prsize)/tt);
            prsize = rsize;
            tmid = tend;
        }
        num = (num+1)%65536;
        if(datalen < 516)   break;   
    }  
    tend = clock();
    printRate(tsize, tsize, tt, 0);
    tt = (double)(tend-tstart)/CLOCKS_PER_SEC;
    cout << endl << "Throughput: " << convertBase(tsize*8/tt) << "bps" << endl;
    logTime();
    fprintf(msgfp, fileName.c_str());
    fprintf(msgfp, " download succeed!\n");
    string retran = "Resend " + to_string(retranblk) + " blks. Total size: " + byteConvert(tsize) + "\n";
    fprintf(msgfp, retran.c_str());
    archive();
    cout << "Finished.\n" << retran;
    fclose(fp);
    recv_flush();
    retranblk = rsize = tsize = prsize = 0;
}

int main()
{ 
    msgfp = fopen("Msg.log", "a+");
    
    WSADATA wsaData;  
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {  
        logTime();
        fprintf(msgfp, "init failed\n");
        archive();
        WSACleanup();  
        return 0;  
    }
    socket1 = socket(AF_INET, SOCK_DGRAM, 0); 
    setsockopt(socket1, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(int));
    // UpLoad();
    // DownLoad();
    // cout << retranblk << endl;
    InterfaceConf ifc;
    string comline;
    do {
        ifc.print();
        RestoreCursor();
        getline(cin, comline);
        HideCursor();
        // transform(comline.begin(), comline.end(), comline.begin(), ::tolower);
    }while(ifc.run(comline));

    fclose(msgfp);
    closesocket(socket1);  
    WSACleanup(); 
}