#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <string>

extern int default_fd;

// Connect to the serial/PTY device specified with environment variables
bool initSerial();

// Read a newline-terminated line with timeout (ms)
// Returns empty string on timeout or error
std::string readLine();

bool closeSerial();

#endif // SERIAL_COMM_H