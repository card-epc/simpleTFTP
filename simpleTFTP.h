#ifndef SIMAPLETFTP
#define SIMAPLETFTP
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <map>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <string>

#define RRQ         1
#define WRQ         2
#define DATA        3
#define ACK         4
#define TERROR      5
#define ASCII       "netascii"
#define BIN         "octet"
#define PACKET      512
#define Square      "¨€"
#define ZEROCHAR    char(0)

using namespace std;


string  fileName = "test.png";
string  dstIP    = "127.0.0.1";
int     dstPort  = 69;
int     TimeOut  = 500;
int     maxTout  = 5;
string  mode     = BIN;

extern void UpLoad();
extern void DownLoad();

inline string byteConvert(size_t size) {
    char ss[30];
    double num;
    if((size>>20) > 0) {
        num = size/1024.0/1024;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" MB";
    } else if((size>>10) > 0) {
        num = size/1024.0;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" KB";
    } else {
        num = size/8.0;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" Bytes";
    }
    //  else {
    //     return to_string(size)+" bits";
    // }
}

int printRate(size_t rsize, size_t tsize, double tt, double speed) {
    int r = 50*rsize/tsize;
    putchar('\r');
    for(int i=0; i<r; i++)
        printf(Square);
    for(int i=r+1; i<=50; i++)
        putchar(' ');
    putchar('|');
    fflush(stdout);
    // cout << byteConvert(rsize) << "/" << byteConvert(tsize) << "            ";
    printf("%s/%s  %s/s              ",
            byteConvert(rsize).c_str(), byteConvert(tsize).c_str(), byteConvert(round(speed)).c_str());
    // printf("            ");
    // printf("%d bits/%d bits", rsize, tsize);
}

void HideCursor()
{
	CONSOLE_CURSOR_INFO cursor;    
	cursor.bVisible = FALSE;    
	cursor.dwSize = sizeof(cursor);    
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);    
	SetConsoleCursorInfo(handle, &cursor);
}

void RestoreCursor()
{
    CONSOLE_CURSOR_INFO cursor;    
	cursor.bVisible = TRUE;    
	cursor.dwSize = sizeof(cursor);    
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);    
	SetConsoleCursorInfo(handle, &cursor);   
}


class Solution {
public:
    string validIPAddress(string queryIP) {
        split(queryIP, '.');
        if(inspectIpv4())
            return "IPv4";
        else{
            part.clear();
            split(queryIP, ':');
            if(InspectIPv6())
                return "IPv6";
        }
        return "Neither";
    }
private:
    vector<string> part;
    void split(const string& str, char c) {
        string temp;
        int pre = 0;
        for(int i = 0; i < str.size(); i++) {
            if(str[i] == c) {
                temp.assign(str.begin()+pre, str.begin() + i);
                pre = i + 1;
                part.push_back(temp);
            }
        }
        temp.assign(str.begin()+pre, str.end());
        part.push_back(temp);
    }
    bool inspectIpv4() {
        if(part.size() != 4) 
            return false;
        else {
            for(const string& str : part)
            {
                if(str.empty() || str.size() > 3)
                    return false;
                for(char c : str) {
                    if(!isdigit(c))
                        return false;
                }
                if(str[0] == '0' && str.size() > 1)
                    return false;
                int num = stoi(str);
                if(num > 255)
                    return false;
            }
            return true;
        }
    }
    bool InspectIPv6() {
        if(part.size() != 8)
            return false;
        else {
            for(string& str : part)
            {
                if(str.empty() || str.size() > 4)
                    return false;
                transform(str.begin(), str.end(), str.begin(), ::tolower);
                for(char c : str) {
                    if(!isdigit(c) && (c > 'f' || c < 'a'))
                        return false;
                }
            }
            return true;
        }
    }
};


