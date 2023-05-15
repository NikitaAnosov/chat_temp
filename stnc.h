#ifndef STNC_H
#define STNC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include "stnc_params.h"
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_EVENTS 2 
#define BUFFER_SIZE 1024
#define DATA_SIZE 104857600
#define SOCK_PATH "/tmp/uds_socket"
#define FILE_PATH "client_data.txt"
int client_main(char* ip_addr, int port,  int type,  char* param, bool performance_mode);
void tcp_client_chat_ipv4(char* ip_addr, int port, bool quiet_mode);
void tcp_client_ipv4(char* ip_addr, int port, bool quiet_mode);
void tcp_client_ipv6(char* ip_addr, int port, bool quiet_mode);
void udp_client_ipv4(char* ip_addr, int port, bool quiet_mode);
void udp_client_ipv6(char* ip_addr, int port, bool quiet_mode);
void uds_client_dgram(char* ip_addr, int port, bool quiet_mode);
void uds_client_stream(char* ip_addr, int port, bool quiet_mode);
void mmap_client_filename(char *file_name, bool quiet_mode);
void pipe_client_filename(char *file_name, bool quiet_mode);

int server_main(int port, bool performance_mode, bool quiet_mode);
void tcp_server_chat_ipv4(int port, bool quiet_mode);
int tcp_server_ipv4(int port, bool quiet_mode, uint8_t *data);
int tcp_server_ipv6(int port, bool quiet_mode, uint8_t *data);
int udp_server_ipv4(int port, bool quiet_mode, uint8_t *data);
int udp_server_ipv6(int port, bool quiet_mode, uint8_t *data);
int uds_server_dgram(int port, bool quiet_mode, uint8_t *data);
int uds_server_stream(int port, bool quiet_mode, uint8_t *data);
int mmap_server_filename(bool quiet_mode, uint8_t* data, char *file_name);
int pipe_server_filename(bool quiet_mode, uint8_t* data, char *file_name);

#endif