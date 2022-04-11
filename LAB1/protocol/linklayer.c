#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "../application/linklayer.h"
#include "util.h"
#include <time.h>

int conta=0;
int flag = TRUE;
unsigned char current = C_N0;


linkLayer aux;
int general_fd;
struct termios oldtio,newtio;

void atende(int signal)                   // atende alarme
{
    conta++;
	flag=TRUE;
}

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(linkLayer connectionParameters){

    (void)signal(SIGALRM, atende);

    //socket setup
    int fd, res;

    int flag2 = FALSE;


    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(connectionParameters.serialPort); return -1; }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }


    general_fd = fd;

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");

    aux = connectionParameters;

    if(connectionParameters.role == TRANSMITTER){
        
        printf("llopen Transmitter\n");
        int state = START;

        unsigned char *buf;
        unsigned char buf1[5];

        int flag2 = FALSE;
        
        set_SET(buf1);

       
        do{
                    printf("Sending SET: %d\n ", conta);
                    res = write(fd,buf1, 5);
                    //printf("%d\n", res);
                    alarm(connectionParameters.timeOut);   
                    if(res == -1) {printf("Error sending SET\n"); return -1;}
                    else
                        printf("SET sent\n");
                    
                    flag = FALSE;

                    while(!flag2 && !flag){
                        res = read(fd,&buf,1);
                        //printf("res: %d\n");
                        if(res == -1)
                            {printf("Error read\n"); return -1;}
                        state = state_machine(state, &buf,BCC_UA, C_UA);
                        
                        if(state == FINISH){
                        printf("End of state machine\n");
                        printf("UA Received\n");
                        flag2 = TRUE;
                        break;
                     }
                }
           

        
        } while(conta < connectionParameters.numTries && flag );
        
        alarm(0);
        
        if(!flag2) {printf("Not UA received\n"); return -1;}
   
    }else if (connectionParameters.role == RECEIVER){

        unsigned char *buf;
        unsigned char buf1[5];
    
        set_UA(buf1);
    
        int state = START;
     
        while(1){

            while(state != FINISH){
                res = read(fd,&buf,1);
                    if(res == -1)
                        break;
                state=state_machine(state, &buf,BCC_SET, C_SET);
            }
             if(state == FINISH){
                    printf("End of state machine\n");
                    printf("SET Received\n");
                    break;
            }
        }
        
        res = write(fd,buf1,5);
    
            if(res == -1)
                printf("Error sending UA\n");
            else
                printf("UA sent\n");
            
        return 1;

    }else 
        return -1;
}

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(char *buf, int bufSize){
        if(buf == NULL || bufSize <= 0){
            printf("Input arguments of llwrite invalid\n");
            return -1;
        }
        if(bufSize > MAX_PAYLOAD_SIZE){
            printf("MAX_PAYLOAD_SIZE reached in llwrite\n");
            return -1;
        }

    printf("----------------llwrite starting------------------\n");  

    unsigned char *newBuf = (unsigned char *) malloc(sizeof(unsigned char)*(bufSize + 6));
    int newBufSize = bufSize + 6;

    int bcc2Size = 1;
    int pos;

    int res;

    unsigned char extra;

    unsigned char bcc2 = BCC2_CALC(buf,bufSize);
    unsigned char *bcc22 =  (unsigned char *)malloc(sizeof(unsigned char));
    bcc22=BCC22_CALC(bcc2, &bcc2Size);
    
    printf("> Constructiong I Frames\n");

    
    newBuf[0] = FLAG;
    newBuf[1] = A;
    if(current == C_N0)
        newBuf[2] = C_N0;
    else
        newBuf[2] = C_N1;
    
    newBuf[3] = newBuf[1]^newBuf[2]; // BCC1

    pos = stuffing(buf,newBuf,bufSize, newBufSize);

    if(bcc2Size == 1){
        newBuf[pos] = bcc2;
        pos++;
    }else{
        newBuf = (unsigned char *) realloc(newBuf, ++newBufSize);
        newBuf[pos] = bcc22[0];
        newBuf[pos+1] = bcc22[1];
        pos = pos +2;
    }

    newBuf[pos] = FLAG;

    int repeat = FALSE;

    int flag2 = FALSE;

    unsigned char control;

    printf("We are starting to write\n");


    printf("BufSize: %d\n", bufSize);
    
    printf("newBufSize: %d\n", newBufSize);
    do{

        res = write(general_fd, newBuf, newBufSize);
        if(res == -1) {printf("Error sending IFRAME\n"); return -1;}
        
        int state = START;


        alarm(aux.timeOut);

        printf(" > Frame sent\n");

        flag = FALSE;

        while(!flag2 && flag){
                        res = read(general_fd,&buf,1);
                        //printf("res: %d\n");
                        if(res == -1)
                            {printf("Error read\n"); return -1;}
                        state = controlMachine(state, &extra,&control);
                        
                        if(state == FINISH){
                        printf("End of state machine\n");
                        printf("ACK received\n");
                        flag2 = TRUE;
                        break;
                     }
            }
        if((control == RR1 && current == C_N0) || (control == RR0 && current == C_N1)){
            if(control == RR1)
                printf("> RR1 RECEIVED\n");
            else 
                printf("> RR0 RECEIVED\n");
            
            repeat = FALSE;
            if(current == C_N0)
                current = C_N1;
            else
                current = C_N0;
        }
        else if(control == REJ0 || control == REJ1){
            if(control == REJ1)
                printf(" > REJ1 Received\n");
            else
                printf(" > REJ0 Received\n");
            
            repeat = TRUE;
        }
    
    } while(repeat);
    /*
    int res;
    conta = 0;
    unsigned char C;
    int counter = 0;

    do{
        if(conta >= aux.timeOut){
            printf("NumTries max reached\n");
            return -1;
        }
        while(counter != newBufSize){
            res = write(general_fd,newBuf,newBufSize);
            counter =counter + res;
        if(res == -1) {printf("Error sending IFRAME\n"); return -1;}
        }
        conta++;
        alarm(aux.timeOut); 
        C = controlMachine_llwrite(general_fd, REJ_CALC(newBuf[2]), RR_CALC(newBuf[2]));  
        
    }while(C == REJ_CALC(newBuf[2]));
    
    alarm(0);
    conta = 0;
    
    current = CONTROL(current);

    free(newBuf);
    return newBufSize;
    */
    free(newBuf);
    if(conta >= aux.numTries)
        return -1;

    return res;

}
// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(char *packet){

    if(packet == NULL){
        printf("Packet Null\n");
        return -1;
    }
    int newSize= 0;
    int state = START;
    unsigned char buf;
    unsigned char *extra = (unsigned char *) malloc(newSize*sizeof(unsigned char));
    unsigned char flagi;
    int repreat = FALSE;
    unsigned char helper = C_N0;
    unsigned char control;
    int res;
    unsigned char bcc2, bcc2_calc;
    unsigned char buf1[5];

    unsigned char buf2[5];

    printf("----------------- Starting to read------------------\n");

    while(state !=FINISH){
        res = read(general_fd, &buf, 1);
        printf(">read stuff\n");
        switch(state){
            case(START):
                if(buf == FLAG)
                    state = FLAG_RCV;
                break;
            case(FLAG_RCV):
                if(buf == A)
                    state = A_RCV;
                else if (buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
            break;
            case(A_RCV):
                if(buf == C_N0){
                    state = C_RCV;
                    flagi = buf;
                    helper = C_N0;

                }
                else if(buf==C_N1){
                    state = C_RCV;
                    flagi = buf;
                    helper = C_N1;
                
                }
                else{
                    if(buf == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                }
            break;

            case(C_RCV):
                if(buf == (A^flagi))
                    state = BCC_OK;
                else 
                    state = START;
            break;

            case(BCC_OK):
                if(buf == FLAG){

                    if(flagBCC2(extra, newSize)){
                        printf("teste 1\n");
                        if(helper == C_N0){
                            set_RR(buf1,RR1);
                            write(general_fd, buf1, 5);
                            printf("RR1 Sent\n");
                        }
                        else{
                            set_RR(buf1,RR0);
                            write(general_fd, buf1, 5);
                            printf("RR0 Sent\n");
                        }
                        state = FINISH;
                    }
                    else{
                        if(helper == C_N0 && current == C_N1){
                            set_RR(buf1,RR1);
                            write(general_fd, buf1, 5);
                            printf("RR1 Sent\n");
                        }
                        if(helper == C_N1 && current == C_N0){
                            set_RR(buf1,RR0);
                            write(general_fd, buf1, 5);
                            printf("RR0 Sent\n");
                        }
                        else{
                            if(helper == C_N0){
                            set_REJ(buf2,REJ0);
                            write(general_fd, buf2, 5);
                            printf("REJ0 Sent\n");
                            }
                            else{
                            set_REJ(buf2,REJ1);
                            write(general_fd, buf2, 5);
                            printf("REJ1 Sent\n");
                            }
                          
                        }
                        state = FINISH;
                        repreat = TRUE;
                    }
                }
                else if(buf == ESCAPE)
                    state = ESCAPING;
                else{
                    extra = (unsigned char *)realloc(extra, ++newSize);
                    extra[newSize-1] = buf;
                }
                break;
            case(ESCAPING):
               if(buf == ESCAPE2){
                   extra = (unsigned char *)realloc(extra,++newSize);
                   extra[newSize -1 ]=FLAG;
               }
               else if(buf == ESCAPE3){
                   extra = (unsigned char *)realloc(extra,++newSize);
                   extra[newSize -1 ]=ESCAPE;
               }
               else{
                   printf("Error in ESCAPE STATE\n");
                   exit(-1);
               }
               state = BCC_OK;
               break;
    }
    /*
    do{
        res = read(general_fd, &buf,1);
        if(res == -1) {printf("Error reading something\n"); return -1;}

        switch (state)
        {
        case START:
            if(buf == FLAG)
                state = FLAG_RCV;
            break;
        case FLAG_RCV: 
            if(buf == A)
                state = A_RCV;
            else if (buf == FLAG)
                state = FLAG_RCV;

            else
                state = START;
            break;
        case A_RCV: 
            if(buf == C_N0 || buf == C_N1){
                flagi = buf;
                state = C_RCV;
            }
            else if (buf == FLAG)
                state = FLAG_RCV;
            else 
                state = START;
            break;
        case C_RCV:
            if(buf == (A^flagi)){
                state = BCC_OK;
                i = 0;
            }
            else
                state = START;
            break;
        case BCC_OK:
            ++newSize;
;
            extra = (unsigned char *) realloc(extra,newSize*sizeof(unsigned char));
            extra[i] = buf;
            if(buf == FLAG){


                if(newSize != -1){
                    for(int j=0;j<newSize;j++){
                        packet[j] = extra[j];
                    }

                    state = FINISH;
                } else{
                    printf("REJ sent\n");
                    return 0;
                }
            
            }
            i++;
            break;
        }

    }while(state != FINISH);
    
    free(extra);
    return newSize;
*/

}   
if(repreat){
    printf("I asked to repeat\n");
    }

    extra = (unsigned char *)realloc(extra, --newSize);
    if(newSize > 0){
        printf("NEw size: %d\n", newSize);
        if(helper == current){
            if(current == C_N0)
                current = C_N1;
            else
                current = C_N0;
        }
        else
            newSize = -1;
    }

    for(int i=0;i<newSize;i++){
        packet[i] = extra[i];
    }
    return newSize;

}

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics){

    (void)signal(SIGALRM, atende);

    //socket setup
    int res;

    int conta = 0;

    int flag2 = FALSE;

    if(aux.role == TRANSMITTER){
        
        int state = START;

        unsigned char *buf;
        unsigned char buf1[5];
        unsigned char buf2[5];

        int flag2 = FALSE;
        
        set_DISC(buf1);
        set_UA(buf2);
       
        do{
                    printf("Sending DISC: %d\n ", conta); 
                    res = write(general_fd,buf1, 5);
                    alarm(aux.timeOut);   
                    if(res == -1) {printf("Error sending DISC\n"); return -1;}
                    else
                        printf("DISC sent\n");
                    
                    flag = FALSE;

                    while(!flag2 && !flag){
                        res = read(general_fd,&buf,1);
                        if(res == -1)
                            {printf("Error read\n"); return -1;}
                        state = state_machine(state, &buf,BCC_DISC, C_DISC);
                        
                        if(state == FINISH){
                        printf("End of state machine\n");
                        printf("DISC Received\n");
                        res = write(general_fd,buf2, 5);
                        if(res == -1) {printf("Error sending UA\n"); return -1;}
                            else
                                printf("UA sent\n");
                        flag2 = TRUE;
                        break;
                     }
                }
           

        
        } while(conta < aux.numTries && !flag );
        
        alarm(0);
        
        if(!flag2) {printf("Not DISC received\n"); return -1;}

    }else if(aux.role == RECEIVER){

        unsigned char *buf;
        unsigned char buf1[5];

        set_DISC(buf1);

        int state = START;
     
        while(1){

            while(state != FINISH){
                res = read(general_fd,&buf,1);
                    if(res == -1)
                        break;
                state=state_machine(state, &buf,BCC_DISC, C_DISC);
            }
             if(state == FINISH){
                    printf("End of state machine\n");
                    printf("DISC Received\n");
                    break;
            }
        }

        do{
                    printf("Sending DISC: %d\n ", conta);
                    res = write(general_fd,buf1, 5);
                    //printf("%d\n", res);
                    alarm(aux.timeOut);   
                    if(res == -1) {printf("Error sending DISC\n"); return -1;}
                    else
                        printf("DISC sent\n");
                    

                    while(!flag2 && !flag){
                        res = read(general_fd,&buf,1);
                        //printf("res: %d\n");
                        if(res == -1)
                            {printf("Error read\n"); return -1;}
                        state = state_machine(state, &buf,BCC_UA, C_UA);
                        
                        if(state == FINISH){
                        printf("End of state machine\n");
                        printf("UA Received\n");
                        flag2 = TRUE;
                        break;
                     }
                }
           

        
        } while(conta < aux.numTries && flag );
        
        alarm(0);
        
        if(!flag2) {printf("Not UA received\n"); return -1;}        
        /*res = write(general_fd,buf1,5);
            
            if(res == -1)
                printf("Error sending DISC\n");
            else
                printf("Disc sent\n");
         */   
    }else 
        return -1;
    
    if(tcsetattr(general_fd,TCSANOW,&oldtio)==-1){
        printf("Error reseting serial port\n");
        return -1;
    }else
        close(general_fd);
        printf("Closed!\n Stats\n");
    
    return 1;
}