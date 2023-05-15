#include "stnc.h"

int client_main(char *ip_addr, int port, int type, char* param, bool performance_mode) {
    int chat_server;
    struct sockaddr_in server_addr;
    MessageType msg;
    bool quiet_mode = false;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    chat_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (chat_server == -1)
    {
        fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(chat_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (connect(chat_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    msg.protocol = type;
    msg.transport = (type != TYPE_MMAP && type != TYPE_PIPE)? check_param(param) : PARAM_FILENAME;
    msg.quiet_mode = quiet_mode;
    if(type == TYPE_MMAP || type == TYPE_PIPE){
        strncpy(msg.file_name, param, FILENAME);
    }

    ssize_t num_written = send(chat_server, &msg, sizeof(msg), 0);
    if (num_written < 0)
    {
        perror("send");
        exit(1);
    }

    recv(chat_server, &msg, sizeof(msg), 0);
    quiet_mode = msg.quiet_mode;

    switch (msg.transport)
    {
  case PARAM_TCP:
        if (msg.protocol == TYPE_IP4)
        {
            if (performance_mode)
                tcp_client_ipv4(ip_addr, port + 1, quiet_mode);
            else
                tcp_client_chat_ipv4(ip_addr, port + 1, quiet_mode);
        }
        else if (msg.protocol == TYPE_IP6)
            tcp_client_ipv6(ip_addr, port + 1, quiet_mode);
        break;
    case PARAM_UDP:
        if (msg.protocol == TYPE_IP4)
            udp_client_ipv4(ip_addr, port + 1, quiet_mode);
        else if (msg.protocol == TYPE_IP6)
            udp_client_ipv6(ip_addr, port + 1, quiet_mode);
        break;
    case PARAM_DGRAM:
        if (msg.protocol == TYPE_UDS)
            uds_client_dgram(ip_addr, port + 1, quiet_mode);
        break;
    case PARAM_STREAM:
        if (msg.protocol == TYPE_UDS)
            uds_client_stream(ip_addr, port + 1, quiet_mode);
        break;
    case PARAM_FILENAME:
        if (msg.protocol == TYPE_MMAP)
            mmap_client_filename(msg.file_name, quiet_mode);
        else if (msg.protocol == TYPE_PIPE)
            pipe_client_filename(msg.file_name, quiet_mode);
        break;
    }
    return 0;
}

void tcp_client_chat_ipv4(char *ip_addr, int port, bool quiet_mode)
{
    int client_sock; // Client socket
    struct sockaddr_in server_addr;
    int fd_count;
    char buffer[BUFFER_SIZE];
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Connected to server.\n");

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    fd_count = 2;

    while (1)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[0].revents & POLLIN)
        {
            ssize_t num_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (num_bytes <= 0)
            {
                perror("read");
                exit(1);
            }
            buffer[num_bytes] = '\0';
            ssize_t num_written = send(client_sock, buffer, strlen(buffer), 0);
            if (num_written < 0)
            {
                perror("send");
                exit(1);
            }
            bzero(buffer, BUFFER_SIZE);
        }
        if (pfds[1].revents & POLLIN)
        {
            ssize_t num_bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);
            if (num_bytes < 0)
            {
                perror("recv");
                exit(1);
            }
            if (num_bytes == 0)
            {
                if (!quiet_mode)
                    printf("Server closed connection.\n");
                exit(1);
            }
            buffer[num_bytes] = '\0';
            if (!quiet_mode)
                printf("Server:\t %s", buffer);
            bzero(buffer, BUFFER_SIZE);
        }
    }
    close(client_sock);
}

void tcp_client_ipv4(char *ip_addr, int port, bool quiet_mode)
{
    printf("Connected to server.\n");
    int client_sock; // Client socket
    struct sockaddr_in server_addr;
    int fd_count;
    char buffer[BUFFER_SIZE];
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    // int enableReuse = 1;
    // if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
    //     if (!quiet_mode)
    //         perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }

    socklen_t server_addr_len = sizeof(server_addr);
    if (connect(client_sock, (struct sockaddr *)&server_addr, server_addr_len) == -1)
    {
        if (!quiet_mode)
            perror("connect");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Connected to server.\n");

    uint8_t *data = generate_100MB(quiet_mode);

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLOUT;
    fd_count = 2;

    while (bytes_sent < DATA_SIZE)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[0].revents & POLLIN)
        {
            ssize_t num_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (num_bytes <= 0)
            {
                if (!quiet_mode)
                    perror("read");
                exit(1);
            }
        }
        if (pfds[1].revents & POLLOUT)
        {
            int bytes_left = (DATA_SIZE - bytes_sent) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_sent);

            int curr_bytes_sent = send(client_sock, data + bytes_sent, bytes_left, 0);
            if (curr_bytes_sent <= 0)
            {
                if (!quiet_mode)
                    perror("send");
                close(client_sock);
                exit(EXIT_FAILURE);
            }

            bytes_sent += curr_bytes_sent;
        }
    }
    close(client_sock);
}

