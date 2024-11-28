#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_USERNAME 32

typedef struct
{
        int socket;
        char username[MAX_USERNAME];
        bool authenticated;
} Client;

typedef struct
{
        Client clients[MAX_CLIENTS];
        int count;
        pthread_mutex_t mutex;
} ClientList;

ClientList client_list = {
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER};

// Function to broadcast online users to a specific client
void broadcast_online_users(int client_socket)
{
        char buffer[BUFFER_SIZE];
        strcpy(buffer, "Online users:\n");

        pthread_mutex_lock(&client_list.mutex);
        for (int i = 0; i < client_list.count; i++)
        {
                if (client_list.clients[i].authenticated)
                {
                        strcat(buffer, client_list.clients[i].username);
                        strcat(buffer, "\n");
                }
        }
        pthread_mutex_unlock(&client_list.mutex);

        send(client_socket, buffer, strlen(buffer), 0);
}

// Function to find client index by username
int find_client_by_username(const char *username)
{
        for (int i = 0; i < client_list.count; i++)
        {
                if (strcmp(client_list.clients[i].username, username) == 0)
                {
                        return i;
                }
        }
        return -1;
}

// Function to handle each client connection
void *handle_client(void *arg)
{
        int client_socket = *(int *)arg;
        free(arg);
        char buffer[BUFFER_SIZE];
        Client *current_client = NULL;

        // Authentication
        send(client_socket, "Enter username: ", 15, 0);
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
                return NULL;
        buffer[bytes_received - 1] = '\0'; // Remove newline

        pthread_mutex_lock(&client_list.mutex);
        if (client_list.count < MAX_CLIENTS)
        {
                current_client = &client_list.clients[client_list.count];
                current_client->socket = client_socket;
                strncpy(current_client->username, buffer, MAX_USERNAME - 1);
                current_client->authenticated = true;
                client_list.count++;
        }
        pthread_mutex_unlock(&client_list.mutex);

        if (!current_client)
        {
                send(client_socket, "Server full!\n", 13, 0);
                close(client_socket);
                return NULL;
        }

        // Send welcome message and online users
        char welcome[BUFFER_SIZE];
        sprintf(welcome, "Welcome %s! You are now connected.\n", current_client->username);
        send(client_socket, welcome, strlen(welcome), 0);
        broadcast_online_users(client_socket);

        // Main message loop
        while (1)
        {
                bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0)
                        break;
                buffer[bytes_received] = '\0';

                // Parse message format: "USERNAME:MESSAGE"
                char *recipient = strtok(buffer, ":");
                char *message = strtok(NULL, "\n");
                if (!recipient || !message)
                        continue;

                pthread_mutex_lock(&client_list.mutex);
                int recipient_idx = find_client_by_username(recipient);
                if (recipient_idx != -1 && client_list.clients[recipient_idx].authenticated)
                {
                        char formatted_msg[BUFFER_SIZE];
                        sprintf(formatted_msg, "From %s: %s\n", current_client->username, message);
                        send(client_list.clients[recipient_idx].socket, formatted_msg, strlen(formatted_msg), 0);
                }
                pthread_mutex_unlock(&client_list.mutex);
        }

        // Cleanup on disconnect
        pthread_mutex_lock(&client_list.mutex);
        current_client->authenticated = false;
        pthread_mutex_unlock(&client_list.mutex);
        close(client_socket);

        return NULL;
}

int main()
{
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1)
        {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(8888),
            .sin_addr.s_addr = INADDR_ANY};

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
                perror("Bind failed");
                exit(EXIT_FAILURE);
        }

        if (listen(server_socket, 5) == -1)
        {
                perror("Listen failed");
                exit(EXIT_FAILURE);
        }

        printf("Chat server started on port 8888\n");

        while (1)
        {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int *client_socket = malloc(sizeof(int));
                *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

                if (*client_socket == -1)
                {
                        perror("Accept failed");
                        free(client_socket);
                        continue;
                }

                pthread_t thread;
                if (pthread_create(&thread, NULL, handle_client, client_socket) != 0)
                {
                        perror("Thread creation failed");
                        free(client_socket);
                        close(*client_socket);
                        continue;
                }
                pthread_detach(thread);
        }

        return 0;
}