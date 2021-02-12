#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "TiqiaaUsb.h"

static FILE *io_file = NULL;
static bool signal_received;

static void test_callback(uint8_t * data, int size, class TiqiaaUsbIr * IrCls, void * context) {
    printf("INFO: Received data %d\n", size);
    fwrite(data, sizeof(char), size, io_file);
    fclose(io_file);
    signal_received = true;
}

static const char usage[] =
    "Usage: ir-usb [-s file_path] [-r file_path] [-r|-s ...]\n"
    "\n"
    "  -h   Show help message and quit\n"
    "  -r   Receive IR signal and store to file_path\n"
    "  -s   Send IR signal from file_path\n";

int main(int argc, char *argv[]) {
    int err = 0;
    int c;

    while( (c = getopt(argc, argv, "hr:s:")) != -1 ) {
        switch( c ) {
        case 'h':
            printf("%s", usage);
            return EXIT_SUCCESS;
        case 's':
        case 'r':
            break; // Just check it's ok
        case '?':
            if( isprint(optopt) )
              fprintf(stderr, "ERROR: Unknown option `-%c'.\n", optopt);
            else
              fprintf(stderr, "ERROR: Unknown option character `\\x%x'.\n", optopt);
            return 1;
        default:
            break;
        }
    }

    TiqiaaUsbIr Ir;
    Ir.IrRecvCallback = test_callback;
    if( Ir.Open() ) {
        fprintf(stderr, "INFO: Device opened\n");

        for( int i = 1; i < argc; i += 2 ) {
            bool send = argv[i][1] == 's';
            if( send ) {
                fprintf(stderr, "INFO: Reading signal from file: %s\n", argv[i+1]);
                io_file = fopen(argv[i+1], "rb");
            } else {
                fprintf(stderr, "INFO: Writing signal to file: %s\n", argv[i+1]);
                io_file = fopen(argv[i+1], "wb");
            }
            if( !io_file ) {
                fprintf(stderr, "ERROR: Unable to open file\n");
                return 1;
            }

            if( send ) {
                uint8_t *buffer;
                long size;
                // Get file size
                fseek(io_file, 0, SEEK_END);
                size = ftell(io_file);
                rewind(io_file);

                buffer = (uint8_t*)malloc(sizeof(uint8_t)*size);
                fread(buffer, sizeof(uint8_t), size, io_file);
                fclose(io_file);

                if( Ir.SendIR(38000, buffer, size) ) {
                    fprintf(stderr, "INFO: Sent IR signal\n");
                } else
                    fprintf(stderr, "ERROR: Unable to send IR\n");

                free(buffer);
            } else {
                signal_received = false;
                if( Ir.StartRecvIR() ) {
                    fprintf(stderr, "INFO: Waiting for IR signal\n");
                    while( !signal_received )
                        usleep(1000);
                } else
                    fprintf(stderr, "ERROR: Unable to receive IR\n");
            }
        }
    } else
        fprintf(stderr, "ERROR: Unable to open the device\n");

    fprintf(stderr, "INFO: Closing device\n");
    Ir.Close();

    return err >= 0 ? err : -err;
}