class InterfaceConf {
    private:
        string promt;
        string param;
        vector<string> cmlpart;
        typedef void (InterfaceConf::*funcptr)(const string&);
        typedef void (InterfaceConf::*printptr)();
        // void (InterfaceConf::*funcptr[4])(const string&);
        funcptr setbuf[4];
        map<string, int> options;
        map<string, int> setparams;
        map<string, int>::iterator iter;
        // void printhelp();
        void printoption();
        // void printlog();
        void printFile(const string& str);
        void update() {
            promt = "to " + dstIP + ":" + to_string(dstPort) + " > ";
        }
        void split_space(const string& str) {
            cmlpart.clear();
            string temp;
            for(int i = 0; i<str.size(); ) {
                if(str[i] == ' ') {
                    while(str[i] == ' ') i++;
                }
                else {
                    int x = i;
                    while(str[i] != ' ' && i<str.size()) i++;
                    temp.assign(str.begin()+x, str.begin()+i);
                    cmlpart.push_back(temp);
                }
            }
        }
    public:
        InterfaceConf() {
            options.emplace("q",        0);
            options.emplace("quit",     0);
            options.emplace("exit",     0);
            options.emplace("h",        1);
            options.emplace("help",     1);
            options.emplace("log",      1);
            options.emplace("options",  1);
            options.emplace("upload",   2);
            options.emplace("download", 3);
            options.emplace("set",      4);
            options.emplace("cls",      5);
            options.emplace("clear",    5);

            setparams.emplace("mode",     0);
            setparams.emplace("dhost",    1);
            setparams.emplace("dport",    2);
            setparams.emplace("timeout",  3);

            update();
            setbuf[1] = setdip;
            setbuf[2] = setdport;
            setbuf[3] = setTimeout;
            setbuf[0] = setTransmode;
        }
        void print() { cout << promt; }
        int run(const string& str);
        void setTimeout(const string& str) {
            for(char c : str) {
                if(!isdigit(c)) {
                    cout << "Syntax Error" << endl;
                    return;
                }
            }
            TimeOut = stoi(str);
        }
        void setdip(const string& str) {
            Solution s;
            string ans = s.validIPAddress(str);
            if(ans == "IPv4")
                dstIP = str;
            else if(ans == "IPv6")
                cout << "IPv6 is not supported." << endl;
            else
                cout << "Wrong IPaddress" << endl;
            update();
        }        
        void setdport(const string& str) {
            for(char c : str) {
                if(!isdigit(c)) {
                    cout << "Syntax Error" << endl;
                    return;
                }
            }
            if(str.size() > 5){
                cout << "Port is too large" << endl;
                return;
            }
            int num = stoi(str);
            if(num > 65535) {
                cout << "Port is out of range(0~65535)." << endl;
                return;
            }
            dstPort = num;
            update();
        }        
        void setTransmode(const string& str) {
            if(str == "bin" || str == "ascii")
                mode = (str == "bin") ? BIN : ASCII;
            else
                cout << "Mode can't found" << endl;
            return;
        }    

};

void InterfaceConf::printFile(const string& str)
{
    string name = (str == "log") ? "Msg.log" : "help";
    FILE *fp;
    int  len;
    char *data;
    fp = fopen(name.c_str(), "rb");
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (char*)malloc(len + 1);
    data[len] = 0;
    fread(data, 1, len, fp);
    fclose(fp);
    printf("%s\n", data);

}

void InterfaceConf::printoption()
{
    printf("%-20s   dstIP(dhost)\n",   dstIP.c_str());
    printf("%-20d   dstPort(dport)\n", dstPort);
    printf("%-20d   timeout(ms)\n",    TimeOut);
    printf("%-20s   transmode(bin <-> octet, ascii <-> netascii)\n", mode.c_str());
    printf("%-20s   Current fileName\n", fileName.c_str());
}

int InterfaceConf::run(const string& str)
{
    split_space(str);
    // for(string& s : cmlpart)
    //     cout << s << " ";
    if(cmlpart.empty()) {
        return 1;
    } else {

        iter = options.find(cmlpart[0]);
        if(iter == options.end()) {
            cout << "Syntax Error" << endl;
        }
        else if(iter->second < 2){
            if(iter->second == 1) {
                if(cmlpart[0] == "options")
                    this->printoption();
                else
                    this->printFile(cmlpart[0]);
            } else {
                cout << "Bye" << endl;
                return 0;
            }
        } else if (iter->second == 2) {
            fileName = (cmlpart.size() > 1) ? cmlpart[1] : fileName;
            UpLoad();
        } else if (iter->second == 3) {
            fileName = (cmlpart.size() > 1) ? cmlpart[1] : fileName;
            DownLoad();
        } else if (iter->second == 4) {
            iter = setparams.find(cmlpart[1]);
            if(iter == setparams.end()) {
                cout << "Syntax Error" << endl;
            } else if(cmlpart.size() != 3) {
                cout << "Wrong Param" << endl;
            } else {
                (this->*setbuf[iter->second])(cmlpart[2]);
            }
        } else {
            if(cmlpart.size() > 1) {
                cout << "Syntax Error" << endl;
            } else {
                system("cls");
            }
        }
        return 1;
    }
}

#endif