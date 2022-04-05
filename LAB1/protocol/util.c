#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include "util.h"


void set_SET(unsigned char *buf){
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C_SET;
    buf[3] = BCC_SET; //BCC
    buf[4] = FLAG;
}

void set_UA(unsigned char *buf){
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C_UA;
    buf[3] = BCC_UA; //BCC
    buf[4] = FLAG;
}

void set_DISC(unsigned char *buf){
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C_DISC;
    buf[3] = BCC_DISC; //BCC
    buf[4] = FLAG;
}

int state_machine(int state, unsigned char *buf, int bcc, int C){
    switch(state){
    
        case START:
            if(*buf == FLAG){
                state = FLAG_RCV;
            }
            else 
                state = START;
            break;

        case FLAG_RCV:
            if(*buf == A)
                state = A_RCV;
            
            else if(*buf == FLAG)
                state = FLAG_RCV;
            else 
                state == START;
            break;        

        case A_RCV:
            if(*buf == C)
                state = C_RCV;        
            else if(*buf == FLAG)
                state = FLAG_RCV;
            else 
                state = START;
            break;

        case C_RCV:
            if(*buf == FLAG)
                state = FLAG_RCV;
            else if(*buf == bcc)
                state = BCC_OK;
            else 
                state = START;
            break;

        case BCC_OK:
            if(*buf == FLAG)
                state = FINISH;
            else
                state = START;
            break;

        default :
            state = START;

    }
    return state;
}

