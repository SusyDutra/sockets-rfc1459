#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <mutex>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

const int MAX_BUFFER_SIZE = 4096;
const int OTHER_USER_PORT = 8080;

int clientSocket;

using namespace std;

mutex inputMutex;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\033[2K\r";  // Apaga a linha atual
        cout.flush();  // Limpa o buffer de saída
        cout << "Do you want to quit already? :(" << endl;
        cout << "To do so, type /quit or press CTRL+D" << endl;
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

void receiveMessages(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            cout << buffer << endl;
        }
        else {
            cerr << "Error receiving response." << endl;
            break;
        }
    }
}

int main() {
    struct sockaddr_in otherUserAddr;
    
    // Create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Set up server address
    otherUserAddr.sin_family = AF_INET;
    otherUserAddr.sin_port = htons(OTHER_USER_PORT);

    signal(SIGINT, signalHandler);

    cout << "Welcome to IRC Chat!" << endl;
    cout << "Enter the desired command: /connect to connect to the server or /quit to terminate the program" << endl;

    while (true) {
        string command, userNickname;
        getline(cin, command);

        if (command == "/connect") {
            string local_ip;
            char ip_buffer[INET_ADDRSTRLEN];
            inet_pton(AF_INET, "127.0.0.1", &(otherUserAddr.sin_addr)); // Get local IP adress
            inet_ntop(AF_INET, &(otherUserAddr.sin_addr), ip_buffer, INET_ADDRSTRLEN); // Convert IP adress to string form
            local_ip = ip_buffer;

            // Connect to the server
            if (connect(clientSocket, (struct sockaddr *)&otherUserAddr, sizeof(otherUserAddr)) == -1) {
                cout << "Failed to connect to the other user." << endl;
                return 1;
            }

            socklen_t addrLen = sizeof(otherUserAddr);
            // Obtém as informações do socket associado ao descritor de arquivo
            if (getsockname(clientSocket, (struct sockaddr*)&otherUserAddr, &addrLen) == -1) {
                cout << "Failed to get socket information." << endl;
                return 1;
            }
            char ipBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(otherUserAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
            int port = ntohs(otherUserAddr.sin_port);

            cout << "Socket info: IP = " << ipBuffer << ", Port = " << port << endl;

            // Start receiving thread
            thread receiveThread(receiveMessages, clientSocket);

            cout << endl << "Hello, " << userNickname << "! Welcome to the chat." << endl;
            cout << "Enjoy chatting with other people." << endl << endl;

            while (true) {
                string message;
                getline(cin, message);

                if (message.empty() || message == "/quit") {
                    message = "/quit";
                    sendMessage(clientSocket, message);
                    break;
                }
                else{
                    sendMessage(clientSocket, message);
                }
            }

            // Close the connection
            close(clientSocket);

            break;
        } else if (command.empty() || command == "/quit") {
            break;
        }
    }

    return 0;
}
