# Project NMS: Networked Management System
## Aryan Gupta @ IIIT-H
## Overview
The NMS project in C is a sophisticated system leveraging TCP and threading to facilitate robust communication and data management between clients and servers. This system is designed to handle concurrent requests, manage data efficiently, and ensure high availability and reliability through advanced mechanisms like redundancy and caching.

## Key Components
- **client1.c**: Manages client-side interactions.
- **server.c**: Acts as the central naming server, orchestrating the entire operation.
- **storage_srvr.c**: Handles storage server operations.
- **makefile**: Compiles and links the C files.

## Key Terms and Concepts

### Initialising
Initialization in this context refers to setting up the server and client, server, storage server environments, including configuring network parameters, initializing data structures, and preparing the system for operation. This process is crucial for ensuring that the system components are ready to handle requests and perform their designated functions efficiently.

### Threading
Threading is employed to handle multiple client and storage server connections concurrently. Each new connection spawns a new thread, allowing the server to manage multiple interactions in parallel. This approach significantly enhances the system's ability to handle a high volume of requests without blocking or delaying responses.

### Communication between Client-Server, Server-Storage, and Storage-Client
Our system facilitates seamless communication across three key interfaces: 
- **Client-Server**: Clients connect to the main server for requests.
- **Server-Storage**: The server interacts with storage servers for data handling.
- **Storage-Client**: Storage servers communicate directly with clients for data transmission.

### Read, Write, Make, Delete, Copy, Get Info
These operations form the core functionality of the storage server, enabling it to handle various file operations like reading, writing, creating, deleting, copying files, and retrieving file information. These operations are essential for data management and manipulation.

### Feedback and Error Codes
The system implements robust feedback mechanisms and error handling protocols. Error codes are used to signify different types of issues encountered during operations, ensuring that users and administrators are well-informed about the system's state and any necessary corrective actions.

### Concurrent Access
Our architecture supports concurrent access to resources. This is achieved through threading and synchronization mechanisms, ensuring that multiple clients can interact with the server simultaneously without data corruption or loss.

### Efficient Searching Using Hash Maps
Hash maps are utilized for efficient data searching and retrieval. This data structure allows for quick lookup of information, significantly reducing the time taken to process client requests.

### LRU Caching
Least Recently Used (LRU) caching is implemented to optimize data retrieval. This caching strategy involves storing recently accessed data for quick access, thereby reducing response times for frequently requested data.

### Redundancy/Replication
Data redundancy and replication are key aspects of our system, ensuring data availability and reliability. This involves maintaining multiple copies of data across different storage servers, safeguarding against data loss in case of server failures.

### Failure Detection
The system is equipped with mechanisms to detect failures in servers or network components. This feature is critical for maintaining system stability and triggering appropriate recovery procedures.

### Data Redundancy and Replication
In addition to standard redundancy, data replication is asynchronously performed to ensure data integrity and availability across different storage locations.

### SS Recovery
Storage Server (SS) recovery processes are in place to handle scenarios where storage servers experience issues or failures. These processes ensure minimal disruption to the system's overall functioning.

### Asynchronous Duplication
Data is duplicated asynchronously across storage servers. This method ensures that the primary operations are not hindered while maintaining up-to-date data copies.

### Logging and Message Display
Comprehensive logging and message display mechanisms are implemented for monitoring and debugging purposes. This includes logging of system operations, error messages, and user interactions.

### IP Address and Port Recording
The system records the IP addresses and ports of connected clients and storage servers. This information is crucial for network communications and for maintaining a record of active connections.



---

## Getting Started
`make all`
`./server`
spwan `./cleint1  & ./storage_srvr` as per your needs 

## How to Use:

COPY Command Syntax: 

`COPY [source_file_path] [destination_folder_path]`
Copy files from one location to another with ease.

`READ, WRITE, DELETE, MAKE, INFO FILENAME COMMAND`:
Standard file operations for managing your files effectively.

`LS` Command:
List files and directories within the specified home path.

Live Logging:
All system events are logged in real-time. Check the 'history.txt' file for detailed information.
Adding Storage Servers:
Storage servers can be added dynamically for increased storage capacity.


# Storage Server 

## Overview
The storage server component of the Networked Management System (NMS) is a critical module designed to handle file operations and client requests in a multi-threaded environment. It is written in C and uses TCP for network communication. This README details the functionality, structure, and usage of the storage server's code.

## Key Features
- **File Operations**: Supports reading, writing, creating, and deleting files and directories.
- **Client Request Handling**: Processes requests from clients, executing file operations based on the commands received.
- **Threading**: Utilizes multi-threading to handle multiple client connections concurrently.
- **Network Communication**: Uses TCP sockets to communicate with clients and the main server.

## Code Structure
The storage server's code is organized into several functions, each serving a specific purpose:

