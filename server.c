#include "stnc.h"

struct timeval start, end;

int server_main(int port, bool performance_mode, bool quiet_mode) {
    int server_sock, chat_server;
    struct sockaddr_in server_addr, client_addr;
    MessageType msg;
    int bytes_received = 0;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Succesfuly binded to address.\n");

    if (listen(server_sock, 10) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to listen.\n");
        exit(EXIT_FAILURE);
    }
    if (!quiet_mode)
        printf("Server listening...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    while (true) {
        chat_server = accept(server_sock, (struct sockaddr *)&client_addr, &client_addrLen);
        if (chat_server == -1) {
            if (!quiet_mode)
                perror("accept");
        }

        if (!quiet_mode)
            printf("Client has been connected\n");

        recv(chat_server, &msg, sizeof(msg), 0);

        msg.quiet_mode = quiet_mode;

        ssize_t num_written = send(chat_server, &msg, sizeof(msg), 0);
        if (num_written < 0) {
            if (!quiet_mode)
                perror("send");
            exit(1);
        }

        uint8_t *data = (uint8_t *)malloc(DATA_SIZE * sizeof(uint8_t));

        switch (msg.transport) {
               case PARAM_TCP:
            if (msg.protocol == TYPE_IP4) {
                if (performance_mode)
                    bytes_received = tcp_server_ipv4(port + 1, quiet_mode, data);
                else
                    tcp_server_chat_ipv4(port + 1, quiet_mode);
            }
            else if (msg.protocol == TYPE_IP6)
                bytes_received = tcp_server_ipv6(port + 1, quiet_mode, data);
            break;
        case PARAM_UDP:
            if (msg.protocol == TYPE_IP4)
                bytes_received = udp_server_ipv4(port + 1, quiet_mode, data);
            else if (msg.protocol == TYPE_IP6)
                bytes_received = udp_server_ipv6(port + 1, quiet_mode, data);
            break;
        case PARAM_DGRAM:
            if (msg.protocol == TYPE_UDS)
                bytes_received = uds_server_dgram(port + 1, quiet_mode, data);
            break;
        case PARAM_STREAM:
            if (msg.protocol == TYPE_UDS)
                bytes_received = uds_server_stream(port + 1, quiet_mode, data);
            break;
        case PARAM_FILENAME:
            if (msg.protocol == TYPE_MMAP)
                bytes_received = mmap_server_filename(quiet_mode, data, msg.file_name);
            else if (msg.protocol == TYPE_PIPE)
                bytes_received = pipe_server_filename(quiet_mode, data, msg.file_name);
            break;
        }

        if (!quiet_mode){ 
            printf("Receiver %d of data.\n", bytes_received);
        }
        free(data);
    }
    close(server_sock);
    close(chat_server);
    return 0;
}

void tcp_server_chat_ipv4(int port, bool quiet_mode) {
    int listener_server, client_sock; // Server and client sockets
    struct sockaddr_in server_addr, client_addr;
    int fd_count;
    char buffer[BUFFER_SIZE];
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    listener_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener_server == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listener_server, 10) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to listen.\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Server...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    client_sock = accept(listener_server, (struct sockaddr *)&client_addr, &client_addrLen);
    if (client_sock == -1) {
        if (!quiet_mode)
            perror("accept");
    }

    if (!quiet_mode)
        printf("Client has been connected\n");
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    fd_count = 2;

    while (1) {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[0].revents & POLLIN) {
            ssize_t num_bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (num_bytes_read < 0) {
                if (!quiet_mode)
                    perror("read");
                continue;
            }
            if (num_bytes_read == 0) {
                break;
            }
            buffer[num_bytes_read] = '\0';
            ssize_t num_written = send(client_sock, buffer, strlen(buffer), 0);
            if (num_written < 0) {
                if (!quiet_mode)
                    perror("send");
            }
            bzero(buffer, BUFFER_SIZE);
        }
        if (pfds[1].revents & POLLIN) {
            ssize_t num_bytes_recv = recv(client_sock, buffer, BUFFER_SIZE, 0);
            if (num_bytes_recv < 0) {
                if (!quiet_mode)
                    perror("recv");
            }

            if (num_bytes_recv == 0) {
                if (!quiet_mode)
                    printf("Client disconnected.\n");
                exit(1);
            }
            else {
                buffer[num_bytes_recv] = '\0';
                if (!quiet_mode)
                    printf("Client:\t %s", buffer);
            }
        }
    }

    // Close all sockets
    close(listener_server);
    close(client_sock);
}

