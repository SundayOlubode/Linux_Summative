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
#define MAX_PASSWORD 32
#define CREDENTIALS_FILE "users.txt"

typedef struct
{
        char username[MAX_USERNAME];
        char password[MAX_PASSWORD];
} User;

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

// Function to save user credentials
void save_user(const char *username, const char *password)
{
        FILE *file = fopen(CREDENTIALS_FILE, "a");
        if (file)
        {
                fprintf(file, "%s:%s\n", username, password);
                fclose(file);
        }
}

// Function to verify credentials
bool verify_credentials(const char *username, const char *password)
{
        FILE *file = fopen(CREDENTIALS_FILE, "r");
        if (!file)
                return false;

        char line[BUFFER_SIZE];
        char stored_user[MAX_USERNAME];
        char stored_pass[MAX_PASSWORD];

        while (fgets(line, sizeof(line), file))
        {
                sscanf(line, "%[^:]:%s", stored_user, stored_pass);
                if (strcmp(username, stored_user) == 0 &&
                    strcmp(password, stored_pass) == 0)
                {
                        fclose(file);
                        return true;
                }
        }
        fclose(file);
        return false;
}

// Function to check if username exists
bool username_exists(const char *username)
{
        FILE *file = fopen(CREDENTIALS_FILE, "r");
        if (!file)
                return false;

        char line[BUFFER_SIZE];
        char stored_user[MAX_USERNAME];

        while (fgets(line, sizeof(line), file))
        {
                sscanf(line, "%[^:]:", stored_user);
                if (strcmp(username, stored_user) == 0)
                {
                        fclose(file);
                        return true;
                }
        }
        fclose(file);
        return false;
}