### Main Functionality
- `int main()`: Initializes the server, creates a backup directory, and sets up socket connections for communication with the main server and clients. It also spawns a server thread to listen and accept client connections.

### Thread Handling
- `void *server_thread(void *port)`: Handles incoming client connections on a dedicated thread.
- `void *handle_client1_connection(void *socket_desc)`: Manages the communication with each connected client, processing their requests.

### File Operations
- `void ReadFile(const char *filename, int server_sock)`: Reads a file's content and sends it to the client.
- `void WriteToFile(const char *filename, const char *message)`: Appends a message to the specified file.
- `void createEmptyFileOrDirectory(char *basePath, const char *name, int isDirectory)`: Creates a new file or directory based on the client's request.
- `int isDirectory(const char *path)`: Checks if a given path is a directory.
- `void deleteFileOrDirectory(const char *path)`: Deletes a file or directory.
- `char *searchFilePath(const char *homepath, const char *searchFileName)`: Searches for a file path in the system.
- `void FileInfo(const char *fileName, int server_sock)`: Sends file information to the client.

### Utility Functions
- `void createFileAndWriteFilePaths()`: Creates a file listing all file paths in the server's directory.
- `void writeFilePathsRecursively(FILE *file, const char *currentPath)`: Recursively writes file paths to a file for tracking.
- `void tokenize(char *input, const char *delimiter, char *commands[])`: Tokenizes input strings for command processing.

## Concurrent Access and Locking
The server uses `pthread_mutex_t writelock` for synchronizing write operations to prevent data races and ensure consistency during concurrent file accesses.

## Network Communication
TCP sockets are used for reliable communication. The server listens on a specific port and accepts incoming connections from clients. It maintains a separate thread for each client to ensure concurrent handling of requests.

## Error Handling and Feedback
The server provides feedback for each operation and implements robust error handling. It sends appropriate messages back to the clients, ensuring clarity in communication and operation status.


Based on the provided code for the NMS server, I will draft a detailed `README.md` that outlines the functionalities and key features of the server component of your project. This README will provide an overview of the server's capabilities, its implementation details, and how it interacts with clients and storage servers.

---

# NMS Server - Networked Management System

## Overview
The NMS Server is a critical component of the Networked Management System, designed to facilitate efficient and robust management of client requests and storage server operations. It employs TCP/IP for communication, threads for concurrent processing, and implements a range of functionalities including file handling and caching.

## Features
- **Multi-threaded Operation**: The server can handle multiple client and storage server connections concurrently, using threading.
- **Client and Storage Server Handlers**: Dedicated handlers for both clients and storage servers to process requests and manage connections.
- **File Operations**: Supports operations like copy, read, write, delete, and directory creation.
- **Least Recently Used (LRU) Caching**: Implements LRU caching for efficient file retrieval.
- **Logging**: Logs messages and events for monitoring and debugging purposes.
- **Dynamic Storage Server Management**: Dynamically handles storage server connections and maintains their state.
- **Hash Mapping**:  To find the source file path faster than iterating recursively in a directory.

## Implementation Details

### Threading
- Utilizes `pthread` for creating and managing threads.
- Each client and storage server connection is handled in a separate thread, allowing for concurrent processing.

### Network Communication
- Employs TCP/IP sockets for network communication.
- Listens on multiple ports for incoming connections from clients and storage servers.

### File Management
- Provides functionalities to create, copy, delete files and directories.
- Implements functions like `copyFile`, `copyDirectory`, `createEmptyFileOrDirectory` for file handling.

### Caching
- The server maintains an LRU cache to store recently accessed files for quick retrieval.
- Cache operations include `put` (add to cache), `get_file` (retrieve from cache), and `printCache` (display cache contents).

### Logging and Monitoring
- Uses `log_message` function to log server events and client interactions.
- Includes time-stamped logging for better traceability.

### Storage Server Management
- Manages storage server connections in an array, tracking their state and information.
- Handles storage server registration and dynamically updates backup relationships.

### Error Handling and Validation
- Implements error handling in network operations and file handling.
- Validates client requests and storage server responses.

## Code Snippets

### Main Server Thread Creation
```c
// Create two threads for two ports
if (pthread_create(&thread1, NULL, server_thread, (void *)&port1) != 0 || pthread_create(&thread2, NULL, server_thread, (void *)&port2) != 0)
{
    perror("could not create server threads");
    return 1;
}
```

### Client Handler
```c
void *client1_handler(void *socket_desc)
{
    // Code to handle client requests...
}
```

### Storage Server Handler
```c
void *storage_srvr_handler(void *socket_desc)
{
    // Code to handle storage server connections...
}
```

### LRU Cache Implementation
```c
void put(LRUCache *lruCache, int uid, const char *filename)
{
    // Code to add a file to LRU cache...
}
```

### Hash Table Implementation

The hash table implementation consists of two main structures: `files_path.txt` for files and `&  directories. Each structure includes key information and an array to store files or directories.

