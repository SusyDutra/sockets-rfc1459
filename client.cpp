#include <iostream>
#include <cstring>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

const int MAX_BUFFER_SIZE = 4096;
const int SERVER_PORT = 8080;

using namespace std;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\033[2K\r";  // Apaga a linha atual
        cout.flush();  // Limpa o buffer de saída
        cout << "Do you want to quit already? :(" << endl;
        cout << "To do so, type /quit or press CTRL+D" << endl;
    }
}

void receiveMessages(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            cout << buffer << endl;
        } else if (bytesRead == 0) {
            cout << "Server closed the connection." << endl;
            close(clientSocket);
            exit(0);
        } else {
            cerr << "Error receiving response." << endl;
            break;
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

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    
    // Create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Set up server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    signal(SIGINT, signalHandler);

    char buffer[MAX_BUFFER_SIZE];

    cout << "Welcome to IRC Chat!" << endl;
    cout << "Enter the desired command: /connect to connect to the server or /quit to terminate the program" << endl;

    while (true) {
        string command, userNickname;
        getline(cin, command);

        if (command == "/connect") {
            string local_ip;
            char ip_buffer[INET_ADDRSTRLEN];
            inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)); // Get local IP adress
            inet_ntop(AF_INET, &(serverAddr.sin_addr), ip_buffer, INET_ADDRSTRLEN); // Convert IP adress to string form
            local_ip = ip_buffer;

            // Connect to the server
            if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
                cout << "Failed to connect to server." << endl;
                return 1;
            }

            socklen_t addrLen = sizeof(serverAddr);
            // Obtém as informações do socket associado ao descritor de arquivo
            if (getsockname(clientSocket, (struct sockaddr*)&serverAddr, &addrLen) == -1) {
                cout << "Failed to get socket information." << endl;
                return 1;
            }
            char ipBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(serverAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
            int port = ntohs(serverAddr.sin_port);

            cout << "Socket info: IP = " << ipBuffer << ", Port = " << port << endl;

            cout << "You need to choose a nickname, use the command /nickname for that" << endl;
            getline(cin, command);
            while (command.substr(0, 9) != "/nickname" || command.length() < 10) {
                cout << "You need to choose a nickname before accessing other functionalities!" << endl;
                cout << "Choose one using the /nickname command" << endl << endl;
                getline(cin, command);
            }
            string userNickname = command.substr(10);
            sendMessage(clientSocket, command);

            cout << endl << "Hello, " << userNickname << "! Welcome to the chat." << endl;
            cout << "Enjoy chatting with other people." << endl << endl;
            cout << "To join a channel, use the /join command" << endl;
            cout << " Use /list to see all available channels" << endl;

            // Start receiving thread
            thread receiveThread(receiveMessages, clientSocket);
    
            while (true) {
                string message;
                getline(cin, message);

                // Check for command input
                if (message.empty() || message == "/quit") {
                    message = "/quit";
                    sendMessage(clientSocket, message);
                    break;
                }
                else if (message.substr(0, 5) == "/join") {
                    if (message.length() < 6) cout << "Incomplete command!" << endl;
                    else sendMessage(clientSocket, message);
                }
                else if (message.substr(0, 5) == "/kick") {
                    if (message.length() < 6) cout << "Incomplete command!" << endl;
                    else if (message.substr(6) == userNickname) cout << "The command cannot be used on yourself!" << endl;
                    else sendMessage(clientSocket, message);
                }
                else if (message.substr(0, 5) == "/mute") {
                    if (message.length() < 6) cout << "Incomplete command!" << endl;
                    else if (message.substr(6) == userNickname) cout << "The command cannot be used on yourself!" << endl;
                    else sendMessage(clientSocket, message);
                }
                else if (message.substr(0, 7) == "/unmute") {
                    if (message.length() < 8) cout << "Incomplete command!" << endl;
                    else if (message.substr(8) == userNickname) cout << "The command cannot be used on yourself!" << endl;
                    else sendMessage(clientSocket, message);
                }
                else if (message.substr(0, 6) == "/whois") {
                    if (message.length() < 7) cout << "Incomplete command!" << endl;
                    else if (message.substr(7) == userNickname) cout << "The command cannot be used on yourself!" << endl;
                    else sendMessage(clientSocket, message);
                } 
                else {
                    // Send message to server
                    sendMessage(clientSocket, message);
                }
            }

            // Join the receive thread to ensure it has finished execution
            receiveThread.join();

            // Close the connection
            close(clientSocket);
        
            break;
        } else if (command.empty() || command == "/quit") {
            break;
        }
    }

    return 0;
}
