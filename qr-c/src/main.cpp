#include <iostream>
#include <thread>
#include <algorithm>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <semaphore>
#include "serial.h"
#include "logger.h"
#include "socket.h"

#define DEFAULT_TIMEOUT_MS 10000 

int pipe_fds[2];
std::string command;
std::binary_semaphore sem(1);  // Protects concurrent serial reads

// Get current Unix timestamp
int timestamp() {
    std::time_t now = std::time(nullptr);
    return static_cast<int>(now);
}

// Read from serial with timeout; can be interrupted by pipe signal
std::string readSerial(int timeout) {
    struct pollfd fds[2];
    fds[0].fd = default_fd; // serial port fd
    fds[0].events = POLLIN;
    fds[1].fd = pipe_fds[0]; // pipe read end
    fds[1].events = POLLIN;

    int ret = poll(fds, 2, timeout); // 10s timeout
    
    if (ret < 0) {
        log_error(std::string("") + "[ERROR] poll failed: " + strerror(errno));
        return "ERROR";
    }
    else if (ret == 0) {
        log(std::string("") + "[INFO] Serial timed out");
        return "TIMEOUT";
    }

    // Check if read was signaled to stop via pipe
    if (fds[1].revents & POLLIN) {
        char dummy;
        read(pipe_fds[0], &dummy, 1);
        log(std::string("") + "[INFO] STOP received, cancelling read");
        return "STOPPED";
    }
    
    // Read serial data
    if (fds[0].revents & POLLIN) {
        std::string line = readLine();
        if (line.empty()) {
            log(std::string("") + "[INFO] No data received");
            return "NULL READING"; // should not happen
        }
        else {
            log(std::string("") + "[DATA] Received: " + line);
            return line;
        }
    }

    return "";
}

// Process socket commands: INIT, PING, START, STOP
void handleCommand(std::string& cmd) {
    log(std::string("") + ("[INFO] Received command: " + cmd));
    std::string response = R"({"status":"ERROR","message":"Unknown command error")";

    // Convert to uppercase for case-insensitive comparison
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c){return std::toupper(c); });

    if (!cmd.compare("INIT")) {
        // Initialize serial connection
        if (!initSerial()) {
            log(std::string("") + "[WARNING] Could not connect to serial device");
            response = R"({"status":"ERROR","message":"Could not connect to serial device")";
        }
        else {
            response = R"({"status":"OK","message":"Connected to serial device")";
        }
    }
    else if (!cmd.compare("PING")) {
        // Simple health check
        response = R"({"status":"PONG")";
    }
    else if (cmd.starts_with("START")) {
        // Start serial read with optional timeout parameter
        int timeout = 0; // default

        char buffer[64];
        // Clear pipe before starting read
        while (true) {
            ssize_t n = read(pipe_fds[0], buffer, sizeof(buffer));
            if (n <= 0) break; // pipe empty
        }

        // Parse timeout from command (e.g., "START 5000")
        if ( cmd.length() == strlen("START")) {
            timeout = DEFAULT_TIMEOUT_MS;
        }
        else if (cmd[std::strlen("START")] == ' ') {
            std::string timeout_command = cmd.substr(strlen("START "));
            try {
                timeout = std::stoi(timeout_command);
            } catch(const std::exception& e) {
                response = R"({"status":"ERROR","message":"Error reading timeout value for START command")";
            }
        }
        else response = R"({"status":"ERROR","message":"Unknown START command")";

        if (timeout != 0) {
            // Acquire semaphore to ensure no concurrent reads
            if (sem.try_acquire()) {
                std::string message = readSerial(timeout);
                sem.release();
                log(std::string("") + "[DATA] Received: " + message);
                response = R"({"qr-data":{"code":")" + message;
            } else {
                response = R"({"status":"ERROR","message":"Cannot start serial read while already reading")";
            }

        } else {
            response = R"({"status":"ERROR","message":"Cannot have timeout of 0 for START command")";
        }
    }
    else if (!cmd.compare("STOP")) {
        // Stop ongoing serial read by writing to pipe
        if (sem.try_acquire()) {
            sem.release();
            response = R"({"status":"ERROR","message":"No ongoing serial read to stop")";
        } else {
            // Signal poll() to wake up
            char dummy;
            write(pipe_fds[1], &dummy, 1); // wakes up poll() immediately
            response = R"({"status":"OK","message":"Stopped serial read")";
        }
    }
    else {
        response = R"({"status":"ERROR","message":"Unknown command")";
    }

    // Append timestamp to response
    response += R"(, "ts":)" + std::to_string(timestamp()) + "}";
    socketSendResponse(response);
}


int main() {
    // Create pipe for signaling read cancellation
    pipe(pipe_fds); // pipe_fds[0] = read end, pipe_fds[1] = write end
    fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);

    while (true)
    {
        // Initialize socket and listen for connections
        initSocket();

        socketListen();

        while (true) {
            socketWaitConnection();

            log(std::string("") + "[INFO] Connected to container B");

            // Handle incoming commands from client
            while (true) {
                
                command = socketReadMessage();
                if (command == "") break;
                // Process command in separate thread
                std::thread(handleCommand, std::ref(command)).detach();

            }
        }
        socketClose();
    }

    closeSerial();
    return 0;
}