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

// Timestamp helper
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

// Logging helper
void log(const std::string& msg) {
    std::cout << "[" << timestamp() << "] " << msg << std::endl;
}

// Logging helper for error
void log_error(const std::string& msg) {
    std::cerr << "[" << timestamp() << "] " << msg << std::endl;
}