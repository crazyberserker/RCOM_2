#include "linklayer.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BAUDRATE 9600
#define NUM_RETRANSMISSIONS 3
#define TIMEOUT 3

#define FINISH_TRANSMISSION 0
#define CONTINUE_TRANSMISSION 1

/*
 * $1 /dev/ttySxx
 * $2 tx | rx
 * $3 filename
 */

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s /dev/ttySxx tx|rx filename\n", argv[0]);
        exit(1);
    }

    char *serialPort = argv[1];
    char *role = argv[2];
    char *filename = argv[3];

    printf("%s %s %s\n", serialPort, role, filename);
    fflush(stdout);
    fflush(stderr);

    if (strcmp(role, "tx") == 0)
    {
        // ***********
        // Tx mode
        printf("Tx mode\n");

        // Open connection
        struct linkLayer ll;
        strcpy(ll.serialPort, serialPort);
        ll.role = TRANSMITTER;
        ll.baudRate = BAUDRATE;
        ll.numTries = NUM_RETRANSMISSIONS;
        ll.timeOut = TIMEOUT;

        if(llopen(ll) == -1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }

        printf("Connection opened\n");
        fflush(stdout);
        fflush(stderr);

        // Open file to read
        int file_desc = open(filename, O_RDONLY);
        if(file_desc < 0) {
            fprintf(stderr, "Error opening file: %s\n", filename);
            exit(1);
        }

        // Cycle through
        const int buf_size = MAX_PAYLOAD_SIZE - 1;
        char buffer[buf_size + 1];
        int bytes_written = 0;
        int bytes_read = 1;
        int total_bytes_sent = 0;

        while (bytes_read > 0)
        {
            bytes_read = read(file_desc, buffer + 1, buf_size);

            if (bytes_read < 0) {
                fprintf(stderr, "Error reading from file\n");
                break;
            }
            else if (bytes_read > 0) {
                // Continue sending data
                buffer[0] = CONTINUE_TRANSMISSION;

                bytes_written = llwrite(buffer, bytes_read + 1);
                if (bytes_written < 0) {
                    fprintf(stderr, "Error sending data to link layer\n");
                    break;
                }
                else if (bytes_written != bytes_read + 1) {
                    fprintf(stderr, "Error sending data to link layer: bytes written (%d) != bytes read (%d)\n",
                        bytes_written, (bytes_read + 1));
                    break;
                }

                total_bytes_sent += bytes_written;

                printf("Read from file (%d) -> Write to link layer (%d), Total bytes sent: %d\n",
                    bytes_read, bytes_written, total_bytes_sent);
            }
            else if (bytes_read == 0) {
                // Stop receiver
                buffer[0] = FINISH_TRANSMISSION;

                llwrite(buffer, 1);
                printf("App layer: done reading and sending file\n");
                break;
            }

            sleep(1);
        }
        // Close file
        close(file_desc);
    }
    else
    {
        // ***************
        // Rx mode
        printf("Rx mode\n");

        struct linkLayer ll;
        strcpy(ll.serialPort, serialPort);
        ll.role = RECEIVER;
        ll.baudRate = BAUDRATE;
        ll.numTries = NUM_RETRANSMISSIONS;
        ll.timeOut = TIMEOUT;

        if (llopen(ll) == -1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }

        int file_desc = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (file_desc < 0) {
            fprintf(stderr, "Error opening file: %s\n", filename);
            exit(1);
        }

        int bytes_read = 0;
        int bytes_written = 0;
        const int buf_size = MAX_PAYLOAD_SIZE;
        char buffer[buf_size];
        int total_bytes_received = 0;

        while (bytes_read >= 0)
        {
            bytes_read = llread(buffer);

            if (bytes_read < 0) {
                fprintf(stderr, "Error receiving from link layer\n");
                break;
            }
            else if (bytes_read > 0) {
                if (buffer[0] == CONTINUE_TRANSMISSION) {
                    bytes_written = write(file_desc, buffer + 1, bytes_read - 1);
                    if (bytes_written < 0 || bytes_written != bytes_read - 1) {
                        fprintf(stderr, "Error writing to file\n");
                        break;
                    }

                    total_bytes_received += bytes_read;

                    printf("Read from link layer (%d) -> Write to file (%d), Total bytes received: %d\n",
                        bytes_read, bytes_written, total_bytes_received);
                }
                else if (buffer[0] == FINISH_TRANSMISSION) {
                    printf("App layer: done receiving file\n");
                    break;
                }
            }
        }

        // Close file
        close(file_desc);
    }

    // Close connection
    llclose(TRUE);
    return 0;
}
