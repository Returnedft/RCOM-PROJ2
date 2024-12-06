#include "download.h"

int parse_url(char *unparsedUrl, struct URL *url) {

    regex_t expression;
    regcomp(&expression, "/", 0);
    if (regexec(&expression, unparsedUrl, 0, NULL, 0)) return -1;

    regcomp(&expression, "@", 0);
    if (regexec(&expression, unparsedUrl, 0, NULL, 0) != 0) { //anonymous mode
        
        sscanf(unparsedUrl, "%*[^/]//%[^/]", url->host);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "password");

    } else { // user-password mode

        sscanf(unparsedUrl, "%*[^/]//%*[^@]@%[^/]", url->host);
        sscanf(unparsedUrl,"%*[^/]//%[^:/]", url->user);
        sscanf(unparsedUrl, "%*[^/]//%*[^:]:%[^@\n$]", url->password);
    }

    sscanf(unparsedUrl, "%*[^/]//%*[^/]/%s", url->resource);
    strcpy(url->file, strrchr(unparsedUrl, '/') + 1);

    struct hostent *h;
    if (strlen(url->host) == 0) return -1;
    if ((h = gethostbyname(url->host)) == NULL) {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    return !(strlen(url->host));
}

int message(int socket, char *msg) {
    char byte;
    int index = 0;
    int responseCode = 0;
    int multiline = 0; 
    char lineBuffer[LENGTH];
    memset(msg, 0, LENGTH);
    memset(lineBuffer, 0, LENGTH);

    while (1) {
        if (read(socket, &byte, 1) <= 0) {
            perror("Socket read error");
            exit(-1);
        }

        if (byte == '\n') { 
            lineBuffer[index] = '\0'; 
            index = 0;

            if (strlen(msg) == 0) {
               
                sscanf(lineBuffer, "%d", &responseCode);
                if (lineBuffer[3] == '-') {
                    multiline = 1; 
                }
            }

            strcat(msg, lineBuffer);
            strcat(msg, "\n");

            if (!multiline || (strncmp(lineBuffer, msg, 3) == 0 && lineBuffer[3] != '-')) {
                break; 
            }

        } else {
            
            if (index < LENGTH - 1) {
                lineBuffer[index++] = byte;
            } else {
                fprintf(stderr, "Line buffer overflow\n");
                exit(-1);
            }
        }
    }

    printf("Message: %s\n", msg);
    return responseCode;
}



int initializeSocket(char *ip, int port) {

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

int requestResource(int sockfd, char *resource) {

    char fileCommand[5 + strlen(resource) + 2], answer[LENGTH];

    sprintf(fileCommand, "retr %s\r\n", resource);

    write(sockfd, fileCommand, strlen(fileCommand));  

    return message(sockfd, answer);
}

int getResource(const int sockfd, const int sockfd2, char *filename) {
    FILE *fd = fopen(filename, "wb");
    if (fd == NULL) {
        printf("Error opening or creating file '%s'\n", filename);
        exit(-1);
    }

    char msg[LENGTH];
    int bytes;
    
    // Receive file data from the data connection
    do {
        bytes = read(sockfd2, msg, LENGTH);
        if (bytes > 0) {
            fwrite(msg, bytes, 1, fd);  // Write to file
        } else if (bytes == 0) {
            // End of data transfer
            break;
        } else {
            perror("Error reading from data connection");
            fclose(fd);
            return -1;
        }
    } while (bytes > 0);

    fclose(fd);

    // Check for proper completion of file transfer
    if (message(sockfd, msg) != 226) {
        printf("Error transferring file '%s'\n", filename);
        return -1;
    }

    return 0; // Success
}



int closeSockets(int sockfd, int sockfd2) {
    char answer[LENGTH];
    write(sockfd, "quit\r\n", 6);
    if (message(sockfd, answer) != 221) {
        printf("Error closing control connection\n");
        return -1;
    }
    close(sockfd);
    close(sockfd2);  // Close both the control and data connections
    return 0;
}




int enterPassiveMode(int sockfd, char *ip, int *port) {
    char msg[LENGTH];
    int ip1, ip2, ip3, ip4, port1, port2;

    // Send the PASV command to the server
    const char *pasvCommand = "pasv\r\n";
    if (write(sockfd, pasvCommand, strlen(pasvCommand)) < 0) {
        perror("Failed to send PASV command");
        return -1;
    }

    if (message(sockfd, msg) != 227) {
        fprintf(stderr, "Failed to enter passive mode. Server response: %s\n", msg);
        return -1;
    }

    if (sscanf(msg,  "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]", &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6) {
        fprintf(stderr, "Failed to parse PASV msg: %s\n", msg);
        return -1;
    }

    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    *port = port1*256 + port2; 

    printf("Entering passive mode. Server IP: %s, Port: %d\n", ip, *port);

    return 227;
}

int loginCmd(int sockfd ,char *user, char* pass){

    char msg[LENGTH];

    char userCommand[6+strlen(user)];
    char passCommand[6+strlen(pass)];

    sprintf(userCommand, "user %s\r\n", user);
    write(sockfd, userCommand, strlen(userCommand));
    if (message(sockfd, msg) != 331){
        exit(-1);
    }

    sprintf(passCommand, "pass %s\r\n", pass);
    write(sockfd, passCommand, strlen(passCommand));

    return message(sockfd,msg);
}


int main(int argc, char *argv[]) {

    if (argc!=2){
        printf("Wrong usage of program.");
        exit(-1);
    }

    struct URL url;

    if ((parse_url(argv[1],&url))) {
        exit(-1);
    }


    printf("Host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n", url.host, url.resource, url.file, url.user, url.password, url.ip);

    int sockfd;
    sockfd = initializeSocket(url.ip, 21);
    char msg[LENGTH];

    if (sockfd < 0 || message(sockfd,msg) != 220){
        printf("Failed to initialized socket to %s and port %d",url.ip, 21);
        exit(-1);
    }

    if (loginCmd(sockfd, url.user,url.password) != 230){
        printf("Failed to login\n");
        exit(-1);
    }

    int port;
    char ip[LENGTH];

    if (enterPassiveMode(sockfd, ip, &port) != 227) {
        printf("Passive mode failed\n");
        exit(-1);
    }

    int sockfd2 = initializeSocket(ip, port);

    if (sockfd2 < 0){
        printf("Failed to initialized socket to %s and port %d",ip, port);
        exit(-1);
    }

    int requestResponse = requestResource(sockfd, url.resource);
    if (requestResponse != 150 && requestResponse != 125) {
        printf("Unknown resouce '%s' in '%s:%d'\n", url.resource, ip, port);
        exit(-1);                                                                       
    }

    if (getResource(sockfd, sockfd2, url.file) != 226) {
        printf("Error transfering file '%s' from '%s:%d'\n", url.file, ip, port);
        exit(-1);
    }

    if (closeSockets(sockfd, sockfd2) != 0) {
        printf("Sockets close error\n");
        exit(-1);
    }



    return 0;


}