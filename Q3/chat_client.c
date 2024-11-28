#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define MAX_USERNAME 32
#define MAX_PASSWORD 32

volatile bool authentication_complete = false;

void *receive_messages(void *arg)
{
        int socket = *(int *)arg;
        char buffer[BUFFER_SIZE];

        while (1)
        {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0)
                {
                        printf("\nDisconnected from server\n");
                        exit(1);
                }
                buffer[bytes] = '\0';

                if (strstr(buffer, "Username already exists"))
                {
                        printf("%s\n", buffer);
                        exit(1);
                }

                printf("%s", buffer);
                if (authentication_complete)
                {
                        printf("Enter the format 'recipient:message' (or 'list' for online users): ");
                        fflush(stdout);
                }
        }
        return NULL;
}

bool handle_authentication(int socket)
{
        char buffer[BUFFER_SIZE];
        char username[MAX_USERNAME];
        char password[MAX_PASSWORD];
        char choice[3];

        // Get initial menu
        int bytes = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
                return false;
        buffer[bytes] = '\0';
        printf("%s", buffer);

        // Send choice
        fgets(choice, sizeof(choice), stdin);
        send(socket, choice, strlen(choice), 0);

        // Get username prompt
        bytes = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
                return false;
        buffer[bytes] = '\0';
        printf("%s", buffer);

        // Send username
        fgets(username, sizeof(username), stdin);
        send(socket, username, strlen(username), 0);

        // Check for username exists error
        bytes = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
                return false;
        buffer[bytes] = '\0';

        if (strstr(buffer, "exists"))
        {
                printf("%s", buffer);
                return false;
        }

        printf("%s", buffer); // Password prompt

        // Send password
        fgets(password, sizeof(password), stdin);
        send(socket, password, strlen(password), 0);

        // Get result
        bytes = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
                return false;
        buffer[bytes] = '\0';
        printf("%s", buffer);

        return strstr(buffer, "Welcome") != NULL;
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

        printf("Connected to server\n");

        if (!handle_authentication(client_socket))
        {
                printf("Authentication failed.\n");
                close(client_socket);
                exit(1);
        }

        authentication_complete = true;

        pthread_t thread;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;

        if (pthread_create(&thread, NULL, receive_messages, socket_ptr) != 0)
        {
                perror("Thread creation failed");
                free(socket_ptr);
                close(client_socket);
                exit(1);
        }

        char buffer[BUFFER_SIZE];
        while (1)
        {
                if (fgets(buffer, BUFFER_SIZE, stdin) != NULL)
                {
                        if (strncmp(buffer, "list", 4) == 0)
                        {
                                send(client_socket, "LIST:USERS\n", 11, 0);
                        }
                        else
                        {
                                if (send(client_socket, buffer, strlen(buffer), 0) == -1)
                                {
                                        perror("Send failed");
                                        break;
                                }
                        }
                }
        }

        close(client_socket);
        free(socket_ptr);
        return 0;
}