void tcp_client_ipv6(char *ip_addr, int port, bool quiet_mode)
{
    int client_sock; // Client socket
    struct sockaddr_in6 server_addr;
    int fd_count;
    char buffer[BUFFER_SIZE];
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip_addr, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    client_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        if (!quiet_mode)
            perror("connect");
        exit(EXIT_FAILURE);
    }
    if (!quiet_mode)
        printf("Connected to server.\n");

    uint8_t *data = generate_100MB(quiet_mode);

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLOUT;
    fd_count = 2;

    while (bytes_sent < DATA_SIZE)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[0].revents & POLLIN)
        {
            ssize_t num_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (num_bytes <= 0)
            {
                if (!quiet_mode)
                    perror("read");
                exit(1);
            }
        }
        if (pfds[1].revents & POLLOUT)
        {
            int bytes_left = (DATA_SIZE - bytes_sent) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_sent);

            int curr_bytes_sent = send(client_sock, data + bytes_sent, bytes_left, 0);
            if (curr_bytes_sent <= 0)
            {
                if (!quiet_mode)
                    perror("send");
                close(client_sock);
                exit(EXIT_FAILURE);
            }

            bytes_sent += curr_bytes_sent;
        }
    }
    close(client_sock);
}

void udp_client_ipv4(char *ip_addr, int port, bool quiet_mode)
{
    int client_sock; // Client socket
    struct sockaddr_in server_addr;
    int fd_count;
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_sock == -1)
    {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create client socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    uint8_t *data = generate_100MB(quiet_mode);
    socklen_t server_addrLen = sizeof(server_addr);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLOUT;
    fd_count = 2;

    while (bytes_sent < DATA_SIZE)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[1].revents & POLLOUT)
        {
            int bytes_left = (DATA_SIZE - bytes_sent) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_sent);

            int curr_bytes_sent = sendto(client_sock, data + bytes_sent, bytes_left, 0, (struct sockaddr *)&server_addr, server_addrLen);
            if (curr_bytes_sent <= 0)
            {
                if (!quiet_mode)
                    perror("send");
                exit(EXIT_FAILURE);
            }

            bytes_sent += curr_bytes_sent;
        }
    }
    close(client_sock);
}
void udp_client_ipv6(char *ip_addr, int port, bool quiet_mode)
{
    int client_sock; // Client socket
    struct sockaddr_in6 server_addr;
    int fd_count;
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip_addr, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    client_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (client_sock == -1)
    {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    uint8_t *data = generate_100MB(quiet_mode);

    socklen_t server_addrLen = sizeof(server_addr);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLOUT;
    fd_count = 2;

    while (bytes_sent < DATA_SIZE)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }
        if (pfds[1].revents & POLLOUT)
        {
            int bytes_left = (DATA_SIZE - bytes_sent) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_sent);

            int curr_bytes_sent = sendto(client_sock, data + bytes_sent, bytes_left, 0, (struct sockaddr *)&server_addr, server_addrLen);
            if (curr_bytes_sent <= 0)
            {
                if (!quiet_mode)
                    perror("send");
                close(client_sock);
                exit(EXIT_FAILURE);
            }

            bytes_sent += curr_bytes_sent;
        }
    }
    close(client_sock);
}

