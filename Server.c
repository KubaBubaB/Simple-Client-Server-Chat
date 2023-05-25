#include <stdio.h>
#include <winsock2.h>
#include <process.h>

#pragma comment(lib,"ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10

HANDLE clientThreads[MAX_CLIENTS];
SOCKET clientSockets[MAX_CLIENTS];
int numClients = 0;

DWORD WINAPI clientThread(LPVOID lpParam) {
    // This function runs in a separate thread for each client
    SOCKET clientSocket = *(SOCKET*)lpParam;
    SOCKADDR_IN clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    // Get the IP address of the client
    char clientIp[46];
    getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, 46);

    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesReceived] = '\0';

        // Send the received message to all other clients
        for (int i = 0; i < numClients; i++) {
            if (clientSocket != clientSockets[i]) {
                char message[1070];
                sprintf_s(message, sizeof(message), "%s: %s", clientIp, buffer);
                send(clientSockets[i], message, strlen(message), 0);
            }
        }

        printf("%s says: %s", clientIp, buffer);
    }

    closesocket(clientSocket);

    return 0;
}


int main() {
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    SOCKADDR_IN serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    // Create a TCP socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return 1;
    }

    // Set the socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        printf("Failed to set socket options : %d", WSAGetLastError());
        return 1;
    }

    // Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
        return 1;
    }

    // Listen for incoming connections
    listen(serverSocket, 3);

    printf("Waiting for incoming connections...\n");

    while (1) {
        // Accept a new connection
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize)) == INVALID_SOCKET) {
            printf("Accept failed with error code : %d", WSAGetLastError());
            continue;
        }
        char clientAddrStr[16];//In most of the OSs the value is 16
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientAddrStr, 16);//16 like above
        printf("Connection accepted from %s:%d\n", clientAddrStr, ntohs(clientAddr.sin_port));

        // Create a new thread for the client
        if (numClients < MAX_CLIENTS) {
            clientSockets[numClients] = clientSocket;
            clientThreads[numClients] = CreateThread(NULL, 0, clientThread, &clientSockets[numClients], 0, NULL);
            numClients++;
        }
        else {
            printf("Max number of clients reached. Closing connection...\n");
            closesocket(clientSocket);
        }
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}