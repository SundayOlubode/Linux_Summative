#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define ARP_CACHE "/proc/net/arp"

char *get_local_ip(char *interface_name)
{
        int fd;
        struct ifreq ifr;
        static char ip[INET_ADDRSTRLEN];

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

        if (ioctl(fd, SIOCGIFADDR, &ifr) == -1)
        {
                close(fd);
                return NULL;
        }

        close(fd);
        strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
        return ip;
}

char *get_hostname(const char *ip)
{
        struct sockaddr_in sa;
        static char hostname[NI_MAXHOST];

        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &(sa.sin_addr));

        if (getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                        hostname, sizeof(hostname),
                        NULL, 0, NI_NAMEREQD))
        {
                return NULL;
        }

        return hostname;
}

void scan_network(int server_socket)
{
        FILE *arp_file = fopen(ARP_CACHE, "r");
        if (!arp_file)
        {
                perror("Cannot open ARP cache");
                return;
        }

        char line[BUFFER_SIZE];
        char ip[16], hw_type[8], flags[8], hw_addr[18], mask[8], device[8];

        fgets(line, sizeof(line), arp_file);

        // Read each entry
        while (fgets(line, sizeof(line), arp_file))
        {
                sscanf(line, "%s %s %s %s %s %s", ip, hw_type, flags, hw_addr, mask, device);

                if (strcmp(hw_addr, "00:00:00:00:00:00") != 0)
                {
                        char *hostname = get_hostname(ip);
                        char buffer[BUFFER_SIZE];

                        if (hostname)
                        {
                                snprintf(buffer, sizeof(buffer), "IP: %s, Hostname: %s, MAC: %s, Interface: %s\n",
                                         ip, hostname, hw_addr, device);
                        }
                        else
                        {
                                snprintf(buffer, sizeof(buffer), "IP: %s, Hostname: Unknown, MAC: %s, Interface: %s\n",
                                         ip, hw_addr, device);
                        }

                        send(server_socket, buffer, strlen(buffer), 0);
                        printf("Sent: %s", buffer);
                }
        }

        fclose(arp_file);
}

int main()
{
        int sock = 0;
        struct sockaddr_in serv_addr;

        // Create socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf("Socket creation error\n");
                return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        {
                printf("Invalid address/Address not supported\n");
                return -1;
        }

        // Connect to server
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
                printf("Connection Failed\n");
                return -1;
        }

        printf("Connected to server. Starting network scan...\n");

        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == -1)
        {
                perror("getifaddrs");
                return -1;
        }

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
                if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
                        continue;

                char *ip = get_local_ip(ifa->ifa_name);
                if (ip)
                {
                        char *hostname = get_hostname(ip);
                        char buffer[BUFFER_SIZE];

                        snprintf(buffer, sizeof(buffer), "Local Machine - IP: %s, Hostname: %s, Interface: %s\n",
                                 ip, hostname ? hostname : "Unknown", ifa->ifa_name);

                        send(sock, buffer, strlen(buffer), 0);
                        printf("Sent local info: %s", buffer);
                }
        }

        freeifaddrs(ifaddr);

        scan_network(sock);

        close(sock);
        printf("Scan completed and data sent to server.\n");
        return 0;
}