#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

void* printThread(void* lpParam) {
    SOCKET clientSocket = *(SOCKET*)lpParam;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        printf("\n%s", buffer);
    }

    return NULL;
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create a socket for the client
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        printf("Could not create client socket\n");
        return 1;
    }

    // Connect to the server
    memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
        printf("Could not connect to server: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Connected to server at %s:%d\n", SERVER_ADDRESS, PORT);

    // Create a separate thread to handle printing of received messages
    HANDLE printThreadHandle = (HANDLE)_beginthreadex(NULL, 0, printThread, &clientSocket, 0, NULL);

    // Send and receive messages to/from the server
    while (1) {
        // Use select() to check if there is any data available to be received from the server
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int ready = select(clientSocket + 1, &readSet, NULL, NULL, &timeout);
        if (ready == SOCKET_ERROR) {
            printf("select() failed\n");
            break;
        }
        else if (ready == 0) {
            // No data available to be received, do something else
            // Send a message to the server
            //printf("Enter your message: ");
            fgets(buffer, sizeof(buffer), stdin);
            if (send(clientSocket, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
                printf("Could not send message\n");
                break;
            }
            continue;
        }
    }

    // Cleanup
    WaitForSingleObject(printThreadHandle, INFINITE);
    CloseHandle(printThreadHandle);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
