#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#define PORT1 8080
#define PORT2 9090
#define MAX_SERVERS 10
#define MAX_PATH_LENGTH 512

// Structure to hold client details
typedef struct
{
    int id;
    int sock;
    struct sockaddr_in addr;
    int isactive;
    char *basepath;
    int backup1;
    int backup2;
} storage_servers;

storage_servers storage_array[MAX_SERVERS];
int storage_count = 1;
pthread_mutex_t storage_ser = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t history = PTHREAD_MUTEX_INITIALIZER;
char ipaddr[1000];
// initialise ip address as 127.0.0.1

typedef struct Node
{
    int uid;
    char filename[1024]; // Assuming a maximum filename length of 50 characters
    struct Node *next;
} Node;

typedef struct LRUCache
{
    int capacity;
    Node *cache;
} LRUCache;

LRUCache *lru;

void *client1_handler(void *socket_desc);
void *storage_srvr_handler(void *socket_desc);
void *server_thread(void *port);
int copy_files_filepath(char *fname, char *fpath);
char *searchFilePath(char *homepath, char *searchFileName);
int copyFile(char *sourcePath, char *destinationDir);
int makescopy(int uid, char *filefullpath, char *folderfullpath2);
void log_message(const char *message);
int isDirectory(const char *path);
int createDirectory(char *path);
int copyDirectory(char *sourcePath, char *destinationDir);
void createEmptyFileOrDirectory(char *basePath, const char *name, int isDirectory);
void put(LRUCache *lruCache, int uid, const char *filename);
void printCache(LRUCache *lruCache);
int get_file(LRUCache *lruCache, const char *filename);
void LS(char *homepath);

LRUCache *initCache(int capacity)
{
    LRUCache *lruCache = (LRUCache *)malloc(sizeof(LRUCache));
    lruCache->capacity = capacity;
    lruCache->cache = NULL;
    return lruCache;
}

void LS(char *homepath)
{
    // concatenate home path to filename
    char temp[1024];
    strcpy(temp, homepath);
    strcat(temp, "/file_paths.txt");
    char *filename = temp;
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        log_message("LS Failed due to file not found");
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char line[MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Remove newline character from the end of the line
        size_t lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] == '\n')
        {
            line[lineLength - 1] = '\0';
        }
        // Print the line
        printf("%s\n", line);
    }
    log_message("LS Done Successfully");
    fclose(file);
}

int get_file(LRUCache *lruCache, const char *filename)
{
    Node *current = lruCache->cache;
    Node *prev = NULL;

    // Traverse the cache to find the node with the given filename
    while (current != NULL)
    {
        if (strcmp(current->filename, filename) == 0)
        {
            // Move the found node to the front (most recently used)
            if (prev != NULL)
            {
                prev->next = current->next;
                current->next = lruCache->cache;
                lruCache->cache = current;
            }
            return current->uid;
        }

        prev = current;
        current = current->next;
    }

    return 0; // Return NULL if filename is not in the cache
}

void put(LRUCache *lruCache, int uid, const char *filename)
{
    // Check if the cache is full
    if (lruCache->cache != NULL && lruCache->capacity > 0)
    {
        // Remove the least recently used item from the cache
        Node *temp = lruCache->cache;
        while (temp->next != NULL && temp->next->next != NULL)
        {
            temp = temp->next;
        }

        free(temp->next);
        temp->next = NULL;
    }

    // Add the new item to the cache at the front
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->uid = uid;
    strcpy(newNode->filename, filename);
    newNode->next = lruCache->cache;
    lruCache->cache = newNode;
}

void printCache(LRUCache *lruCache)
{
    Node *current = lruCache->cache;

    printf("LRU Cache: ");
    while (current != NULL)
    {
        printf("(%d, %s) ", current->uid, current->filename);
        current = current->next;
    }
    printf("\n");
}

