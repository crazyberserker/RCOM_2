#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include "util.h"
#include <stdlib.h>


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

void set_RR(unsigned char *buf, unsigned char curr){
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = curr;
    buf[3] = buf[1]^buf[2]; //BCC
    buf[4] = FLAG;

}

void set_REJ(unsigned char *buf, unsigned char curr){
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = curr;
    buf[3] = buf[1]^buf[2]; //BCC
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

unsigned char BCC2_CALC(unsigned char *buf, int bufSize){

    unsigned char bcc2 = buf[0];

    for(int i=1; i < bufSize; i++){
        
        bcc2^=buf[i];
    }

    return bcc2;
}

unsigned char *BCC22_CALC(unsigned char BCC2, int *bcc2Size){

    unsigned char *aux;

    if(BCC2 == FLAG){
        aux = (unsigned char *) malloc(2*sizeof(unsigned char));
        aux[0] = ESCAPE;
        aux[1] = ESCAPE2;
        (*bcc2Size)++;
    }
    else{
        if(BCC2 == ESCAPE){
            aux = (unsigned char *) malloc(2*sizeof(unsigned char));
            aux[0] = ESCAPE;
            aux[1] = ESCAPE2;
            (*bcc2Size)++;
        }        
    }


    return aux;
}

int stuffing(char *buf, unsigned char *newBuf, int bufSize, int newBufSize){

    int pos =4;

    for(int i=0; i<bufSize; i++){
        if(buf[i] == FLAG ){
            ++newBufSize;
            newBuf = (unsigned char *)realloc(newBuf,newBufSize);
            newBuf[pos] = ESCAPE;
            newBuf[pos+1] =  ESCAPE2;
            pos=pos+2;
        }else if(buf[i] == ESCAPE){
            ++newBufSize;
            newBuf = (unsigned char *)realloc(newBuf,newBufSize);
            newBuf[pos] = ESCAPE;
            newBuf[pos+1] =  ESCAPE3;
            pos=pos+2;
        }
        else{
            newBuf[pos] = buf[i];
            pos++;
        }
    }
    return pos;
}

unsigned char REJ_CALC(unsigned char curr){
    if(curr == C_N0)
        return REJ1;
    else 
        return REJ0;

}

unsigned char RR_CALC(unsigned char curr){

    if(curr == C_N0)
        return RR1;
    else 
        return RR0;
}

unsigned char controlMachine_llwrite(int fd, unsigned char rej, unsigned char rr){
    unsigned char a,C, buf;
    int state = START;
    unsigned char flagi;
    int res;

    do{ 
        res = read(fd,&buf,1);
        if(res == -1) {printf("Error reading ACK\n"); return -1;}

        switch(state){
            case START:
                if(buf == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
                a = buf;
                if(buf == A)
                    state = A_RCV;
                else if (buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
                C = buf;
                if(C == rej || C == rr){
                    flagi = C;
                    state = C_RCV;
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;

            case C_RCV:
                if(buf == FLAG)
                    state = FLAG_RCV;
                else if (buf == (a^C))
                    state = BCC_OK;
                else 
                    state = START;
                break;

            case BCC_OK:
                if(buf == FLAG){
                    state = FINISH;
                }
                else
                    state = START;
                break; 
        }

    }while(state!=FINISH);

    if(C == RR0 || C == RR1)
        printf("Received RR\n");
    
    if(C == REJ0 || C == REJ1)
        printf("Received REJ\n");

 
    return flagi;

}


int controlMachine(int state, unsigned char *buf, unsigned char *curr){
    switch (state){
    case(START):
        if(*buf == FLAG)
            state = FLAG_RCV;
        break;
    case(FLAG_RCV):
        if(*buf == A)
            state = A_RCV;
        else if (*buf == FLAG)
            state = FLAG_RCV;
        else 
            state = START;
        break;

    case(A_RCV):
        if(*buf == RR0 || *buf == RR1 || *buf == REJ0 || *buf == REJ1){
            state = C_RCV;
            *curr = *buf;
        }
        else if (*buf == FLAG)
            state = FLAG_RCV;
        else 
            state = START;
        break;
    case(C_RCV):
        if(*buf == (A^ *curr))
            state = BCC_OK;
        else
            state = START;
        break;
    case(BCC_OK):
        if(*buf == FLAG)
            state = FINISH;
        else
            state = START;
        break;
    }
    return state;
}

int flagBCC2(unsigned char *buf, int bufSize){
    
    unsigned char BCC2 = buf[0];
    for(int i =1; i<bufSize-1;i++){
        BCC2^=buf[i];
    }
    if(BCC2 == buf[bufSize-1]){
        printf("HElper deu 1 aqui\n");
        return 1;
    }
    else{
        printf("HElper deu 0 aqui\n");
 
        return 0;}
    
}