// Function to broadcast online users
void broadcast_online_users(int client_socket)
{
        char buffer[BUFFER_SIZE] = "Online users:\n";

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

// Function to handle authentication
bool handle_authentication(int client_socket, char *username)
{
        char buffer[BUFFER_SIZE];
        char password[MAX_PASSWORD];

        // Send initial menu
        const char *menu = "1. Register\n2. Login\nChoice: ";
        send(client_socket, menu, strlen(menu), 0);

        // Get choice
        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
                return false;
        buffer[bytes] = '\0';
        char choice = buffer[0];

        // Send username prompt
        send(client_socket, "Enter username: ", 15, 0);

        // Get username
        bytes = recv(client_socket, username, MAX_USERNAME - 1, 0);
        if (bytes <= 0)
                return false;
        username[bytes - 1] = '\0'; // Remove newline

        if (choice == '1')
        { // Register
                if (username_exists(username))
                {
                        send(client_socket, "Username already exists\n", 22, 0);
                        return false;
                }

                // Send password prompt
                send(client_socket, "Enter password: ", 15, 0);

                // Get password
                bytes = recv(client_socket, password, MAX_PASSWORD - 1, 0);
                if (bytes <= 0)
                        return false;
                password[bytes - 1] = '\0'; // Remove newline

                save_user(username, password);
                return true;
        }
        else if (choice == '2')
        { // Login
                // Send password prompt
                send(client_socket, "Enter password: ", 15, 0);

                // Get password
                bytes = recv(client_socket, password, MAX_PASSWORD - 1, 0);
                if (bytes <= 0)
                        return false;
                password[bytes - 1] = '\0'; // Remove newline

                return verify_credentials(username, password);
        }

        return false;
}

// Function to handle client connection
void *handle_client(void *arg)
{
        int client_socket = *(int *)arg;
        free(arg);
        char username[MAX_USERNAME];
        Client *current_client = NULL;
        char buffer[BUFFER_SIZE];

        // Handle authentication
        if (!handle_authentication(client_socket, username))
        {
                send(client_socket, "Authentication failed!\n", 21, 0);
                close(client_socket);
                return NULL;
        }

        // Add client to active clients list
        pthread_mutex_lock(&client_list.mutex);
        if (client_list.count < MAX_CLIENTS)
        {
                current_client = &client_list.clients[client_list.count];
                current_client->socket = client_socket;
                strncpy(current_client->username, username, MAX_USERNAME - 1);
                current_client->authenticated = true;
                client_list.count++;

                // Send welcome message
                char welcome[BUFFER_SIZE];
                sprintf(welcome, "Welcome %s! You are now connected.\n", username);
                send(client_socket, welcome, strlen(welcome), 0);

                // Broadcast to all clients that a new user joined
                char join_msg[BUFFER_SIZE];
                sprintf(join_msg, "User %s has joined the chat.\n", username);
                for (int i = 0; i < client_list.count; i++)
                {
                        if (client_list.clients[i].authenticated &&
                            client_list.clients[i].socket != client_socket)
                        {
                                send(client_list.clients[i].socket, join_msg, strlen(join_msg), 0);
                        }
                }
        }
        pthread_mutex_unlock(&client_list.mutex);

        if (!current_client)
        {
                send(client_socket, "Server full!\n", 13, 0);
                close(client_socket);
                return NULL;
        }

        // Show initial online users list
        broadcast_online_users(client_socket);

        // Main message handling loop
        while (1)
        {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0)
                {
                        // Client disconnected
                        break;
                }

                buffer[bytes_received] = '\0';

                // Handle list command
                if (strncmp(buffer, "LIST:USERS", 10) == 0)
                {
                        broadcast_online_users(client_socket);
                        continue;
                }

                // Handle regular messages
                char *recipient = strtok(buffer, ":");
                char *message = strtok(NULL, "\n");

                if (recipient && message)
                {
                        pthread_mutex_lock(&client_list.mutex);
                        bool message_sent = false;

                        for (int i = 0; i < client_list.count; i++)
                        {
                                if (client_list.clients[i].authenticated &&
                                    strcmp(client_list.clients[i].username, recipient) == 0)
                                {
                                        // Format and send the message
                                        char formatted_msg[BUFFER_SIZE];
                                        sprintf(formatted_msg, "\n\nFrom %s: %s\n\n", username, message);
                                        send(client_list.clients[i].socket, formatted_msg, strlen(formatted_msg), 0);

                                        // Send confirmation to sender
                                        char confirm_msg[BUFFER_SIZE];
                                        sprintf(confirm_msg, "Message sent to %s\n", recipient);
                                        send(client_socket, confirm_msg, strlen(confirm_msg), 0);

                                        message_sent = true;
                                        break;
                                }
                        }

                        if (!message_sent)
                        {
                                char error_msg[BUFFER_SIZE];
                                sprintf(error_msg, "User %s is not online.\n", recipient);
                                send(client_socket, error_msg, strlen(error_msg), 0);
                        }

                        pthread_mutex_unlock(&client_list.mutex);
                }
                else
                {
                        // Invalid message format
                        send(client_socket, "Invalid message format. Use 'username:message'\n", 44, 0);
                }
        }

        // Handle client disconnection
        pthread_mutex_lock(&client_list.mutex);
        current_client->authenticated = false;

        // Notify other clients about the disconnection
        char disconnect_msg[BUFFER_SIZE];
        sprintf(disconnect_msg, "User %s has left the chat.\n", username);

        for (int i = 0; i < client_list.count; i++)
        {
                if (client_list.clients[i].authenticated &&
                    client_list.clients[i].socket != client_socket)
                {
                        send(client_list.clients[i].socket, disconnect_msg, strlen(disconnect_msg), 0);
                }
        }

        // Clean up client list if this was the last client
        if (current_client == &client_list.clients[client_list.count - 1])
        {
                client_list.count--;
        }
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

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
                        free(client_socket);
                        continue;
                }

                pthread_t thread;
                if (pthread_create(&thread, NULL, handle_client, client_socket) != 0)
                {
                        free(client_socket);
                        close(*client_socket);
                        continue;
                }
                pthread_detach(thread);
        }

        return 0;
}