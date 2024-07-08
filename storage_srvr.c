#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH_LENGTH 512
#define SERVER_PORT 9090
#define CLIENT1PORT 8080
#define MAX_CLIENTS 10
int id;

void *handle_client1_connection(void *socket_desc);
void *server_thread(void *port);
void tokenize(char *input, const char *delimiter, char *commands[]);
void ReadFile(const char *filename, int server_sock);
void WriteToFile(const char *filename, const char *message);
void createEmptyFileOrDirectory(char *basePath, const char *name, int isDirectory);
int isDirectory(const char *path);
void deleteFileOrDirectory(const char *path);
char *searchFilePath(const char *homepath, const char *searchFileName);
void FileInfo(const char *fileName, int server_sock);

pthread_mutex_t writelock = PTHREAD_MUTEX_INITIALIZER;

char *searchFilePath(const char *homepath, const char *searchFileName)
{
    // contactnate home path to filename
    char temp[1024];
    memset(temp, 0, sizeof(temp));
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
    memset(line, 0, sizeof(line));
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

int isDirectory(const char *path)
{
    struct stat path_stat;

    // Use stat to get information about the file/directory
    if (stat(path, &path_stat) != 0)
    {
        // Print an error message based on errno
        perror("Error in stat");
        return -1; // Return -1 to indicate an error
    }

    // Check if it is a directory
    if (S_ISDIR(path_stat.st_mode))
    {
        return 1; // It's a directory
    }
    else
    {
        return 0; // It's not a directory
    }
}

void createEmptyFileOrDirectory(char *basePath, const char *name, int isDirectory)
{
    char fullPath[1024];
    char *token[100];

    char temp[1024];
    memset(temp, 0, sizeof(temp));
    memset(fullPath, 0, sizeof(fullPath));
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

void deleteFileOrDirectory(const char *path)
{
    // Dynamically allocate fullPath
    char *fullPath = strdup(path);
    if (fullPath == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", fullPath);

    struct stat st;
    if (stat(fullPath, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            // If it's a directory, recursively delete its contents
            DIR *dir = opendir(fullPath);
            if (dir == NULL)
            {
                perror("opendir");
                printf("Unable to open directory: %s\n", fullPath);
                free(fullPath);
                return; // Return an error code
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                {
                    continue; // Skip "." and ".."
                }

                char entryPath[MAX_PATH_LENGTH];
                memset(entryPath, 0, sizeof(entryPath));
                snprintf(entryPath, sizeof(entryPath), "%s/%s", fullPath, entry->d_name);

                // Recursively delete subdirectories and files
                deleteFileOrDirectory(entryPath);
            }

            closedir(dir);

            // Remove the empty directory
            if (rmdir(fullPath) != 0)
            {
                perror("rmdir");
                printf("Unable to delete directory: %s\n", fullPath);
            }
            else
            {
                printf("Successfully deleted directory: %s\n", fullPath);
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            // If it's a file, simply delete it
            if (remove(fullPath) == 0)
            {
                printf("Successfully deleted file: %s\n", fullPath);
            }
            else
            {
                perror("remove");
                printf("Unable to delete file: %s\n", fullPath);
            }
        }
        else
        {
            // If it's neither a file nor a directory, display an error
            printf("Invalid path: %s\n", fullPath);
        }
    }
    else
    {
        printf("File or directory '%s' does not exist.\n", fullPath);
    }

    // Free dynamically allocated fullPath
    free(fullPath);
}

void WriteToFile(const char *filename, const char *message)
{
    char homepath[1024];
    memset(homepath, 0, sizeof(homepath));
    getcwd(homepath, sizeof(homepath));

    char *filePath = searchFilePath(homepath, filename);

    // printf("%s\n", filePath);

    FILE *file = fopen(filePath, "a"); // Open the file in append mode
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Write the message to the end of the file
    fprintf(file, "%s\n", message);
    fclose(file);
    printf("Message: %s\n", message);
}

void ReadFile(const char *filename, int server_sock)
{
    char homepath[1024];
    memset(homepath, 0, sizeof(homepath));
    getcwd(homepath, sizeof(homepath));
    char *filePath = searchFilePath(homepath, filename);
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[100000], temp[1000];
    memset(buffer, 0, sizeof(buffer));
    memset(temp, 0, sizeof(temp));
    buffer[0] = '\0';
    while (fgets(temp, sizeof(temp), file) != NULL)
    {
        strcat(buffer, temp);
        if (strlen(buffer) > 100000)
        {
            printf("File too large to read\n");
            write(server_sock, "File too large to read\n", 1024);
        }
    }
    write(server_sock, buffer, strlen(buffer));
    printf("%s", buffer);
    fclose(file);
}

void writeFilePathsRecursively(FILE *file, const char *currentPath)
{
    // Open the current directory
    DIR *dir = opendir(currentPath);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    // Iterate through the files in the directory
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "Backup") == 0)
        {
            continue;
        }
        // Construct the relative path
        char relativePath[512];
        memset(relativePath, 0, sizeof(relativePath));
        snprintf(relativePath, sizeof(relativePath), "%s/%s", currentPath, entry->d_name);
        // Write the relative path to the file
        fprintf(file, "%s\n", relativePath);
        // If the entry is a directory, recursively write paths inside it
        if (entry->d_type == DT_DIR)
        {
            writeFilePathsRecursively(file, relativePath);
        }
    }
    closedir(dir);
}

void createFileAndWriteFilePaths()
{
    // Get the current working directory
    const char *filename = "file_paths.txt";
    char cwd[256];
    memset(cwd, 0, sizeof(cwd));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    // Open the file for writing (creates the file if it doesn't exist)
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    writeFilePathsRecursively(file, cwd);
    fclose(file);
    printf("File '%s' created with relative paths.\n", filename);
}

void FileInfo(const char *fileName, int server_sock)
{
    char homepath[1024];
    memset(homepath, 0, sizeof(homepath));
    getcwd(homepath, sizeof(homepath));

    char *filePath = searchFilePath(homepath, fileName);

    struct stat fileStat;
    if (stat(filePath, &fileStat) == -1)
    {
        perror("Error getting file information");
        return;
    }

    char message[100000];
    memset(message, 0, sizeof(message));
    message[0] = '\0';
    sprintf(message, "File Information for: %s; File size is: %ld bytes; File Permissions: ", fileName, fileStat.st_size);
    if (fileStat.st_mode & S_IRUSR)
    {
        strcat(message, "r");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IWUSR)
    {
        strcat(message, "w");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IXUSR)
    {
        strcat(message, "x");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IRGRP)
    {
        strcat(message, "r");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IWGRP)
    {
        strcat(message, "w");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IXGRP)
    {
        strcat(message, "x");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IROTH)
    {
        strcat(message, "r");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IWOTH)
    {
        strcat(message, "w");
    }
    else
    {
        strcat(message, "-");
    }
    if (fileStat.st_mode & S_IXOTH)
    {
        strcat(message, "x");
    }
    else
    {
        strcat(message, "-");
    }

    write(server_sock, message, strlen(message));
    printf("%s", message);
}

int main()
{
    createFileAndWriteFilePaths(); // make a file with all the paths
    char homepath[2048];
    getcwd(homepath, sizeof(homepath));

    char backupFolderPath[256];
    snprintf(backupFolderPath, sizeof(backupFolderPath), "%s/Backup", homepath);

    struct stat backupDirStat;
    if (stat(backupFolderPath, &backupDirStat) == 0 && S_ISDIR(backupDirStat.st_mode))
    {
        printf("Backup folder already exists at %s.\n", backupFolderPath);
    }
    else
    {
        // Create the Backup folder if it doesn't exist
        if (mkdir(backupFolderPath, 0700) == 0)
        {
            printf("Backup folder created successfully at %s.\n", backupFolderPath);
        }
        else
        {
            perror("Failed to create Backup folder");
        }
    }

    int server_sock, storage_srvr_server_sock, new_socket;
    struct sockaddr_in server_addr, storage_srvr_addr, client1_addr;
    socklen_t client1_len = sizeof(client1_addr);
    pthread_t thread_id;

    // Connect to the main server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to main server failed");
        exit(EXIT_FAILURE);
    }
    char server_message[1024];
    memset(server_message, 0, sizeof(server_message));
    server_message[0] = '\0';
    storage_srvr_server_sock = socket(AF_INET, SOCK_STREAM, 0); // ipv4, tcp, default
    storage_srvr_addr.sin_family = AF_INET;                     // ipv4
    storage_srvr_addr.sin_addr.s_addr = INADDR_ANY;             // localhost
    storage_srvr_addr.sin_port = htons(CLIENT1PORT);            // port 8080
    // send the current working port to the main server
    char cwd[1024];
    memset(cwd, 0, sizeof(cwd));
    getcwd(cwd, sizeof(cwd));
    write(server_sock, cwd, strlen(cwd));
    if (read(server_sock, server_message, 1024) > 0)
    {
        id = atoi(server_message);
        printf("Server ID: %d\n", id);
    }
    pthread_t server_thread_id;
    int port = CLIENT1PORT + id;
    if (pthread_create(&server_thread_id, NULL, server_thread, (void *)&port) != 0)
    {
        perror("could not create thread");
        exit(EXIT_FAILURE);
    }
    pthread_join(server_thread_id, NULL);
    close(server_sock);
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
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server %d listening on port %d\n", id, port_no);
    // Accept clients and create a handler thread for each
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)))
    {
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        if (pthread_create(&thread_id, NULL, handle_client1_connection, (void *)new_sock) != 0)
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

void *handle_client1_connection(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    char client_message[1024];
    memset(client_message, 0, sizeof(client_message));
    // Read message from Client 1
    if (read(sock, client_message, 1024) > 0)
    {
        pthread_mutex_lock(&writelock);
        createFileAndWriteFilePaths();
        pthread_mutex_unlock(&writelock);
        char temp[1024];
        memset(temp, 0, sizeof(temp));
        strcpy(temp, client_message);

        char *commands[1000];
        commands[0] = strtok(temp, " \t\n");
        int i = 0;

        while (commands[i] != NULL)
        {
            commands[++i] = strtok(NULL, " \t\n");
        }
        // Echo back the message to Client 1
        printf("\nMessage from Client 1: %s\n", client_message);
        if (strcasecmp(client_message, "RESETLS") == 0)
        {
        }
        else if (strcmp(commands[0], "READ") == 0)
        {
            if (i != 2)
            {
                printf("Invalid Arguments\n");
                write(sock, "Invalid arguments", 1024);
            }
            else
                ReadFile(commands[1], sock);
        }
        else if (strcmp(commands[0], "WRITE") == 0)
        {
            if (i <= 2)
            {
                printf("Invalid Arguments\n");
                write(sock, "Invalid arguments", 1024);
            }
            else
            {
                char messageToWrite[1024];
                memset(messageToWrite, 0, sizeof(messageToWrite));
                int numSpace = 0;
                int startIndex = 0;
                for (int i = 0; i < strlen(client_message); i++)
                {
                    if (numSpace < 2)
                    {
                        if (client_message[i] == ' ')
                        {
                            numSpace++;
                            startIndex = i + 1; // Update the start index after encountering each space
                        }
                    }
                    else
                    {
                        messageToWrite[i - startIndex] = client_message[i];
                    }
                }
                // lock the file
                pthread_mutex_lock(&writelock);
                messageToWrite[strlen(client_message) - startIndex] = '\0';
                WriteToFile(commands[1], messageToWrite);
                write(sock, "Message written executed.\n", 1024);
                pthread_mutex_unlock(&writelock);
            }
        }
        else if (strcmp(commands[0], "DELETE") == 0)
        {
            if (i != 2)
            {
                printf("Invalid Arguments\n");
                write(sock, "Invalid arguments", 1024);
            }
            else
            {
                char homepath[1024];
                memset(homepath, 0, sizeof(homepath));
                getcwd(homepath, sizeof(homepath));
                char *filePath = searchFilePath(homepath, commands[1]);
                pthread_mutex_lock(&writelock);
                deleteFileOrDirectory(filePath);
                createFileAndWriteFilePaths();
                pthread_mutex_unlock(&writelock);
                write(sock, "File deleted executed.\n", 1024);
            }
        }
        else if (strcmp(commands[0], "MAKE") == 0)
        {
            if (i != 3)
            {
                printf("Invalid Arguments\n");
                write(sock, "Invalid arguments", 1024);
            }
            else
            {
                int i_d = isDirectory(commands[1]);
                pthread_mutex_lock(&writelock);
                createEmptyFileOrDirectory(commands[1], commands[2], i_d);
                createFileAndWriteFilePaths();
                pthread_mutex_unlock(&writelock);
                write(sock, "File created executed.\n", 1024);
            }
        }
        else if (strcmp(commands[0], "INFO") == 0)
        {
            if (i != 2)
            {
                printf("Invalid Arguments\n");
                write(sock, "Invalid arguments", 1024);
            }
            else
                FileInfo(commands[1], sock);
        }
    }
    close(sock);
    free(socket_desc);
    return NULL;
}
