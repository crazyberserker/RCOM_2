#ifndef _UTIL_H

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define BCC_SET (A^C_SET)
#define BCC_UA (A^C_UA)

#define TIMEOUT 3

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define FINISH 5


void set_SET(unsigned char *buf);
    
void set_UA(unsigned char *buf);

int state_machine(int state, unsigned char *buf,int bcc);

#endif
