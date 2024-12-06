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

struct URL {
    char host[LENGTH];      
    char file[LENGTH];     
    char resource[LENGTH];  
    char ip[LENGTH]; 
    char user[LENGTH];      
    char password[LENGTH];      
};

#endif