int makescopy(int uid, char *filefullpath, char *folderfullpath2)
{
    if (isDirectory(filefullpath))
    {
        char temp1[1024];
        char temp2[1024];
        strcpy(temp1, filefullpath);
        strcpy(temp2, folderfullpath2);
        // createEmptyFileOrDirectory(temp2, fname, 1);
        if (temp2[strlen(temp2) - 1] != '/')
        {
            strcat(temp2, "/"); // Append "/" if not already present
        }
        snprintf(temp2, sizeof(temp2), "%s/%d", temp2, uid);
        copyDirectory(filefullpath, temp2);
    }
    else
    {
        return copyFile(filefullpath, folderfullpath2);
    }
}

int copy_files_filepath(char *fname, char *fpath)
{
    char *filefullpath, *folderfullpath2;

    pthread_mutex_lock(&storage_ser);
    int flag = -1;
    for (int i = 0; i < storage_count; i++)
    {
        if (storage_array[i].isactive != 1)
        {
            continue;
        }
        filefullpath = searchFilePath(storage_array[i].basepath, fname);
        if (filefullpath != NULL)
        {
            flag = i;
            // set p1 as flag
            break;
        }
    }
    for (int i = 0; i < storage_count; i++)
    {
        if (storage_array[i].isactive != 1)
        {
            continue;
        }
        folderfullpath2 = searchFilePath(storage_array[i].basepath, fpath);
        if (folderfullpath2 != NULL)
        {
            flag = i;
            // set p2 as flag
            break;
        }
    }
    pthread_mutex_unlock(&storage_ser);
    if (folderfullpath2 == NULL || filefullpath == NULL)
    {
        printf("File or folder not found\n");
        log_message("File or folder not found");
        return 0;
    }

    if (isDirectory(filefullpath))
    {
        char temp1[1024];
        char temp2[1024];

        strcpy(temp1, filefullpath);
        strcpy(temp2, folderfullpath2);

        // createEmptyFileOrDirectory(temp2, fname, 1);
        if (temp2[strlen(temp2) - 1] != '/')
        {
            strcat(temp2, "/"); // Append "/" if not already present
        }
        strcat(temp2, fname);
        return copyDirectory(filefullpath, temp2);
    }
    else
    {
        return copyFile(filefullpath, folderfullpath2);
    }
}

int main()
{
    pthread_t thread1, thread2;
    lru = initCache(1000);
    int port1 = PORT1;
    int port2 = PORT2;

    // pthread_mutex_lock(&storage_ser);
    // for (int i = 0; i < MAX_SERVERS; i++)
    // {
    //     storage_array[i].isactive = 0;
    // }
    // pthread_mutex_unlock(&storage_ser);

    // Initialize storage_array array
    memset(storage_array, 0, sizeof(storage_array));
    strcpy(ipaddr, "127.0.0.1");
    // Create two threads for two ports
    if (pthread_create(&thread1, NULL, server_thread, (void *)&port1) != 0 || pthread_create(&thread2, NULL, server_thread, (void *)&port2) != 0)
    {
        perror("could not create server threads");
        return 1;
    }
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}

