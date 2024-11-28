#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *arg)
{
        int socket = *(int *)arg;
        char buffer[BUFFER_SIZE];

        while (1)
        {
                int bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0)
                {
                        printf("Server disconnected\n");
                        exit(1);
                }
                buffer[bytes_received] = '\0';
                printf("%s", buffer);
        }
        return NULL;
}

int main()
{
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1)
        {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(8888),
            .sin_addr.s_addr = inet_addr("127.0.0.1")};

        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
                perror("Connection failed");
                exit(EXIT_FAILURE);
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, receive_messages, &client_socket) != 0)
        {
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        printf("Chat client started\n");

        while (1)
        {
                fgets(buffer, sizeof(buffer), stdin);
                if (send(client_socket, buffer, strlen(buffer), 0) == -1)
                {
                        perror("Send failed");
                        break;
                }
        }

        close(client_socket);
        return 0;
}