#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/sendfile.h>

// TODOS:
// - add request queue
// - add response queue
// - add thread pool

int main() {
    int socket_fd = socket(
        AF_INET,
        SOCK_STREAM, // tcp
        0 // default protocol
    );
    if (socket_fd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // ipv4
    addr.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces
    addr.sin_port = htons(9090); // port in hex, reverse byte order / network byte order / big endian
    // addr.sin_len is not required by the POSIX specification
    memset(addr.sin_zero, '\0', sizeof addr.sin_zero); // zero out the rest of the struct: https://stackoverflow.com/questions/15608707/why-is-zero-padding-needed-in-sockaddr-in
    socklen_t addr_length = sizeof(addr);
    struct sockaddr *addr_ptr = (struct sockaddr *) &addr;

    if (bind(socket_fd, (struct sockaddr *) addr_ptr, addr_length) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    int max_pending_connections = 10;
    if (listen(socket_fd, max_pending_connections) < 0) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        int client_socket_fd = accept(socket_fd,  NULL, NULL);
        if (client_socket_fd < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        int req_buffer_size = 256;
        char req_buffer[256] = {0};

        int size = recv(client_socket_fd, req_buffer, req_buffer_size, 0);

        // GET /file.html HTTP/1.1
        char* file_name = req_buffer + 5; // skip "GET /"
        // strchr(file_name, ' ')[0] = '\0'; // terminate the string at the first space
        *strchr(file_name, ' ') = '\0'; // terminate the string at the first space

        printf("Requested file: %s\n", req_buffer);

        int opened_fd = open(file_name, O_RDONLY);
        if (opened_fd < 0) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // if (sendfile(opened_fd, client_socket_fd, 0, (off_t *) &req_buffer_size, NULL, 0)) {
        //     perror("Error sending file");
        //     exit(EXIT_FAILURE);
        // }

        if (sendfile(client_socket_fd, opened_fd, 0, req_buffer_size) == -1) {
            perror("Error sending file");
            exit(EXIT_FAILURE);
        }


        //char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
        // char response_buffer[30000] = {0};
        // long valread = read( client_socket_fd , response_buffer, 30000);
        // printf("%s\n",response_buffer);
        // write(client_socket_fd , response , strlen(response));
        // printf("------------------Hello message sent-------------------");
        


        close(opened_fd);
        close(client_socket_fd);
    }

    return 0;
}
