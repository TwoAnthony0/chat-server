#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10

int client_sockets[MAX_CLIENTS];
char usernames[MAX_CLIENTS][50];
pthread_t threads[MAX_CLIENTS];

void *handle_client(void *arg) {
    int index = *(int *)arg;
    char buffer[1024];

    // Receive username
    recv(client_sockets[index], usernames[index], sizeof(usernames[index]), 0);
    printf("User '%s' connected.\n", usernames[index]);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_sockets[index], buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        printf("%s: %s", usernames[index], buffer);

        // Broadcast to other clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != index && client_sockets[i] != 0) {
                char message[1074];
                snprintf(message, sizeof(message), "%s: %s", usernames[index], buffer);
                send(client_sockets[i], message, strlen(message), 0);
            }
        }
    }

    close(client_sockets[index]);
    client_sockets[index] = 0;
    printf("User '%s' disconnected.\n", usernames[index]);
    free(arg);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, &addrlen);
                int *index = malloc(sizeof(int));
                *index = i;
                pthread_create(&threads[i], NULL, handle_client, index);
                break;
            }
        }
    }

    close(server_fd);
    return 0;
}
