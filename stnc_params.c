#include "stnc.h"

int check_type(char* type) {
    if(strcmp(type, "ipv4") == 0) {
        return TYPE_IP4;
    }
    else if(strcmp(type, "ipv6") == 0) {
        return TYPE_IP6;
    }
    else if(strcmp(type, "mmap") == 0) {
        return TYPE_MMAP;
    }
    else if(strcmp(type, "pipe") == 0) {
        return TYPE_PIPE;
    }
    else if(strcmp(type, "uds") == 0) {
        return TYPE_UDS;
    }
    return -1;
}

int check_param(char* param) {
    if(strcmp(param, "chat") == 0) {
        return PARAM_CHAT;
    }
    if(strcmp(param, "tcp") == 0) {
        return PARAM_TCP;
    }
    else if(strcmp(param, "udp") == 0) {
        return PARAM_UDP;
    }
    else if(strcmp(param, "dgram") == 0) {
        return PARAM_DGRAM;
    }
    else if(strcmp(param, "stream") == 0) {
        return PARAM_STREAM;
    }
    return PARAM_FILENAME;
}

bool check_combination(int type, int param) {
    if (type == TYPE_IP4 || type == TYPE_IP6) {
        return param == PARAM_TCP || param == PARAM_UDP;
    }
    else if(type == TYPE_UDS) {
        return param == PARAM_DGRAM || param == PARAM_STREAM;
    }
    else if(type == TYPE_MMAP || type == TYPE_PIPE) {
        return param == PARAM_FILENAME;
    }
    return false;
}

void generate_100_mega(bool quiet_mode) {
    FILE *file;

    file = fopen("client_data.txt", "wb");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }

    for (int i = 0; i < DATA_SIZE; i++) {
        fputc('1', file);
    }

    fclose(file);
    if(!quiet_mode) printf("File generated successfully.\n");
}

uint8_t* generate_100MB(bool quiet_mode) {
    uint8_t *file =NULL;

    file = (uint8_t *)malloc(DATA_SIZE * sizeof(uint8_t));

    for(uint32_t i = 0; i <DATA_SIZE; i++) {
        *(file + i) = (uint32_t)rand() % 256;
    }
    if(!quiet_mode) printf("Data generated successfully.\n");

    return file;
}

void delete_file(bool quiet_mode) {
    int status;
    char filename[] = "client_data.txt";
    
    status = remove(filename);
    if (status == 0) {
        if(!quiet_mode) printf("%s file deleted successfully.\n", filename);
    } else {
        printf("Error deleting %s file.\n", filename);
    } 
}