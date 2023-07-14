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
        //cout << "Ctrl+C (SIGINT) ignored." << endl;
        cout << "\033[2K\r";  // Apaga a linha atual
        cout.flush();  // Limpa o buffer de saída
    }
}

void receiveMessages(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            cout << "Server response: " << buffer << endl;
        } else if (bytesRead == 0) {
            cout << "Server closed the connection." << endl;
            break;
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
            std::cerr << "Failed to send message." << std::endl;
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

    cout << "Digite o comando desejado: /connect para se conectar ao servidor ou /quit para encerrar o programa" << endl;

    while (true) {
        string command;
        getline(cin, command);

        if (command == "/connect") {
            string local_ip;
            char ip_buffer[INET_ADDRSTRLEN];
            inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)); // Get local IP adress
            inet_ntop(AF_INET, &(serverAddr.sin_addr), ip_buffer, INET_ADDRSTRLEN); // Convert IP adress to string form
            local_ip = ip_buffer;

            // Connect to the server
            if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
                cerr << "Failed to connect to server." << endl;
                return 1;
            }

            cout << "Connected to the server." << endl;

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
        
                // Send message to server
                sendMessage(clientSocket, message);
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