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
        return -1;
}
// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(char *packet){
        return -1;
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
           

        
        } while(conta < aux.numTries && flag );
        
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
                    
                    flag = FALSE;

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