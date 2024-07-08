#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define MAX_CLIENTS 10

int main()
{
    int server_sock, storage_sock;
    struct sockaddr_in server_addr, storage_srvr_addr;
    // Connect to the server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }
    char message[2000], server_message[100000];
    char temp[2000];
    char *commands[1000];

    while (1)
    {
        // memset all the buffers
        memset(message, 0, sizeof(message));
        memset(server_message, 0, sizeof(server_message));
        memset(temp, 0, sizeof(temp));
        memset(commands, 0, sizeof(commands));
        server_message[0] = '\0';
        // send message from main server
        fgets(message, sizeof(message), stdin);

        strcpy(temp, message);

        commands[0] = strtok(temp, " \t\n");
        int i = 0;
        while (commands[i] != NULL)
        {
            commands[++i] = strtok(NULL, " \t\n");
        }

        if (strcmp(commands[0], "COPY") == 0)
        {
            write(server_sock, message, strlen(message));
            // read message from main server
            if (read(server_sock, server_message, 100000) > 0)
            {
                printf("Was copy successful %s\n", server_message);
            }
            if (read(server_sock, server_message, 100000) > 0)
            {
                if (atoi(server_message) <= 0)
                {
                    printf("File was not found\n");
                    continue;
                }
                storage_sock = socket(AF_INET, SOCK_STREAM, 0);
                storage_srvr_addr.sin_family = AF_INET;
                int port = 8080 + atoi(server_message);
                if (port <= 8080)
                {
                    printf("Connection denied please reconnect\n");
                    return 0;
                }
                storage_srvr_addr.sin_port = htons(port);
                inet_pton(AF_INET, "127.0.0.1", &storage_srvr_addr.sin_addr);
                if (connect(storage_sock, (struct sockaddr *)&storage_srvr_addr, sizeof(storage_srvr_addr)) < 0)
                {
                    perror("Connection Failed");
                    exit(EXIT_FAILURE);
                }
                write(storage_sock, "RESETLS", strlen("RESETLS"));
                char ser_message[100000];
                memset(ser_message, 0, sizeof(ser_message));
                if (read(storage_sock, ser_message, 100000) > 0)
                {
                    printf("%s", ser_message);
                }
                printf("\n");
                close(storage_sock);
            }
            if (read(server_sock, server_message, 100000) > 0)
            {
                if (atoi(server_message) <= 0)
                {
                    printf("File was not found\n");
                    continue;
                }
                storage_sock = socket(AF_INET, SOCK_STREAM, 0);
                storage_srvr_addr.sin_family = AF_INET;
                int port = 8080 + atoi(server_message);
                if (port <= 8080)
                {
                    printf("Connection denied please reconnect\n");
                    return 0;
                }
                storage_srvr_addr.sin_port = htons(port);
                inet_pton(AF_INET, "127.0.0.1", &storage_srvr_addr.sin_addr);
                if (connect(storage_sock, (struct sockaddr *)&storage_srvr_addr, sizeof(storage_srvr_addr)) < 0)
                {
                    perror("Connection Failed");
                    exit(EXIT_FAILURE);
                }
                write(storage_sock, "RESETLS", strlen("RESETLS"));
                char ser_message[100000];
                memset(ser_message, 0, sizeof(ser_message));
                if (read(storage_sock, ser_message, 100000) > 0)
                {
                    printf("%s", ser_message);
                }
                printf("\n");
                close(storage_sock);
            }
        }
        else if ((strcmp(commands[0], "READ") == 0) || (strcmp(commands[0], "WRITE") == 0) || (strcmp(commands[0], "DELETE") == 0) || (strcmp(commands[0], "MAKE") == 0) || (strcmp(commands[0], "INFO") == 0)||(strcmp(commands[0], "LS") == 0))
        {
            write(server_sock, message, strlen(message));
            // read message from main server
            if (read(server_sock, server_message, 100000) > 0)
            {
                if (atoi(server_message) <= 0)
                {
                    printf("File was not found\n");
                    continue;
                }
                printf("File was found in SS %d\n", atoi(server_message));
                // send message to port 8080 after making a connection
                storage_sock = socket(AF_INET, SOCK_STREAM, 0);
                storage_srvr_addr.sin_family = AF_INET;
                int port = 8080 + atoi(server_message);
                if (port <= 8080)
                {
                    printf("Connection denied please reconnect\n");
                    return 0;
                }
                storage_srvr_addr.sin_port = htons(port);
                inet_pton(AF_INET, "127.0.0.1", &storage_srvr_addr.sin_addr);
                if (connect(storage_sock, (struct sockaddr *)&storage_srvr_addr, sizeof(storage_srvr_addr)) < 0)
                {
                    perror("Connection Failed");
                    exit(EXIT_FAILURE);
                }
                write(storage_sock, message, strlen(message));
                char ser_message[100000];
                memset(ser_message, 0, sizeof(ser_message));
                if (read(storage_sock, ser_message, 100000) > 0)
                {
                    printf("%s", ser_message);
                    // Update if required for big files
                }
                printf("\n");
                close(storage_sock);
            }
        }
        else
        {
            printf("Invalid command\n");
        }
        // write(server_sock, message, strlen(message));
        // // read message from main server
        // if (read(server_sock, server_message, 1024) > 0)
        // {
        //     printf("Was copy successful %s\n", server_message);
        // }
    }
    close(server_sock);
    return 0;
}
