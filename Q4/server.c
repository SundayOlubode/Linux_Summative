#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define LOG_FILE "network_devices.log"

void log_entry(const char *entry)
{
        FILE *log_file = fopen(LOG_FILE, "a");
        if (log_file == NULL)
        {
                perror("Error opening log file");
                return;
        }

        // Get current timestamp
        time_t now = time(NULL);
        char timestamp[26];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // Write entry with timestamp
        fprintf(log_file, "[%s] %s\n", timestamp, entry);
        fclose(log_file);
}

int main()
{
        int server_fd, client_socket;
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);
        char buffer[BUFFER_SIZE] = {0};

        // Create socket
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
        }

        // Set socket options
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
                perror("setsockopt failed");
                exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        // Bind socket
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
                perror("Bind failed");
                exit(EXIT_FAILURE);
        }

        // Listen for connections
        if (listen(server_fd, 3) < 0)
        {
                perror("Listen failed");
                exit(EXIT_FAILURE);
        }

        printf("IP Logger Server started on port %d\n", PORT);
        printf("Logging to file: %s\n", LOG_FILE);

        while (1)
        {
                if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
                {
                        perror("Accept failed");
                        continue;
                }

                printf("New client connected\n");

                while (1)
                {
                        int valread = read(client_socket, buffer, BUFFER_SIZE);
                        if (valread <= 0)
                                break;

                        buffer[valread] = '\0';
                        printf("Received: %s", buffer);
                        log_entry(buffer);
                }

                close(client_socket);
                printf("Client disconnected\n");
        }

        return 0;
}