void uds_client_dgram(char *ip_addr, int port, bool quiet_mode)
{
    int client_sock; // Client socket
    client_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client_sock == -1)
    {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCK_PATH);
    
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    char buf[1024];
    generate_100_mega(quiet_mode);
    // Open file to send
    FILE *file = fopen(FILE_PATH, "r");
    if (file == NULL)
    {
        perror("can not open the file.txt");
        exit(1);
    }

    // Send file to server
    while (fgets(buf, sizeof(buf), file))
    {
        int curr_bytes_sent = send(client_sock, buf, strlen(buf), 0);
        if (curr_bytes_sent <= 0)
        {
            perror("send");
            exit(1);
        }
    }

    // Close file and socket
    fclose(file);
    close(client_sock);
}
void uds_client_stream(char *ip_addr, int port, bool quiet_mode)
{
  struct sockaddr_un server_addr;
    int server_fd, rc;
    char buf[BUFFER_SIZE];
    FILE *fp;
    // Create a socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);
    rc = connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rc < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }
    generate_100_mega(quiet_mode);
    sleep(1);
    // open file to send
    fp = fopen(FILE_PATH, "rb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }    
    
    while (fgets(buf, sizeof(buf), fp))
    {
            int curr_bytes_sent = send(server_fd, buf, strlen(buf), 0);
            if (curr_bytes_sent <= 0)
            {
                perror("send");
                exit(1);
            }
    }
    // close the file
    fclose(fp);

    // close the connection
    close(server_fd);
}

void mmap_client_filename(char *file_name, bool quiet_mode) {
	uint8_t *data = MAP_FAILED;
	int sock = 0;
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

	FILE *file = NULL;
	unlink(file_name);

	if ((file = fopen(file_name, "w+")) == NULL) {
		if (!quiet_mode) perror("fopen");
		exit(EXIT_FAILURE);
	}

	sock = fileno(file);

	ftruncate(sock, sizeof(uint32_t) + DATA_SIZE);

	if ((data = mmap(NULL, sizeof(uint32_t) + DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sock, 0)) == MAP_FAILED) {
		if (!quiet_mode) perror("mmap");

		fclose(file);
		unlink(file_name);

		exit(EXIT_FAILURE);
	}
    uint8_t *data_to_send = generate_100MB(quiet_mode);
	uint8_t *data_side = data + sizeof(uint32_t);
	uint32_t *bytes_w = (uint32_t *)data;
	*bytes_w = 0;

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = sock;
	pfds[1].events = POLLOUT;
	
    while (bytes_sent < DATA_SIZE) {
		int poll_count = poll(pfds, 2, -1);

		if (poll_count < 0) {
			if (!quiet_mode)
				perror("poll");
			munmap(data, sizeof(uint32_t) + DATA_SIZE);
			fclose(file);
			exit(EXIT_FAILURE);
		}

		if (pfds[1].revents & POLLOUT) {
			int bytes_left = (((DATA_SIZE - bytes_sent) > BUFFER_SIZE) ? BUFFER_SIZE :(DATA_SIZE - bytes_sent));

			memcpy(data_side, data_to_send, bytes_left);

			data_to_send += bytes_left;
			data_side += bytes_left;

			bytes_sent += bytes_left;
			*bytes_w = bytes_sent;
		}
	}

	if (munmap(data, sizeof(uint32_t) + DATA_SIZE) == -1) {
		if (!quiet_mode)
			perror("munmap");

		fclose(file);
		exit(EXIT_FAILURE);
	}

	fclose(file);
}

void pipe_client_filename(char *file_name, bool quiet_mode) {
	int sock = 0;
    int bytes_sent = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

	if (mkfifo(file_name, 0644) == -1) {
		if (!quiet_mode) perror("mknod");
		exit(EXIT_FAILURE);
	}

	if ((sock = open(file_name, O_WRONLY)) == -1) {
		if (!quiet_mode)
			perror("open");
		exit(EXIT_FAILURE);
	}

    uint8_t *data_to_send = generate_100MB(quiet_mode);

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = sock;
	pfds[1].events = POLLOUT;

	while (bytes_sent < DATA_SIZE) {
		int poll_count = poll(pfds, 2, -1);

		if (poll_count < 0) {
			if (!quiet_mode)
				perror("poll");
			close(sock);
			exit(EXIT_FAILURE);
		}

		if (pfds[1].revents & POLLOUT) {
			int bytes_left = (((DATA_SIZE - bytes_sent) > BUFFER_SIZE) ? BUFFER_SIZE:(DATA_SIZE - bytes_sent));

			write(sock, data_to_send + bytes_sent, bytes_left);
			bytes_sent += bytes_left;
		}
	}

	close(sock);
}

