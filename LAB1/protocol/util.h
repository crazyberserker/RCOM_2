#ifndef _UTIL_H

//Protocol Framing constants
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define BCC_SET (A^C_SET)
#define BCC_UA (A^C_UA)
#define BCC_DISC (A^C_DISC)
#define ESCAPE 0x7D
#define ESCAPE2 0X5E
#define ESCAPE3 0x5D


//State Machine 
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define FINISH 5
#define ESCAPING 6


// I frame control flags
#define C_N0 0x00
#define C_N1 0x02
#define RR0 0x01
#define RR1 0x21
#define REJ0 0x05
#define REJ1 0x25


//C control for the information frames

void set_SET(unsigned char *buf);
    
void set_UA(unsigned char *buf);

void set_DISC(unsigned char *buf);

void set_RR(unsigned char *buf, unsigned char curr);

void set_REJ(unsigned char *buf, unsigned char curr);

int state_machine(int state, unsigned char *buf,int bcc, int C);

unsigned char CONTROL(unsigned char current);

unsigned char BCC2_CALC(unsigned char *buf, int bufSize);

unsigned char *BCC22_CALC(unsigned char BCC2, int *bcc2Size);

int stuffing(char *buf, unsigned char *newBuf, int bufSize, int newBufSize);

unsigned char REJ_CALC(unsigned char curr);

unsigned char RR_CALC(unsigned char curr);

unsigned char controlMachine_llwrite(int fd,unsigned char rej, unsigned char rr);

//int destuffing(char *buf, int bufSize);

int flagBCC2(unsigned char *buf, int bufSize);

#endif
