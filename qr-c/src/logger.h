#ifndef LOGGER_H
#define LOGGER_H

#include <string>

// Write a message to stdout with a timestamp
void log(const std::string& msg);

// Write a message to stderr with a timestamp
void log_error(const std::string& msg);

#endif // LOGGER_H