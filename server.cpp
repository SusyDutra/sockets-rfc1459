#include <iostream>
#include <cstring>
#include <string>
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
    bool mute = false;
    string ipAddress;
};

mutex clientMutex;
map<int, Client> clients;
map<string, vector<int>> channels;

void printChannels(const map<string, vector<int>>& channels, const map<int, Client>& clients);

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
        message += channelName + ", ";
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
            } else if (message == "/ping") {
                // Respond with "pong" for "/ping" command
                sendMessage(clientSocket, "pong");
            } else if (message == "/list") {
                listChannels(clientSocket);
            } 
            else if (message.substr(0, 5) == "/join") {
                string channel = message.substr(6);
                treatChannelName(channel);

                // Join a channel
                if (channels[channel].empty()) {
                    // Create new channel if it doesn't exist
                    channels[channel] = vector<int>();
                    cout << "Channel " << channel << " created!" << endl;
                }

                string userNickname = clients[clientSocket].nickname;
                cout << userNickname << " joining channel: " << channel << endl;

                // Remove client from previous channel
                lock_guard<mutex> lock(clientMutex);
                if (clients.count(clientSocket) > 0) {
                    string previousChannel = clients[clientSocket].channel;
                    auto it = find(channels[previousChannel].begin(), channels[previousChannel].end(), clientSocket);
                    if (it != channels[previousChannel].end()) {
                        channels[previousChannel].erase(it);
                    }
                }

                // Add client to new channel
                clients[clientSocket].channel = channel;
                channels[channel].push_back(clientSocket);

                printChannels(channels, clients);
            }
            else if (message.substr(0, 9) == "/nickname") {
                clients[clientSocket].nickname = message.substr(10);
                cout << "New client: " << clients[clientSocket].nickname << endl;
            }
            else if (message.substr(0, 5) == "/kick") {
                string userChannel = clients[clientSocket].channel;
                if (channels[userChannel][0] != clientSocket) {
                    sendMessage(clientSocket, "Command valid only for admin user");
                    continue;
                }

                string kickedUser = message.substr(6);
                lock_guard<mutex> lock(clientMutex);

                // search the socket of the user the client wants to kick
                int kickedUserSocket = -1;
                for(int i = 0; i < channels[userChannel].size(); i++){
                    if(clients[channels[userChannel][i]].nickname == kickedUser){
                        kickedUserSocket = clients[channels[userChannel][i]].socket;
                    }
                }

                cout << kickedUser << endl;
                // search for that socket on the user's channel
                auto it = find(channels[userChannel].begin(), channels[userChannel].end(), kickedUserSocket);
                if (it != channels[userChannel].end()) {
                    channels[userChannel].erase(it);
                    cout << kickedUser<< endl;
                    
                    string message = "The user " + kickedUser + " has been banned!";
                    for (int client : channels[userChannel]){
                        cout << message << endl;
                        sendMessage(client, message);
                    }

                    sendMessage(kickedUserSocket, "");
                    close(kickedUserSocket);
                } else {
                    sendMessage(clientSocket, "User not found!");
                }
            }
            else if (message.substr(0, 5) == "/mute") {
                string userChannel = clients[clientSocket].channel;
                if (channels[userChannel][0] != clientSocket) {
                    sendMessage(clientSocket, "Command valid only for admin user");
                    continue;
                }

                string mutedUser = message.substr(6);
                lock_guard<mutex> lock(clientMutex);

                // search the socket of the user the client wants to mute
                int mutedUserSocket = -1;
                for(int i = 0; i < channels[userChannel].size(); i++){
                    if(clients[channels[userChannel][i]].nickname == mutedUser){
                        mutedUserSocket = clients[channels[userChannel][i]].socket;
                    }
                }

                // search for that socket on the user's channel
                auto it = find(channels[userChannel].begin(), channels[userChannel].end(), mutedUserSocket);
                if (it != channels[userChannel].end()) {
                    clients[*it].mute = true;
                    sendMessage(mutedUserSocket, "You have been muted :(");

                    string message = "User " + mutedUser + " has been muted!";
                    for (int client : channels[userChannel]) {
                        sendMessage(client, message);
                    }
                }
                else{
                    sendMessage(clientSocket, "User not found or already muted!");
                }
            }
            else if (message.substr(0, 7) == "/unmute") {
                string userChannel = clients[clientSocket].channel;
                if (channels[userChannel][0] != clientSocket) {
                    sendMessage(clientSocket, "Command valid only for admin user");
                    continue;
                }

                string unmutedUser = message.substr(8);
                lock_guard<mutex> lock(clientMutex);

                // search the socket of the user the client wants to unmute
                int unmutedUserSocket = -1;
                for(int i = 0; i < channels[userChannel].size(); i++){
                    if(clients[channels[userChannel][i]].nickname == unmutedUser){
                        unmutedUserSocket = clients[channels[userChannel][i]].socket;
                    }
                }

                // search for that socket on the user's channel
                auto it = find(channels[userChannel].begin(), channels[userChannel].end(), unmutedUserSocket);
                if (it != channels[userChannel].end()) {
                    clients[*it].mute = false;
                    sendMessage(unmutedUserSocket, "You are no longer muted :)");

                    string message = "User " + unmutedUser + " is no longer muted!";
                    for (int client : channels[userChannel]) {
                        sendMessage(client, message);
                    }
                } else {
                    sendMessage(clientSocket, "User not found or not muted!");
                }
            } else if (message.substr(0, 6) == "/whois"){
                string userChannel = clients[clientSocket].channel;
                if (channels[userChannel][0] != clientSocket) {
                    sendMessage(clientSocket, "Command valid only for admin user");
                    continue;
                }

                string wantedUser = message.substr(7);
                lock_guard<mutex> lock(clientMutex);

                // search the socket of the user the client wants to unmute
                int wantedUserSocket = -1;
                for(int i = 0; i < channels[userChannel].size(); i++){
                    if(clients[channels[userChannel][i]].nickname == wantedUser){
                        wantedUserSocket = clients[channels[userChannel][i]].socket;
                        break;
                    }
                }

                // search for that socket on the user's channel
                string userIp;
                auto it = find(channels[userChannel].begin(), channels[userChannel].end(), wantedUserSocket);
                if (it != channels[userChannel].end()) {
                    userIp = clients[*it].ipAddress;
                    string message = "The IP address of " + wantedUser + " is: " + userIp;
                    sendMessage(clientSocket, message);
                } else {
                    sendMessage(clientSocket, "User not found!");
                }

            }
            else {
                message = clients[clientSocket].nickname + ": " + message;
                if(!clients[clientSocket].mute) {
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
                }
                else {
                    string response = "message not sent, you are muted";
                    send(clientSocket, response.c_str(), response.length(), 0);
                }
                numAttempts = 0;  // Reset attempts
            }
        } else if (bytesRead == 0) {
            // Connection closed by client
            break;
        } else {
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

void printChannels(const map<string, vector<int>>& channels, const map<int, Client>& clients) {
    for (const auto& channelPair : channels) {
        const string& channel = channelPair.first;
        const vector<int>& clientSockets = channelPair.second;

        cout << "Channel: " << channel << endl;

        for (int clientSocket : clientSockets) {
            const Client& client = clients.at(clientSocket);

            cout << "Client Socket: " << client.socket << endl;
            cout << "Nickname: " << client.nickname << endl;
            cout << "Mute: " << (client.mute ? "True" : "False") << endl;

            cout << "-----" << endl;
        }

        cout << endl;
    }
}