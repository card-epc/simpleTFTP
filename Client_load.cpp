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
size_t  tsize, rsize;
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

int wrapped_recvfrom(char* data, int rcsize, int sdsize, int opcode, int num) 
{
    char *oldData = new char[sdsize+3];
    memcpy(oldData, data, sdsize);
    while (1) {
        int len = recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len);
        // cout << "No Packet..."  << " " << recvfrom(socket1, data, rcsize, 0, (struct sockaddr*)&from, &from_len) << endl;
        if(len == SOCKET_ERROR) {
            sendto(socket1, oldData, sdsize, 0, (struct sockaddr*)&from, from_len);
            retranblk++;
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
                return len;
            } else {
                // cout << "OldData " << (int)oldData[0] << (int)oldData[1] << endl;
                // cout << "OldData " << (int)oldData[2] << (int)oldData[3] << endl;
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
    // printf("Client Create SOCKET.\n");
    // char sendData[1024];
    memset(Data, 0, sizeof(Data));
    
    Data[1] = DATA;
    int ssize = init_RQ(WRQ);
    printf("Start Upload.\n");       

    sendto(socket1, Data, ssize, 0, (struct sockaddr*)&server, slen);

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        logTime();
        fprintf(msgfp, "ReadFile failed.\n");
        archive();
    }
    tsize = GetFileSize(hFile, NULL);

    int x = wrapped_recvfrom(Data, sizeof(Data), ssize, ACK, 0);
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
        ReadFile(hFile, Data+4, rlen, &read, NULL);
        

        sendto(socket1, Data, rlen+4, 0, (struct sockaddr*)&from, from_len);
        rsize += rlen;
        // cout << "send " << num << endl;
        if (wrapped_recvfrom(Data, sizeof(Data), rlen+4, ACK, num) == 0) {
            return;
        }
        tend = clock();
        tt = (double)(tend-tmid)/CLOCKS_PER_SEC;
        if(tt > time_Interval) {
            tmid = tend;
            printRate(rsize, tsize, tt);
        }

        num++;
        len-=PACKET;
    }  
    tend = clock();
    printRate(rsize, tsize, tt);
    tt = (double)(tend-tstart)/CLOCKS_PER_SEC;
    cout << endl << "Throughput: " << convertBase(tsize/tt) << "bps" << endl;
    logTime();
    fprintf(msgfp, fileName.c_str());
    fprintf(msgfp, " upload succeed!\n");
    string retran = "Resend " + to_string(retranblk) + " blks. Total size: " + convertBase(tsize) + " bits\n";
    fprintf(msgfp, retran.c_str());
    archive();
    cout << "Finished.\n" << retran;
    retranblk = rsize = tsize = 0;
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

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
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
    tstart = clock();
    while(true)  
    {   
        // int datalen = recvfrom(socket1, Data, sizeof(Data), 0, (struct sockaddr*)&from, &from_len);
        int datalen = wrapped_recvfrom(Data, sizeof(Data), 4, DATA, num);
        // & -> (automatically convert to unsigned int)
        if(!datalen)    return;
        int datanum = (Data[3]&0xff) + ((Data[2]&0xff)<<8);
        rsize += (datalen-4);
        if(!WriteFile(hFile, Data+4, datalen-4, &written, NULL)) {
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
            tmid = tend;
            printRate(rsize, tsize, tt);
        }
        num++;
        if(datalen < 516)   break;   
    }  
    tend = clock();
    printRate(rsize, tsize, tt);
    tt = (double)(tend-tstart)/CLOCKS_PER_SEC;
    cout << endl << "Throughput: " << convertBase(tsize/tt) << "bps" << endl;
    logTime();
    fprintf(msgfp, fileName.c_str());
    fprintf(msgfp, " download succeed!\n");
    string retran = "Resend " + to_string(retranblk) + " blks. Total size: " + convertBase(tsize) + " bits\n";
    fprintf(msgfp, retran.c_str());
    archive();
    cout << "Finished.\n" << retran;
    retranblk = rsize = tsize = 0;
}

int main()
{ 
    msgfp = fopen("Msg.log", "a+");
    // fprintf(msgfp, "ASADSSAD");
    
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