void *server_thread(void *port)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t thread_id;
    int port_no = *(int *)port;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_no);
    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Listen for client connections
    if (listen(server_fd, MAX_SERVERS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // Accept clients and create a handler thread for each
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)))
    {
        void *(*handler)(void *) = (port_no == PORT1) ? client1_handler : storage_srvr_handler;
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        if (pthread_create(&thread_id, NULL, handler, (void *)new_sock) != 0)
        {
            perror("could not create thread");
            break;
        }
    }
    if (new_socket < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void *client1_handler(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    char client_message[2000];
    int read_size;
    while (1)
    {
        // Process messages from Client 1
        if ((read_size = recv(sock, client_message, 2000, 0)) > 0)
        {
            printf("Client %d: %s\n", sock, client_message);
            char temp2[1024];
            strcpy(temp2, "Client ");
            char uid1[10];
            sprintf(uid1, "%d", sock);
            strcat(temp2, uid1);
            strcat(temp2, ": ");
            strcat(temp2, client_message);
            log_message(temp2);
        }
        if (read_size == 0)
        {
            puts("Client 1 disconnected");
            char temp3[1024];
            strcpy(temp3, "Client ");
            char uid2[10];
            sprintf(uid2, "%d", sock);
            strcat(temp3, uid2);
            strcat(temp3, " disconnected");
            log_message(temp3);
            fflush(stdout);
            break;
        }
        else if (read_size == -1)
        {
            perror("recv failed");
            break;
        }
        // send all uids to client 1

        // for now assuming clients wants to copy file a.txt to folder b
        // strcpy(client_message, "COPY a.txt b") ;
        char temp[2000];
        strcpy(temp, client_message);

        char *commands[1000];
        commands[0] = strtok(temp, " \t\n");
        int i = 0;

        while (commands[i] != NULL)
        {
            commands[++i] = strtok(NULL, " \t\n");
        }

        if (strcmp("COPY", commands[0]) == 0)
        {
            printf("%s %s\n", commands[1], commands[2]);
            char *filefullpath = commands[1], *folderfullpath2 = commands[2];
            int a = copy_files_filepath(filefullpath, folderfullpath2);
            if (a)
                send(sock, "1", strlen("1"), 0);
            else
                send(sock, "0", strlen("0"), 0);

            // signaling the client that the file was copied
            char *filename = commands[1], *fpv;
            pthread_mutex_lock(&storage_ser);
            int flag = -1;
            int check = get_file(lru, filename);
            if (check == 1)
            {
                printf("File found in cache\n");
                log_message("File found in cache");
                log_message(filename);
                put(lru, check, filename);
                flag = check;
            }
            else
            {
                for (int i = 0; i < storage_count; i++)
                {
                    if (storage_array[i].isactive != 1)
                    {
                        continue;
                    }
                    fpv = searchFilePath(storage_array[i].basepath, filename);
                    if (fpv != NULL)
                    {
                        flag = storage_array[i].id;
                        put(lru, flag, filename);
                        log_message("File found in storage server");
                        log_message(filename);
                        log_message("pushed To LRU Cashe");
                        break;
                    }
                }
            }
            char uid[100];
            sprintf(uid, "%d", flag);
            send(sock, uid, strlen(uid), 0);
            flag = -1;
            filename = commands[2];
            check = get_file(lru, filename);
            if (check == 1)
            {
                printf("File found in cache\n");
                log_message("File found in cache");
                log_message(filename);
                put(lru, check, filename);
                flag = check;
            }
            else
            {
                for (int i = 0; i < storage_count; i++)
                {
                    if (storage_array[i].isactive != 1)
                    {
                        continue;
                    }
                    fpv = searchFilePath(storage_array[i].basepath, filename);
                    if (fpv != NULL)
                    {
                        flag = storage_array[i].id;
                        put(lru, flag, filename);
                        log_message("File found in storage server");
                        log_message(filename);
                        log_message("pushed To LRU Cashe");
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&storage_ser);
            char uid2[100];
            sprintf(uid2, "%d", flag);
            send(sock, uid2, strlen(uid2), 0);
        }
        else if ((strcmp(commands[0], "READ") == 0) || (strcmp(commands[0], "WRITE") == 0) || (strcmp(commands[0], "DELETE") == 0) || (strcmp(commands[0], "MAKE") == 0) || (strcmp(commands[0], "INFO") == 0))
        {
            char *filename = commands[1], *filefullpath;
            pthread_mutex_lock(&storage_ser);
            int flag = -1;

            int check = get_file(lru, filename);
            if (check == 1)
            {
                printf("File found in cache\n");
                log_message("File found in cache");
                log_message(filename);
                put(lru, check, filename);
                flag = check;
            }
            else
            {
                for (int i = 0; i < storage_count; i++)
                {
                    if (storage_array[i].isactive != 1)
                    {
                        continue;
                    }
                    filefullpath = searchFilePath(storage_array[i].basepath, filename);
                    if (filefullpath != NULL)
                    {
                        flag = storage_array[i].id;
                        put(lru, flag, filename);
                        log_message("File found in storage server");
                        log_message(filename);
                        log_message("pushed To LRU Cashe");
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&storage_ser);
            if (filefullpath == NULL)
            {
                printf("File or folder not found\n");
                // return 0;
            }
            char uid[100];
            sprintf(uid, "%d", flag);
            send(sock, uid, strlen(uid), 0);
        }
        else if (strcmp(commands[0], "LS") == 0)
        {
            for (int i = 0; i < storage_count; i++)
            {
                if (storage_array[i].isactive != 1)
                {
                    continue;
                }
                LS(storage_array[i].basepath);
            }
        }
    }
    free(socket_desc);
    printf("Client sock %d disconnected\n", sock);
    snprintf(client_message, sizeof(client_message), "Client sock %d disconnected\n", sock);
    log_message(client_message);
    return 0;
}

char *searchFilePath(char *homepath, char *searchFileName)
{
    // contactnate home path to filename
    char temp[1024];
    strcpy(temp, homepath);
    strcat(temp, "/file_paths.txt");
    char *filename = temp;
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char line[MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Remove newline character from the end of the line
        size_t lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] == '\n')
        {
            line[lineLength - 1] = '\0';
        }
        // Compare the line with the target file name
        if (strcmp(searchFileName, strrchr(line, '/') + 1) == 0)
        {
            // Found the file, close the file and return the path
            fclose(file);
            return strdup(line); // Duplicate the path to ensure it survives after closing the file
        }
    }
    // File not found, close the file and return NULL
    fclose(file);
    return NULL;
}

int copyFile(char *sourcePath, char *destinationDir)
{
    // Open the source file for reading
    FILE *sourceFile = fopen(sourcePath, "rb");
    if (sourceFile == NULL)
    {
        return 0;
        perror("fopen source file");
        exit(EXIT_FAILURE);
    }
    // Extract the file name from the source path
    char *fileName = strrchr(sourcePath, '/');
    if (fileName == NULL)
    {
        fileName = sourcePath; // No '/' found, use the full path as the filename
    }
    else
    {
        fileName++; // Move past the '/'
    }

    // Construct the destination path by combining the destination directory and file name
    char destinationPath[512];
    snprintf(destinationPath, sizeof(destinationPath), "%s/%s", destinationDir, fileName);
    // Open the destination file for writing
    FILE *destinationFile = fopen(destinationPath, "wb");
    if (destinationFile == NULL)
    {
        fclose(sourceFile);
        return 0;
        perror("fopen destination file");
        exit(EXIT_FAILURE);
    }
    // Copy the content of the source file to the destination file
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }
    // Close the files
    fclose(sourceFile);
    fclose(destinationFile);
    printf("File '%s' copied to '%s'.\n", sourcePath, destinationPath);
    char temp1[1024];
    strcpy(temp1, "File '");
    strcat(temp1, sourcePath);
    strcat(temp1, "' copied to '");
    strcat(temp1, destinationPath);
    strcat(temp1, "'.\n");
    log_message(temp1);
    return 1;
}

void *storage_srvr_handler(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    char *path, serv_message[2000];
    path = malloc(2000 * sizeof(char));
    int read_size, id = storage_count;
    read_size = recv(sock, path, 2000, 0);
    if (read_size == 0)
    {
        puts("Client 2 disconnected");
        fflush(stdout);
        printf("Storage Server %d of sock %d disconnected\n", id, sock);
        free(socket_desc);
        char temp4[1024];
        strcpy(temp4, "Storage Server ");
        char uid3[10];
        sprintf(uid3, "%d", id);
        strcat(temp4, uid3);
        strcat(temp4, " of sock ");
        char uid4[10];
        sprintf(uid4, "%d", sock);
        strcat(temp4, uid4);
        strcat(temp4, " disconnected");
        log_message(temp4);
        return 0;
    }
    else if (read_size == -1)
    {
        perror("recv failed");
        printf("Storage Server %d of sock %d disconnected\n", id, sock);
        free(socket_desc);
        return 0;
    }
    pthread_mutex_lock(&storage_ser);
    // Add client details to storage_array array
    storage_array[storage_count].id = id;
    storage_array[storage_count].sock = sock;
    storage_array[storage_count].isactive = 1;
    storage_array[storage_count].basepath = path;
    storage_array[storage_count].backup1 = -1;
    storage_array[storage_count].backup2 = -1;

    if (storage_count == 1)
    {
        // storage_array[storage_count].backup1 = 1;
        // storage_array[storage_count].backup2 = 2;
    }
    else if (storage_count == 2)
    {
        storage_array[1].backup1 = 2;
        storage_array[2].backup1 = 1;

        char temp1[1024];
        char temp2[1024];

        snprintf(temp1, sizeof(temp1), "%s/Backup", storage_array[1].basepath);
        snprintf(temp2, sizeof(temp2), "%s/Backup", storage_array[2].basepath);

        copyDirectory(storage_array[2].basepath, temp1);
        copyDirectory(storage_array[1].basepath, temp2);
    }
    else if (storage_count == 3)
    {
        storage_array[1].backup2 = 3;
        storage_array[2].backup2 = 3;
        storage_array[3].backup1 = 1;
        storage_array[3].backup2 = 2;

        char temp1[1024];
        char temp2[1024];
        char temp3[1024];

        snprintf(temp1, sizeof(temp1), "%s/Backup", storage_array[1].basepath);
        snprintf(temp2, sizeof(temp2), "%s/Backup", storage_array[2].basepath);
        snprintf(temp3, sizeof(temp3), "%s/Backup", storage_array[3].basepath);

        copyDirectory(storage_array[3].basepath, temp1);
        copyDirectory(storage_array[3].basepath, temp2);
        copyDirectory(storage_array[1].basepath, temp3);
        copyDirectory(storage_array[2].basepath, temp3);
    }
    else
    {
        storage_array[storage_count].backup1 = storage_count - 1;
        storage_array[storage_count].backup2 = storage_count - 2;

        char temp1[1024];
        char temp2[1024];

        snprintf(temp1, sizeof(temp1), "%s/Backup", storage_array[storage_count - 1].basepath);
        snprintf(temp2, sizeof(temp2), "%s/Backup", storage_array[storage_count - 2].basepath);

        copyDirectory(storage_array[storage_count].basepath, temp1);
        copyDirectory(storage_array[storage_count].basepath, temp2);
    }
    storage_count++;
    pthread_mutex_unlock(&storage_ser);
    // make a char array of the port number
    char uid[10];
    sprintf(uid, "%d", id);
    send(sock, uid, strlen(uid), 0);
    printf("Storage Server %s @ %s, socket %d was added\n", uid, path, sock);
    char temp5[1024];
    strcpy(temp5, "Storage Server ");
    strcat(temp5, uid);
    strcat(temp5, " @ ");
    strcat(temp5, path);
    strcat(temp5, ", socket ");
    char uid5[10];
    sprintf(uid5, "%d", sock);
    strcat(temp5, uid5);
    strcat(temp5, " was added");
    log_message(temp5);
    while (1)
    {
        serv_message[0] = '\0';
        // Process messages from Client 2
        if ((read_size = recv(sock, serv_message, 2000, 0)) > 0)
        {
            printf("Storage Server of id %d, socket %d says %s\n", id, sock, serv_message);
        }
        if (read_size == 0)
        {
            puts("Server disconnected");
            fflush(stdout);
            break;
        }
        else if (read_size == -1)
        {
            perror("recv failed");
            break;
        }
        send(sock, "reconnect", strlen("reconnect"), 0);
    }
    // Process messages from Client 2 (if needed)
    // ...
    // Remove client details from storage_array array
    pthread_mutex_lock(&storage_ser);
    storage_array[id].isactive = 0;
    pthread_mutex_unlock(&storage_ser);
    printf("Storage Server %d of sock %d disconnected\n", id, sock);
    char temp6[1024];
    strcpy(temp6, "Storage Server ");
    char uid6[10];
    sprintf(uid6, "%d", id);
    strcat(temp6, uid6);
    strcat(temp6, " of sock ");
    char uid7[10];
    sprintf(uid7, "%d", sock);
    strcat(temp6, uid7);
    strcat(temp6, " disconnected");
    log_message(temp6);
    free(socket_desc);
    return 0;
}

void log_message(const char *message)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    // lock the mutex
    pthread_mutex_lock(&history);
    FILE *file = fopen("history.txt", "a");
    if (file != NULL)
    {
        fprintf(file, "FROM IP ADDRESS %s", ipaddr);
        fprintf(file, " %s\n", message);
        fclose(file);
    }
    else
    {
        printf("Error opening file!\n");
    }
    close(file);
    // unlock the mutex
    pthread_mutex_unlock(&history);
}

int isDirectory(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

int createDirectory(char *path)
{
#ifdef _WIN32
    return _mkdir(path);
#else
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        return mkdir(path, 0777);
    }
    return 0; // Directory already exists
#endif
}

int copyDirectory(char *sourcePath, char *destinationDir)
{
    // Create the destination directory if it doesn't exist
    if (createDirectory(destinationDir) != 0)
    {
        perror("mkdir destination directory");
        exit(EXIT_FAILURE);
    }

    // Open the source directory
    DIR *sourceDir = opendir(sourcePath);
    if (sourceDir == NULL)
    {
        perror("opendir source directory");
        exit(EXIT_FAILURE);
    }

    // Iterate over the contents of the source directory
    struct dirent *entry;
    while ((entry = readdir(sourceDir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "Backup") != 0)
        {
            // Construct the source and destination paths for the current entry
            char sourceEntryPath[512];
            snprintf(sourceEntryPath, sizeof(sourceEntryPath), "%s%c%s", sourcePath, PATH_SEPARATOR, entry->d_name);
            char destinationEntryPath[512];
            snprintf(destinationEntryPath, sizeof(destinationEntryPath), "%s%c%s", destinationDir, PATH_SEPARATOR, entry->d_name);

            // Recursively copy files and directories
            if (isDirectory(sourceEntryPath))
            {
                copyDirectory(sourceEntryPath, destinationEntryPath);
            }
            else
            {
                copyFile(sourceEntryPath, destinationDir);
            }
        }
    }

    // Close the source directory
    closedir(sourceDir);

    return 1;
}

void createEmptyFileOrDirectory(char *basePath, const char *name, int isDirectory)
{
    char fullPath[1024];
    char *token[100];

    char temp[1024];
    strcpy(temp, name);

    token[0] = strtok(temp, "/");
    int i = 0;
    while (token[i] != NULL)
    {
        token[++i] = strtok(NULL, "/");
    }

    for (int j = 0; j < i; j++)
    {
        printf("%s\n", token[j]);
    }

    for (int j = 0; j < i; j++)
    {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", basePath, token[j]);
        strcpy(basePath, fullPath);

        struct stat st;
        if (stat(fullPath, &st) == 0)
        {
            // write(server_sock, "Error: File or directory already exists.\n", 1024);
            printf("Error: File or directory already exists.\n");
            return; // Return an error code
        }

        int flagIsDir = 1;

        for (int k = 0; k < strlen(token[j]); k++)
        {
            if (token[j][k] == '.')
            {
                flagIsDir = 0;
                break;
            }
        }

        if (flagIsDir)
        {
            if (mkdir(fullPath, 0700) != 0)
            {
                perror("mkdir");
                return; // Return an error code
            }
            printf("Directory '%s' created successfully.\n", fullPath);
        }
        // Create an empty file
        else
        {
            FILE *file = fopen(fullPath, "w");
            if (file == NULL)
            {
                perror("fopen");
                return; // Return an error code
            }
            fclose(file);
            printf("File '%s' created successfully.\n", fullPath);
        }
    }
    // return 0;  // Return success
}