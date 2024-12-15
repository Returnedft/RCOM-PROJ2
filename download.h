#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <termios.h>

#define LENGTH 500

//Parsed URL
struct URL {
    char host[LENGTH];      
    char file[LENGTH];     
    char resource[LENGTH];  
    char ip[LENGTH]; 
    char user[LENGTH];      
    char password[LENGTH];      
};

// Parses the "unparsedUrl" into the struct URL
// Returns -1 or 1 if there is an error, 0 otherwise
int parse_url(char *unparsedUrl, struct URL *url);

// Reads the responses from the server throught the "socket" and writes into the "msg"
// Returns the response code
int message(int socket, char *msg);

// Creates a socket based on the server ip and port
// Returns the socket file descriptor
int initializeSocket(char *ip, int port);

// Requests the file with the name "filename" from the server
// Returns the server response code of the operation
int requestFile(int sockfd, char *filename);

// Reads the file with the name "filename" from the server and downloads it
// Returns -1 if there is an error, 226 otherwise (response code if the operation happens successfully)
int getFile(const int sockfd, const int sockfd2, char *filename);

// Closes both socket connections
// Returns -1 if there is an error, 0 otherwise
int closeSockets(int sockfd, int sockfd2);

// Enters the passive mode
// Returns -1 if there is an error, 227 otherwise (response code if the operation happens successfully)
int enterPassiveMode(int sockfd, char *ip, int *port);

// Authenticates the user with username "user" and password "password"
// Returns the server response code of the operation
int loginCmd(int sockfd ,char *user, char* password);

#endif