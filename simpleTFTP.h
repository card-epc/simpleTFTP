#ifndef SIMAPLETFTP
#define SIMAPLETFTP
#include <Windows.h>
#include <cstdio>

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

char Sqbuf[300];
char Spbuf[300];
int  pos;
int  x, y;
const int rpos = 200;

int printRate(int rsize, int tsize, double tt) {
    int r = 50*rsize/tsize;
    for(int i=0; i<r; i++)
        printf(Square);
    for(int i=r+1; i<50; i++)
        putchar(' ');
    putchar('|');
    fflush(stdout);
    printf("%d bits/%d bits \r", rsize, tsize);
}

// void GetCurrentCursorPosition(int &x,int &y)
// {
//     HANDLE   hStdout;
//     CONSOLE_SCREEN_BUFFER_INFO   pBuffer;
//     hStdout   =   GetStdHandle(STD_OUTPUT_HANDLE);
//     GetConsoleScreenBufferInfo(hStdout,   &pBuffer);
//     x=pBuffer.dwCursorPosition.X;
//     y=pBuffer.dwCursorPosition.Y;
// }



// class ProgressBar {
//     private:
//         int x, y, pos;
//         int rsize, tsize;
//         double tt;
//         double tp;
//         const int rpos = 200;
//         inline void setLocation() {
//             printf("%c[%d;%dH",27, y, pos);
//         }
//     public:
//         ProgressBar(int size) : tsize(size), pos(0) {} 
//         inline void initLocation() {
//             GetCurrentCursorPosition(x, y);
//             // cout << x << ":" << y << endl;
//             printf("%d %d", x, y);
//             system("pause");
//         }
//         inline void changeState(int size, double t) {
//             rsize = size; tt = t; tp = rsize/t;
//         }
//         // inline void print() {
//         //     int idx = rsize*100/tsize;
//         //     setLocation();
//         //     for(; pos <= idx*2; pos+=2) {
//         //         printf(Square);
//         //     }
//         // }
//         inline void printRate() {
//             int r = (rsize*100/tsize)/5;
//             // if(tsize > 10000000) {
//             //     printf("%.2lfM bits/%.2lfM bits ", rsize/1000000.0, tsize/1000000.0);
//             // }
//             // else if(tsize > 10000) {
//             //     printf("%.2lfK bits/%.2lfK bits ", rsize/1000.0, tsize/1000.0);
//             // }
//             // else {
//                 printf("%d bits/%d bits ", rsize, tsize);
//             // }
//             if(r > pos) {
//                 for(int i = 0; i < r; i++)
//                     printf(Square);
//                 for(int i = r+1; i<20; i++)
//                     putchar(' ');
//                 pos = r;
//             }
//             putchar('\r');
//         }
// };

void HideCursor()
{
	CONSOLE_CURSOR_INFO cursor;    
	cursor.bVisible = FALSE;    
	cursor.dwSize = sizeof(cursor);    
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);    
	SetConsoleCursorInfo(handle, &cursor);
}



// void Gotoxy(int x, int y)
// {    
//     COORD c;    
//     c.X = x ;    
//     c.Y = y ;    
//     SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), c);    
// }

// inline void initProgressBar()
// {
//     // putchar('9');
//     // GetCurrentCursorPosition(x, y);
//     // Gotoxy(rpos, y);
//     // putchar('|');
//     for(int i=0; i<100; i++) {
//         strcat(Sqbuf, Square);
//         Spbuf[i] = ' ';
//     }
// }

// inline void ProgressBar(double time, int rsize, int tsize)
// {
//     double speed = rsize/time;
//     int i = rsize*100/tsize;    
//     // Gotoxy(pos*2, y);
//     // for(pos; pos<=i*2; pos+=2) {
//     //     printf(Square);
//     // }
//     printf("\r");
//     // printf("%s", Sqbuf+(100-i)*2);
//     // printf("%s", Spbuf+i);
//     // putchar('|');
//     // Gotoxy(rpos, y);
//     printf("%.2lfbps %dbit/%dbit", rsize/time, rsize, tsize);
//     fflush(stdout);
//     // printf("%d", i);
//     // putchar('%');

// }

#endif