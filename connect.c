#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LISTEN_BACKLOG (5)
#define READ_ERROR (-1)
#define WRITE_ERROR (-2)

int serve_client(int client);

void serve(const char* filename) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un server_address;

    if (sockfd < 0) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, filename, strlen(filename));
    if (bind(sockfd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, LISTEN_BACKLOG) < 0) {
        fprintf(stderr, "Unable to listen specified filename.\n");
        exit(EXIT_FAILURE);
    }
    for (;;) {
        // struct sockaddr_un client_address; // TODO: read accept man page
        const int client = accept(sockfd, NULL, NULL);

        if (client < 0) {
            fprintf(stderr, "Connection failed.\n");
            continue;
        }
        int return_code = serve_client(client);
        if (return_code == READ_ERROR) {
            fprintf(stderr, "Client read error.\n");
        }
        else if (return_code == WRITE_ERROR) {
            fprintf(stderr, "Client write error.\n");
        }
        else if (return_code < 0) {
            fprintf(stderr, "Bogus data from client\n");
        }
        close(client);
    }
}
