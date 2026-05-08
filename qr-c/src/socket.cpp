#include "socket.h"
#include "logger.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

#define DEFAULT_BUFFER_SIZE 1024

const char* socket_path;
std::vector<char> buffer;

int default_fd;

std::atomic<bool> running_read(false);
std::atomic<bool> stop_requested(false);

int server_fd, client_fd;

std::string current_serial_port = "/dev/ttyS1";
int baud_rate = 9600;

// Send JSON response to connected client
bool socketSendResponse(const std::string& response) {
    ssize_t bytes = send(client_fd, response.c_str(), response.size(), 0);

    if (bytes <= 0) {
        log_error("[ERROR] Error sending response");
        perror("listen");
        return 0;
    }
    return 1;
}

// Create Unix socket and bind to path from environment
bool initSocket() {
    struct sockaddr_un addr;

    socket_path = std::getenv("SOCKET_PATH");
    const char* buffer_size_string = std::getenv("BUFFER_SIZE");
    int buffer_size = buffer_size_string ? std::atoi(buffer_size_string) : DEFAULT_BUFFER_SIZE;

    buffer.resize(buffer_size);

    // Create socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 0;
    }

    unlink(socket_path);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 0;
    }

    return 1;
}

// Listen for incoming client connections
bool socketListen(){
    // Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 0;
    }
    return 1;
}

// Accept an incoming client connection
bool socketWaitConnection(){
    client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
        perror("accept");
        return false;
    }
    return true;
}

// Receive message from connected client
std::string socketReadMessage() {
    memset(buffer.data(), 0, buffer.size());
    ssize_t bytes = recv(client_fd, buffer.data(), buffer.size() - 1, 0);
    
    if (bytes <= 0) {
        log_error("[ERROR] Client disconnected.");
        close(client_fd);
        return "";
    }

    return std::string(buffer.data());
}

// Close socket and clean up
bool socketClose() {
    close(server_fd);
    unlink(socket_path);
    return 1;
}

// Test function for socket communication
int testSocket() {
    // if (env_serial) current_serial_port = env_serial;
    // if (env_baud) baud_rate = std::atoi(env_baud);

    // log("Starting QR Serial Service...");
    // log("Serial Port: " + current_serial_port);
    // log("Baud Rate: " + std::to_string(baud_rate));

    initSocket();

    socketListen();


    log("Unix socket server listening at " + std::string(socket_path));

    while (true) {
        socketWaitConnection();

        log("Container B connected.");

        while (true) {
            
            std::string command = socketReadMessage();
            if (command == "") break;

        }
    }
    socketClose();

    return 0;
}