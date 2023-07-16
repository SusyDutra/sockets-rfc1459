Segundo trabalho prático da disciplina SSC0142-Redes de Computadores

**IRC Chat - Readme**

This is a simple IRC (Internet Relay Chat) chat application that allows multiple clients to connect to a server and communicate with each other in different channels. The project includes both the server and client components.

**Project Members:**
- Guilherme Sousa Panza
- Susy da Costa Dutra

**System Requirements:**
- Operating System: Linux Mint 21 (or any other Linux distribution)
- Compiler: g++

**Instructions:**
1. Compile the server and client codes using the g++ compiler. Make sure to include the necessary libraries while compiling.
   ```bash
   g++ -o server server.cpp -lpthread
   g++ -o client client.cpp
   ```

2. Run the server in one terminal:
   ```bash
   ./server
   ```
   The server will start running and listening on port 8080.

3. Run as many clients as you want in separate terminals:
   ```bash
   ./client
   ```
   The client will prompt you to choose a nickname using the `/nickname` command. Enter a unique nickname to join the chat.

4. Once connected, the client can interact with the chat by sending messages and using various commands like `/join`, `/list`, `/kick`, `/mute`, `/unmute`, `/whois`, and `/invite`. 

**Available Commands:**

1. `/join <channel>`: Joins the specified channel. If the channel does not exist, it will be created.

2. `/list`: Lists all the existing channels.

3. `/quit`: Disconnects the client from the server and exits the client application.

4. `/kick <nickname>`: Only available for the admin user of the channel. Kicks the specified user from the channel.

5. `/mute <nickname>`: Only available for the admin user of the channel. Mutes the specified user in the channel.

6. `/unmute <nickname>`: Only available for the admin user of the channel. Unmutes the specified user in the channel.

7. `/whois <nickname>`: Only available for the admin user of the channel. Retrieves the IP address of the specified user.

8. `/invite <nickname> <channel>`: Only available for the admin user of the channel. Invites the specified user to the specified channel.

**Important Notes:**
- The server can handle multiple clients concurrently through multi-threading.
- The client can be used to connect to the server running on the same machine.
- The `/nickname` command is mandatory to choose a unique nickname before accessing other functionalities.
- When invited to a channel, the client will be prompted to accept or decline the invitation.
- Admin users can use `/kick`, `/mute`, `/unmute`, `/whois`, and `/invite` commands on other users.
- Regular users can use `/join` to join channels and send messages.

**Makefile:**

```make
all: server client

server: server.cpp
	g++ -o server server.cpp -lpthread

client: client.cpp
	g++ -o client client.cpp

clean:
	rm -f server client
```

To compile the project and create both the server and client executables, simply run:
```bash
make
```

To clean up the build files and executables, run:
```bash
make clean
```

**Enjoy chatting with your IRC Chat!**