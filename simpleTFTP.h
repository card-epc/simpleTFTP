#ifndef SIMAPLETFTP
#define SIMAPLETFTP
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <map>
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
#define Square      "█"
#define ZEROCHAR    char(0)

using namespace std;


string  fileName = "FLEX.pdf";
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
    if((size>>23) > 0) {
        num = size/8.0/1024/1024;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" MB";
    } else if((size>>13) > 0) {
        num = size/8.0/1024;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" KB";
    } else if((size>>3) > 0) {
        num = size/8.0;
        sprintf(ss, "%.2lf", num);
        return string(ss)+" Bytes";
    } else {
        return to_string(size)+" bits";
    }
}

int printRate(size_t rsize, size_t tsize, double tt) {
    int r = 50*rsize/tsize;
    putchar('\r');
    for(int i=0; i<r; i++)
        printf(Square);
    for(int i=r+1; i<=50; i++)
        putchar(' ');
    putchar('|');
    fflush(stdout);
    cout << byteConvert(rsize) << "/" << byteConvert(tsize) << "            ";
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

class InterfaceConf {
    private:
        string promt;
        string param;
        vector<string> cmlpart;
        void (InterfaceConf::*funcptr[4])(const string&);
        map<string, int> options;
        map<string, int>::iterator iter;
        void printhelp()
        {
            FILE *fp;
            int  len;
            char *data;
            fp = fopen("help", "rb");
            fseek(fp, 0, SEEK_END);
            len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            data = (char*)malloc(len + 1);
            data[len] = 0;
            fread(data, 1, len, fp);
            fclose(fp);
            printf("%s\n", data);
        }
        void update() {
            promt = "to " + dstIP + ":" + to_string(dstPort) + " > ";
            // cout << promt << endl; 
        }
        void split(const string& str) {
            cmlpart.clear();
            string temp;
            for(int i = 0; i<str.size(); i++) {
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
            options.emplace("q", 0);
            options.emplace("h", 1);
            options.emplace("help", 1);
            options.emplace("quit", 0);
            options.emplace("exit", 0);
            options.emplace("set",  4);
            options.emplace("mode", 5);
            options.emplace("dhost",    6);
            options.emplace("dport",    7);
            options.emplace("timeout",  8);
            options.emplace("upload",   2);
            options.emplace("download", 3);
            update();
            funcptr[1] = &InterfaceConf::setdip;
            funcptr[2] = &InterfaceConf::setdport;
            funcptr[3] = &InterfaceConf::setTimeout;
            funcptr[0] = &InterfaceConf::setTransmode;
        }
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
            dstIP = str;
            update();
        }        
        void setdport(const string& str) {
            for(char c : str) {
                if(!isdigit(c)) {
                    cout << "Syntax Error" << endl;
                    return;
                }
            }
            dstPort = stoi(str);
            update();
        }        
        void setTransmode(const string& str) {
            if(str == "bin" || str == "ascii")
                mode = (str=="bin") ? BIN : ASCII;
            else
                cout << "Mode can't found" << endl;
            return;
        }    
        void print() {
            cout << promt;
        }
        int run(const string& str) {
            split(str);
            // for(string& s : cmlpart)
            //     cout << s << " ";
            if(cmlpart.empty()) {
                return 1;
            } else {

                iter = options.find(cmlpart[0]);
                if(iter == options.end() || iter->second > 4) {
                    cout << "Syntax Error" << endl;
                }
                else if(iter->second < 2){
                    if(iter->second == 1) {
                        printhelp();
                    } else {
                        cout << "Bye" << endl;
                        return 0;
                    }
                }
                else if(iter->second == 2) {
                    fileName = (cmlpart.size() > 1) ? cmlpart[1] : fileName;
                    UpLoad();
                } else if(iter->second == 3) {
                    fileName = (cmlpart.size() > 1) ? cmlpart[1] : fileName;
                    DownLoad();
                }
                else {
                    iter = options.find(cmlpart[1]);
                    if(iter == options.end()) {
                        cout << "Syntax Error" << endl;
                    } else if(cmlpart.size() != 3) {
                        cout << "Param needed" << endl;
                    } else {
                        (this->*funcptr[iter->second-5])(cmlpart[2]);
                    }
                }
                return 1;
            }
        }
};

#endif