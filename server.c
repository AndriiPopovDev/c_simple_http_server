#include <netinet/in.h> // IP-address constant (struct sockaddr_in)
#include <stdio.h> // input/output (printf)
#include <stdlib.h> // general function lib (exit, malloc)
#include <string.h>  // for strings (strlen, strcpy)
#include <sys/socket.h> // general function for sockets (socket, bind, listen, accept)
#include <unistd.h> // sys calls (read, write, close)
#include <fcntl.h> // file control (open)
#include <sys/sendfile.h> // sendfile

#define PORT 8080
#define STATIC_DIR "static/"

int main(int argc, char const* argv[]) {
    int server_fd, new_socket;
    size_t valread;
    struct sockaddr_in address;  // server's structure
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};

    char* hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 17\n\nHello from server";

    // Creating socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET - IPv4, SOCK_STREAM - secure TCP socket type, 0 - default protocol
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Keep server running to handle multiple connections
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("accept");
            continue; // Continue to next iteration instead of exiting
        }

        // Read request
        valread = read(new_socket, buffer, 1024 - 1);
        if (valread <= 0) {
            close(new_socket);
            continue; // Skip to next connection if read fails
        }
        printf("%s\n", buffer);

        // Handle file serving
        char* f = buffer + 5; // Assumes "GET /" prefix
        *strchr(f, ' ') = 0;  // Null-terminate at first space

        // Check if root path ("/" or empty)
        if (strcmp(f, "") == 0 || strcmp(f, "/") == 0) {
            send(new_socket, hello, strlen(hello), 0);
            printf("Hello message sent for root path\n");
        } else {
            // Construct file path with static directory
            char filepath[1024] = STATIC_DIR;
            strncat(filepath, f, sizeof(filepath) - strlen(STATIC_DIR) - 1);

            // Check if the requested file is test.img
            int open_fd = -1;
            if (strcmp(f, "test.img") == 0) {
                open_fd = open(filepath, O_RDONLY);
                if (open_fd < 0) {
                    perror("Failed to open test.img");
                    // Fallback to hello message
                    send(new_socket, hello, strlen(hello), 0);
                    printf("Hello message sent\n");
                } else {
                    printf("Serving test.img\n");
                    sendfile(new_socket, open_fd, 0, 256); // Send up to 256 bytes
                    close(open_fd);
                }
            } else {
                open_fd = open(filepath, O_RDONLY);
                if (open_fd >= 0) {
                    printf("Serving file: %s\n", filepath);
                    sendfile(new_socket, open_fd, 0, 256); // Send up to 256 bytes
                    close(open_fd);
                } else {
                    perror("Failed to open file");
                    // Fallback to hello message
                    send(new_socket, hello, strlen(hello), 0);
                    printf("Hello message sent\n");
                }
            }
        }

        // Clear buffer for next request
        memset(buffer, 0, sizeof(buffer));

        // Closing the connected socket
        close(new_socket);
    }

    // Closing the listening socket (unreachable in this version, but kept for completeness)
    close(server_fd);
    return 0;
}