/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "util.h"


#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


#define BUF_SIZE 256

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    (void) signal(SIGALRM, alarmHandler);
    printf("Waiting message\n");

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
  

    /*
       while (1) {      //loop for input 
       res = read(fd,buf,1); 
      printf("res:%d\n",res);  // returns after 1 chars have been input 
      if(res == -1){
        printf("Error reading message\n");
        break;
      }
      mes[counter++]=buf[0];
      printf("%s\n",mes);
      if(buf[0]=='\0'){
        break;
      }
    }
    */



  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