int tcp_server_ipv4(int port, bool quiet_mode, uint8_t *data) {
    int listener_server, client_sock; // Server and client sockets
    struct sockaddr_in server_addr, client_addr;
    int fd_count;
    int bytes_received = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    listener_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener_server == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listener_server, 10) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to listen.\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Server...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    client_sock = accept(listener_server, (struct sockaddr *)&client_addr, &client_addrLen);
    if (client_sock == -1) {
        if (!quiet_mode)
            perror("accept");
    }

    gettimeofday(&start, NULL);

    if (!quiet_mode)
        printf("Client has been connected\n");
    pfds[0].fd = listener_server;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    fd_count = 2;

    while (bytes_received < DATA_SIZE) {
        int poll_count = poll(pfds, fd_count, -1);
        
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[1].revents & POLLIN) {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recv(client_sock, data + bytes_received, bytes_left, 0);
            gettimeofday(&end, NULL);
            if (num_bytes_recv < 0) {
                if (!quiet_mode)
                    perror("recv");
                close(client_sock);
            }

            bytes_received += (int)num_bytes_recv;
        }
    }

    if (!quiet_mode)
        printf("Receiver %d of data.\n", bytes_received);

    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("ipv4_tcp,%ld\n", time);

    // Close all sockets
    close(listener_server);
    close(client_sock);

    return bytes_received;
}

int tcp_server_ipv6(int port, bool quiet_mode, uint8_t *data) {
    int listener_server, client_sock; // Server and client sockets
    struct sockaddr_in6 server_addr, client_addr;
    int fd_count;
    int bytes_received = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    // Connect to server running on localhost
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    listener_server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (listener_server == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listener_server, 10) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to listen.\n");
        exit(EXIT_FAILURE);
    }
    if (!quiet_mode)
        printf("Server...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    client_sock = accept(listener_server, (struct sockaddr *)&client_addr, &client_addrLen);
    if (client_sock == -1) {
        if (!quiet_mode)
            perror("accept");
    }

    gettimeofday(&start, NULL);

    if (!quiet_mode)
        printf("Client has been connected\n");
    pfds[0].fd = listener_server;
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    fd_count = 2;

    while (bytes_received < DATA_SIZE) {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[1].revents & POLLIN) {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recv(client_sock, data + bytes_received, bytes_left, 0);
            gettimeofday(&end, NULL);

            if (num_bytes_recv < 0) {
                if (!quiet_mode)
                    perror("recv");
                close(client_sock);
            }

            bytes_received += (int)num_bytes_recv;
        }
    }

    if (!quiet_mode)
        printf("Receiver %d of data.\n", bytes_received);

    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("ipv6_tcp,%ld\n", time);

    // Close all sockets
    close(listener_server);
    close(client_sock);
    return bytes_received;
}

int udp_server_ipv4(int port, bool quiet_mode, uint8_t *data) {
    int listener_server; // Server socket
    struct sockaddr_in server_addr, client_addr;
    int fd_count;
    int bytes_received = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    listener_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listener_server == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }
    if (!quiet_mode)
        printf("Server...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = listener_server;
    pfds[1].events = POLLIN;
    fd_count = 2;
    gettimeofday(&start, NULL);
    while (bytes_received < DATA_SIZE) {
        int poll_count = poll(pfds, fd_count, 2000);
        if (poll_count == 0) {
            break;
        }
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[1].revents & POLLIN) {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recvfrom(listener_server, data + bytes_received, bytes_left, 0, (struct sockaddr *)&client_addr, &client_addrLen);
            if (num_bytes_recv <= 0) {
                if (!quiet_mode)
                    perror("recvfrom");
                close(listener_server);
                exit(EXIT_FAILURE);
            }
            gettimeofday(&end, NULL);
            bytes_received += (int)num_bytes_recv;
        }
    }
    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("ipv4_udp,%ld\n", time);

    // Close socket
    close(listener_server);
    return bytes_received;
}

