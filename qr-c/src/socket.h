#ifndef SOCKET_COMM_H
#define SOCKET_COMM_H

#include <string>

// Binds the Unix socket based on env variables
bool initSocket();

// Starts listening on the socket
bool socketListen();

// Waits for a client to connect to the socket
bool socketWaitConnection();

// Read any new message from client
std::string socketReadMessage();

// Send response to client
bool socketSendResponse(const std::string& response);

// Close the socket
bool socketClose();

#endif // SOCKET_COMM_H