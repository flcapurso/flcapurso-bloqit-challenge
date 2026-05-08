#include "serial.h"
#include "logger.h"
#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <linux/serial.h>

// ioctl codes for termios2 (arbitrary baud rate support)
#ifndef TCGETS2
#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#endif
#ifndef TCSETS2
#define TCSETS2 _IOW('T', 0x2B, struct termios2)
#endif

// Open serial/PTY device in non-blocking mode
int openPort(const char* path) {
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        log_error(std::string("") + "[ERROR] Could not open " + path + ": " + strerror(errno));
    }
    return fd;
}

// Check if fd is a real serial device (vs PTY)
bool isRealSerialDevice(int fd) {
    struct serial_struct serinfo;
    return ioctl(fd, TIOCGSERIAL, &serinfo) == 0;
}

// Configure serial port: baud rate, 8N1, raw mode, non-blocking
bool configurePort(int fd, const char* baudRate) {
    struct termios2 tty;
    int baudRate_value;

    if (ioctl(fd, TCGETS2, &tty) != 0)
    {
        log_error(std::string("") + "[ERROR] TCGETS2 failed: " + strerror(errno));
        return false;
    }

    try {
        baudRate_value = std::stoi(baudRate);
    } catch (...) {
        log_error(std::string("") + "[ERROR] Invalid BAUD_RATE env value");
        return false;
    }

    // ===== Control flags =====
    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= BOTHER;   // enable arbitrary baud mode

    tty.c_ispeed = baudRate_value;
    tty.c_ospeed = baudRate_value;

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // Enable receiver, ignore modem control lines, disable flow control
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;

    // Raw mode: no echo, no canonical input
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    // Disable all software flow control and special character processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Non-blocking read with small timeout
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;

    if (ioctl(fd, TCSETS2, &tty) != 0)
    {
        log_error(std::string("") + "[ERROR] TCSETS2 failed: " + strerror(errno));
        return false;
    }

    return true;
}

// Initialize serial connection from environment variables
bool initSerial() {
    // Opening serial port based on ENV variable
    const char* port = std::getenv("SERIAL_PORT");
    if (!port) port = "/tmp/ttyS1";

    default_fd = openPort(port);
    if (default_fd < 0) return false;

    // Configuring serial port parameters if it's not virtual (PTY)
    if (isRealSerialDevice(default_fd)) {
        const char* baudRate = std::getenv("BAUD_RATE");
        if (!baudRate) baudRate = "9600"; // default

        log(std::string("") + "[INFO] Real serial device detected, configuring port with baud rate: " + baudRate);

        if (!configurePort(default_fd, baudRate)) return false;
    } else {
        log(std::string("") + "[INFO] PTY device detected, port configuration not required");
    }
    return true;
}

// Read one newline-terminated line from serial port
std::string readLine() {
    std::string accumulator;
    char buf[256];

    while (true) {
        int n = read(default_fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EAGAIN) continue; // no data yet, try again
            log_error(std::string("") + "[ERROR] read failed: " + strerror(errno));
            return "";
        }
        if (n == 0) {
            // EOF — device disconnected or closed
            log_error(std::string("") + "[ERROR] Device disconnected");
            return "";
        };

        accumulator.append(buf, n);

        // Check for complete line (newline terminated)
        size_t pos = accumulator.find('\n');
        if (pos != std::string::npos) {
            std::string line = accumulator.substr(0, pos);
            // Strip \r if present (common with serial ports)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }

        // Prevent buffer overflow from malformed input
        if (accumulator.size() > 1024) {
            log_error(std::string("") + "[WARN] Buffer too large, discarding");
            accumulator.clear();
        }
    }
}

// Close serial connection
bool closeSerial() {
    close(default_fd);
    return true;
}

// Test function for serial communication
int test_serial() {
    const char* port = std::getenv("SERIAL_PORT");
    if (!port) port = "/tmp/ttyS1";

    log(std::string("") + "[INFO] Opening port: " + port);

    int fd = openPort(port);
    if (fd < 0) return 1;
    if (isRealSerialDevice(fd)) {
        log(std::string("") + "[INFO] Real serial device detected, configuring port...");
        if (!configurePort(fd, "9600")) return 1;
    } else {
        log(std::string("") + "[INFO] PTY device detected, skipping port configuration...");
    }

    log(std::string("") + "[INFO] Waiting for data (write to /tmp/ttyS2 to test)...");

    while (true) {
        std::string line = readLine(); // 5 second timeout

        if (line.empty()) {
            log(std::string("") + "[INFO] No data received (timeout), still waiting...");
            continue;
        }

        log(std::string("") + "[DATA] Received: " + line);
    }

    close(fd);
    return 0;
}