int udp_server_ipv6(int port, bool quiet_mode, uint8_t *data) {
    int listener_server; // Server socket
    struct sockaddr_in6 server_addr, client_addr;
    int fd_count;
    int bytes_received = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

    // Connect to server running on localhost
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    listener_server = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (listener_server == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to bind to port.\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Server...\n");
    gettimeofday(&start, NULL);

    socklen_t client_addrLen = sizeof(client_addr);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = listener_server;
    pfds[1].events = POLLIN;
    fd_count = 2;

    while (bytes_received < DATA_SIZE) {
        int poll_count = poll(pfds, fd_count, 2000);
        if (poll_count == 0) {
            break;
        }
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }

        if (pfds[1].revents & POLLIN) {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recvfrom(listener_server, data + bytes_received, bytes_left, 0, (struct sockaddr *)&client_addr, &client_addrLen);
            gettimeofday(&end, NULL);
            if (num_bytes_recv <= 0) {
                if (!quiet_mode)
                    perror("recvfrom");
                close(listener_server);
                exit(EXIT_FAILURE);
            }

            bytes_received += (int)num_bytes_recv;
        }
    }

    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("ipv6_udp,%ld\n", time);

    // Close socket
    close(listener_server);
    return bytes_received;
}

int uds_server_dgram(int port, bool quiet_mode, uint8_t *data) {
    int listener_server; // Server socket
    struct sockaddr_un server_addr, client_addr;
    int fd_count;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);
    // Create and bind server socket
    listener_server = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (listener_server == -1)
    {
        fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    // Set up server socket address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCK_PATH);
    unlink(server_addr.sun_path);

    int enableReuse = 1;
    if(setsockopt(listener_server, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) {
        if (!quiet_mode)
            perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        fprintf(stderr, "failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Server...\n");

    socklen_t client_addrLen = sizeof(client_addr);
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = listener_server;
    pfds[1].events = POLLIN;
    fd_count = 2;
    int bytes_received = 0;
    gettimeofday(&start, NULL);
    while (1)
    {
        int poll_count = poll(pfds, fd_count, 2000);
        if (poll_count == 0)
        {
            break;
        }
        if (poll_count == -1)
        {
            fprintf(stderr, "Poll error.\n");
            exit(EXIT_FAILURE);
        }
        if (pfds[1].revents & POLLIN)
        {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recvfrom(listener_server, data + bytes_received, bytes_left, 0, (struct sockaddr *)&client_addr, &client_addrLen);

            if (num_bytes_recv <= 0)
            {
                if (!quiet_mode)
                    printf("Client disconnected.\n");
                close(listener_server);
                exit(1);
            }
            gettimeofday(&end, NULL);
            bytes_received += (int)num_bytes_recv;
        }
    }
    
    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("uds_dgram,%ld\n", time);
    
    // Close socket
    close(listener_server);
    return bytes_received;
}

int uds_server_stream(int port, bool quiet_mode, uint8_t *data) {
   int listener_server, client_sock; // Server and client sockets
    struct sockaddr_un server_addr;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);
    // Create and bind server socket
    listener_server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener_server == -1)
    {
        fprintf(stderr, "Couldn't create server socket.\n");
        exit(EXIT_FAILURE);
    }

    // Set up server socket address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);
    unlink(SOCK_PATH);


    if (bind(listener_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)   
     {
        fprintf(stderr, "failed to bind\n");
        exit(EXIT_FAILURE);
    }
    if (listen(listener_server, 10) == -1)
    {
        if (!quiet_mode)
            fprintf(stderr, "Server socket failed to listen.\n");
        exit(EXIT_FAILURE);
    }

    if (!quiet_mode)
        printf("Server...\n");

    client_sock = accept(listener_server, NULL, NULL);
    if (client_sock == -1)
    {
        if (!quiet_mode)
            perror("accept");
    }

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = listener_server;
    pfds[1].events = POLLIN;
    int bytes_received = 0;
    if (!quiet_mode)
        printf("Client has been connected\n");
    gettimeofday(&start, NULL);
    while (bytes_received < DATA_SIZE)
    {
            int bytes_left = (DATA_SIZE - bytes_received) > BUFFER_SIZE ? BUFFER_SIZE : (DATA_SIZE - bytes_received);
            ssize_t num_bytes_recv = recv(client_sock, data + bytes_received, bytes_left, 0);
            gettimeofday(&end, NULL);
            if (num_bytes_recv < 0)
            {
                if (!quiet_mode)
                    perror("recv");
                close(client_sock);
            }

            bytes_received += (int)num_bytes_recv;
    }
    delete_file(quiet_mode);
    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("uds_stream,%ld\n", time);
    // Close socket
    close(listener_server);
    return 1;
}

int mmap_server_filename(bool quiet_mode, uint8_t* data, char *file_name) {
	uint8_t *data_left = MAP_FAILED;
	int sock = 0;
    int bytes_received = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

	FILE* file = NULL;
	if ((file = fopen(file_name, "r")) == NULL) {
		if (!quiet_mode)
			fprintf(stderr, "Failed to open file \"%s\"\n", file_name);
		exit(EXIT_FAILURE);
	}

	sock = fileno(file);

	if ((data_left = mmap(NULL, sizeof(uint32_t) + DATA_SIZE, PROT_READ, MAP_SHARED, sock, 0)) == MAP_FAILED) {
		if (!quiet_mode)
			perror("mmap");

		fclose(file);
		exit(EXIT_FAILURE);
	}

	uint8_t *temp_data_left = data_left + sizeof(uint32_t);
	uint32_t *memory_bytes = (uint32_t *)data_left;

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = sock;
	pfds[1].events = POLLIN;

	gettimeofday(&start, NULL);

	while (bytes_received < DATA_SIZE) {
		int poll_count = poll(pfds, 2, -1);

        if (poll_count == 0) {
            fclose(file);
            break;
        }
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

		if (pfds[1].revents & POLLIN) {
			int bytes_left = ((*memory_bytes - bytes_received) > DATA_SIZE) ? DATA_SIZE:(*memory_bytes - bytes_received);

			memcpy(data, temp_data_left, bytes_left);

			gettimeofday(&end, NULL);

			data += bytes_left;
			temp_data_left += bytes_left;
			bytes_received += bytes_left;
		}
	}

	if (munmap(data_left, sizeof(uint32_t) + DATA_SIZE) == -1) {
		if (!quiet_mode)
			perror("munmap");
		fclose(file);
		exit(EXIT_FAILURE);
	}

    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("mmap,%ld\n", time);

	fclose(file);

	unlink(file_name);
	return bytes_received;
}

int pipe_server_filename(bool quiet_mode, uint8_t* data, char *file_name) {
	int bytes_received = 0;
    int sock = 0;
    struct pollfd *pfds = malloc(sizeof(*pfds) * MAX_EVENTS);

	sleep(1);

	if ((sock = open(file_name, O_RDONLY)) == -1) {
		if (!quiet_mode)
			perror("open");
		exit(EXIT_FAILURE);
	}

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[1].fd = sock;
	pfds[1].events = POLLIN;

	gettimeofday(&start, NULL);
	while (bytes_received < DATA_SIZE) {
		int poll_count = poll(pfds, 2, -1);

        if (poll_count == 0) {
            close(sock);
            break;
        }
        if (poll_count == -1) {
            if (!quiet_mode)
                fprintf(stderr, "Poll error.\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

		if (pfds[1].revents & POLLIN) {
			int bytes_left = ((DATA_SIZE - bytes_received) > DATA_SIZE) ? DATA_SIZE:(DATA_SIZE - bytes_received);

			if (read(sock, data + bytes_received, bytes_left) == -1) {
				if (!quiet_mode)
					perror("write");

				close(sock);
				exit(EXIT_FAILURE);
			}

			gettimeofday(&end, NULL);

			bytes_received += bytes_left;
		}
	}

    long time = (end.tv_usec - start.tv_usec) * 1000;
    printf("pipe,%ld\n", time);

	close(sock);
	unlink(file_name);
	return bytes_received;
}