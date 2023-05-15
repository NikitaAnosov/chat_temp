#include "stnc.h"

int main(int argc, char *argv[]) {
    int port, type, param;
    bool performance_mode = false;
    bool quiet_mode = false;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -s PORT | -c IP PORT\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    else if (strcmp(argv[1], "-s") == 0) {
        port = atoi(argv[2]);

        if (argc >= 4) {
            performance_mode = strcmp(argv[3], "-p") == 0? true : false;
            quiet_mode = strcmp(argv[3], "-q") == 0? true : false;

            if (argc == 5) {
                quiet_mode = strcmp(argv[4], "-q") == 0? true : false;
            }
        }
    
        server_main(port, performance_mode, quiet_mode);
    }
    else if (strcmp(argv[1], "-c") == 0) {
        port = atoi(argv[3]);

        if(argc == 4) {
            performance_mode = false;
            type = TYPE_IP4;
            client_main(argv[2], port, type, argv[5], performance_mode);
        }
        else {
            if (strcmp(argv[4], "-p") == 0) {
                performance_mode = true;
                type = check_type(argv[5]);
                param = check_param(argv[6]);
                if(check_combination(type, param)) {
                    client_main(argv[2], port, type, argv[6], performance_mode);
                }
                else {
                    fprintf(stderr, "Invalid combination\n");
                }
            }
            else {
                type = check_type(argv[4]);
                param = check_param(argv[5]);
                if(check_combination(type, param)) {
                    client_main(argv[2], port, type, argv[5], performance_mode);
                }
                else {
                    fprintf(stderr, "Invalid combination\n");
                }
            }
        }
    }
    else {
        fprintf(stderr, "Usage: %s -s PORT | -c IP PORT\n", argv[0]);
    }

    return 0;
}