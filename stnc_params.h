#ifndef STNC_PARAMS_H
#define STNC_PARAMS_H
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#define SOCK_PATH "/tmp/uds_socket"
#define FILE_PATH "client_data.txt"
#define TYPE_IP4 1 
#define TYPE_IP6 2
#define TYPE_MMAP 3
#define TYPE_PIPE 4
#define TYPE_UDS 5

#define PARAM_CHAT 0
#define PARAM_TCP 1 
#define PARAM_UDP 2
#define PARAM_DGRAM 3
#define PARAM_STREAM 4
#define PARAM_FILENAME 5

#define FILENAME 256

int check_type(char* type);
int check_param(char* param);
bool check_combination(int type, int param);
uint8_t* generate_100MB(bool quiet_mode);
void generate_100_mega(bool quiet_mode);
void delete_file(bool quiet_mode);

typedef struct {
    int protocol;
    int transport;
    bool quiet_mode;
    char file_name[FILENAME];
} MessageType;

#endif