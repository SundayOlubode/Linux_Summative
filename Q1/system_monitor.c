#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <ctype.h>

// Structure to store CPU statistics
typedef struct
{
        long user;
        long nice;
        long system;
        long idle;
        long iowait;
        long irq;
        long softirq;
        long steal;
        long guest;
} CPUStats;

// Function to read CPU statistics
void read_cpu_stats(CPUStats *stats)
{
        FILE *fp = fopen("/proc/stat", "r");
        if (fp == NULL)
        {
                perror("Error opening /proc/stat");
                exit(1);
        }

        char cpu[10];
        fscanf(fp, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld",
               cpu, &stats->user, &stats->nice, &stats->system,
               &stats->idle, &stats->iowait, &stats->irq,
               &stats->softirq, &stats->steal, &stats->guest);

        fclose(fp);
}

// Function to calculate CPU usage percentage
float calculate_cpu_usage(CPUStats *prev, CPUStats *curr)
{
        long prev_idle = prev->idle + prev->iowait;
        long curr_idle = curr->idle + curr->iowait;

        long prev_non_idle = prev->user + prev->nice + prev->system +
                             prev->irq + prev->softirq + prev->steal;
        long curr_non_idle = curr->user + curr->nice + curr->system +
                             curr->irq + curr->softirq + curr->steal;

        long prev_total = prev_idle + prev_non_idle;
        long curr_total = curr_idle + curr_non_idle;

        long total_diff = curr_total - prev_total;
        long idle_diff = curr_idle - prev_idle;

        float cpu_usage = ((float)(total_diff - idle_diff) / total_diff) * 100;
        return cpu_usage;
}

// Function to get memory usage percentage
float get_memory_usage()
{
        struct sysinfo si;
        if (sysinfo(&si) == -1)
        {
                perror("Error getting system info");
                exit(1);
        }

        float total_ram = si.totalram;
        float free_ram = si.freeram;
        float used_ram = total_ram - free_ram;
        return (used_ram / total_ram) * 100;
}

// Function to get network usage
void get_network_usage(long *received, long *transmitted)
{
        FILE *fp = fopen("/proc/net/dev", "r");
        if (fp == NULL)
        {
                perror("Error opening /proc/net/dev");
                exit(1);
        }

        char line[200];
        *received = 0;
        *transmitted = 0;

        // Skip first two lines (headers)
        fgets(line, sizeof(line), fp);
        fgets(line, sizeof(line), fp);

        while (fgets(line, sizeof(line), fp))
        {
                char interface[20];
                long recv, trans;
                sscanf(line, "%s %ld %*d %*d %*d %*d %*d %*d %*d %ld",
                       interface, &recv, &trans);
                if (strcmp(interface, "lo:") != 0)
                { // Exclude loopback
                        *received += recv;
                        *transmitted += trans;
                }
        }

        // Convert to KB
        *received /= 1024;
        *transmitted /= 1024;

        fclose(fp);
}

// Function to list active processes
void list_processes()
{
        DIR *dir;
        struct dirent *ent;
        FILE *out = fopen("processes.txt", "w");
        if (out == NULL)
        {
                perror("Error opening processes.txt");
                exit(1);
        }

        dir = opendir("/proc");
        if (dir == NULL)
        {
                perror("Error opening /proc");
                fclose(out);
                exit(1);
        }

        fprintf(out, "PID\tCOMMAND\n");
        while ((ent = readdir(dir)) != NULL)
        {
                if (isdigit(ent->d_name[0]))
                {
                        char path[512];    // Increased buffer size
                        char cmdline[512]; // Increased buffer size
                        if (snprintf(path, sizeof(path), "/proc/%s/cmdline", ent->d_name) < (int)sizeof(path))
                        {
                                FILE *cmd = fopen(path, "r");
                                if (cmd != NULL)
                                {
                                        if (fgets(cmdline, sizeof(cmdline), cmd) != NULL)
                                        {
                                                fprintf(out, "%s\t%s\n", ent->d_name, cmdline);
                                        }
                                        fclose(cmd);
                                }
                        }
                }
        }

        closedir(dir);
        fclose(out);
}

int main()
{
        FILE *usage_file = fopen("usage.txt", "w");
        if (usage_file == NULL)
        {
                perror("Error opening usage.txt");
                return 1;
        }

        // Fixed format string by escaping % symbols
        fprintf(usage_file, "Timestamp\tCPU(%%)\tMemory(%%)\tNetwork_Received(KB)\tNetwork_Transmitted(KB)\n");

        CPUStats prev_stats, curr_stats;
        read_cpu_stats(&prev_stats);

        while (1)
        {
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char timestamp[9];
                strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);

                read_cpu_stats(&curr_stats);
                float cpu_usage = calculate_cpu_usage(&prev_stats, &curr_stats);
                memcpy(&prev_stats, &curr_stats, sizeof(CPUStats));

                float memory_usage = get_memory_usage();

                long received, transmitted;
                get_network_usage(&received, &transmitted);

                fprintf(usage_file, "%s\t%.2f\t%.2f\t%ld\t%ld\n",
                        timestamp, cpu_usage, memory_usage, received, transmitted);
                fflush(usage_file);

                list_processes();
                sleep(2);
        }

        fclose(usage_file);
        return 0;
}