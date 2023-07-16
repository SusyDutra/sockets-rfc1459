#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

const int MAX_BUFFER_SIZE = 4096;
const int MAX_ATTEMPTS = 5;
const int SERVER_PORT = 8080;

using namespace std;

struct Client {
    int socket;
    string channel;
    string nickname;
    string ipAddress;
};

mutex clientMutex;
map<int, Client> clients;
map<string, vector<int>> channels;

vector<string> split(const string& str, char delimiter);

void treatChannelName(string& newChannel){
    if(newChannel[0] != '#' && newChannel[0] != '&'){
        newChannel.insert(newChannel.begin(), '#');
    }

    for (size_t i = 0; i < newChannel.length(); i++) {
        if (newChannel[i] == ' ' || newChannel[i] == ',' || int(newChannel[i]) == 17) {
            newChannel.erase(i, 1);
            i--;  // Update index so that we don´t jump characters
        }
    }
}

void sendMessage(int clientSocket, const string& message) {
    size_t pos = 0;
    while (pos < message.length()) {
        string chunk = message.substr(pos, MAX_BUFFER_SIZE - 1);
        ssize_t bytesSent = send(clientSocket, chunk.c_str(), chunk.length(), 0);
        if (bytesSent == -1) {
            cerr << "Failed to send message." << endl;
            break;
        }
        pos += chunk.length();
    }
}

void listChannels(int clientSocket) {
    string message = "Existing channels: ";
    for (const auto& channelPair : channels) {
        const string& channelName = channelPair.first;
        message += channelName + "\n";
    }
    sendMessage(clientSocket, message);
}

void handleClient(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    int numAttempts = 0;

    while (numAttempts < MAX_ATTEMPTS) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0) {
            string message(buffer);
            if (message == "/quit") {
                // Close connection for "/quit" command
                break;
            }
            else if (message == "/ping") {
                // Respond with "pong" for "/ping" command
                sendMessage(clientSocket, "pong");
            }
            else {
                message = clients[clientSocket].nickname + ": " + message;
                // Broadcast message to clients in the same channel
                lock_guard<mutex> lock(clientMutex);
                string channel = clients[clientSocket].channel;
                for (int client : channels[channel]) {
                    size_t pos = 0;
                    while (pos < message.length()) {
                        string chunk = message.substr(pos, MAX_BUFFER_SIZE - 1);
                        ssize_t bytesSent = send(client, chunk.c_str(), chunk.length(), 0);
                        if (bytesSent == -1) {
                            cout << "Failed to send message." << endl;
                            break;
                        }
                        pos += chunk.length();
                        if (pos < message.length()) message = clients[clientSocket].nickname + ": " + message;
                    }
                }
                numAttempts = 0;  // Reset attempts
            }
        }
        else if (bytesRead == 0) {
            // Connection closed by client
            break;
        }
        else {
            // Error or timeout
            numAttempts++;
        }
    }

    lock_guard<mutex> lock(clientMutex);
    if (clients.count(clientSocket) > 0) {
        string channel = clients[clientSocket].channel;
        auto it = find(channels[channel].begin(), channels[channel].end(), clientSocket);
        if (it != channels[channel].end()) {
            channels[channel].erase(it);
        }
        clients.erase(clientSocket);
    }

    close(clientSocket);
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\033[2K\r";  // Apaga a linha atual
        cout.flush();  // Limpa o buffer de saída
        cout << "Server shutting down..." << endl;
        exit(0);
    }
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr;
    
    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Set up server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    
    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Failed to bind socket." << endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == -1) {
        cerr << "Failed to listen on socket." << endl;
        return 1;
    }
    
    signal(SIGINT, signalHandler);
    
    cout << "Server running. Listening on port " << SERVER_PORT << endl;
    
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        // Accept a new client connection
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        // Get client IP
        string clientIP = inet_ntoa(clientAddr.sin_addr);

        // Create a new client structure
        Client client;
        client.socket = clientSocket;
        client.ipAddress = clientIP;

        lock_guard<mutex> lock(clientMutex);
        clients[clientSocket] = client;

        // Start a new thread to handle the client
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    return 0;
}