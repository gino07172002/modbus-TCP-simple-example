#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5979
#define MAX_CLIENTS 10
#define MAX_READ_REGISTERS 125
#define TCP_MAX_ADU_LENGTH 256
#define RESPONSE_HEADER_LENGTH 7

#define LENGTH_HEADER_INDEX 4
#define ID_HEADER_INDEX 6
#define FUNC_CODE_HEADER_INDEX 7
#define REGISTER_COUNT_INDEX 8

// Set a socket to non-blocking mode
void setNonBlocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, activity, valread, sd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    uint8_t buffer[TCP_MAX_ADU_LENGTH] = {0};

    fd_set readfds;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking
    setNonBlocking(server_fd);

    // Set up address and bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize client sockets to 0, indicating no connection
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    uint8_t response[TCP_MAX_ADU_LENGTH]={0};
    uint8_t registers[] = {0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10};  // 6 register with value 5, 6, 7, 8, 9 ,10

    union trans {
        uint8_t uint8[2];
        uint16_t uint16;
    };

    std::cout << "Server is running and waiting for connections..." << std::endl;

    while (true) {
        std::cout << "======= modbus server select loop start =======" << std::endl;

        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to the set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add all client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on one of the sockets (timeout is NULL, so wait indefinitely)
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            std::cerr << "select error" << std::endl;
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(address.sin_port);

            std::cout << "Connection accepted from " << client_ip << ":" << client_port << std::endl;

            // Set the new socket to non-blocking
            setNonBlocking(new_socket);

            // Add the new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Handle IO operations for each client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                valread = read(sd, buffer, TCP_MAX_ADU_LENGTH);
                if (valread > 0) {
                    response[0] = buffer[0];
                    response[1] = buffer[1];


                    response[ID_HEADER_INDEX] = buffer[ID_HEADER_INDEX];
                    response[FUNC_CODE_HEADER_INDEX] = buffer[FUNC_CODE_HEADER_INDEX];


                    std::cout << "tid: " << (int)buffer[0] << "," << (int)buffer[1] << std::endl;

                    std::cout << "request size should be: " << (6 + buffer[5]) << " valread = " << valread << std::endl;
                    // valread may be longer than real data , use ( 6 + buffer ) instead

                    std::cout<<" getting request : "<<std::endl;
                    for(int i=0;i<  (6 + buffer[5]) ;i++)
                    {
                        std::cout<<"["<<(int)buffer[i]<<"] ";
                    }
                    union trans temp;
                    temp.uint8[0] = buffer[9];
                    temp.uint8[1] = buffer[8];

                    int start = temp.uint16;

                    temp.uint8[0] = buffer[11];
                    temp.uint8[1] = buffer[10];
                    int quantity = temp.uint16;

                    if (quantity < MAX_READ_REGISTERS) {
                        std::cout << std::endl;
                        std::cout << "start: " << start << ", quantity: " << quantity <<" output should be : ";
                        response[REGISTER_COUNT_INDEX]=quantity * 2;

                        for (int i = start; i < start + quantity * 2; i++) {
                            std::cout << (int)registers[i] << " ";
                            response[REGISTER_COUNT_INDEX+1+i-start]=registers[i];
                        }
                        std::cout << std::endl;

                        union trans length;
                        length.uint16= 3 + quantity * 2;
                        response[LENGTH_HEADER_INDEX] = length.uint8[1];
                        response[LENGTH_HEADER_INDEX+1] = length.uint8[0];



                    }

                    std::cout<<" -------------  "<<std::endl;

                    std::cout<<" response : "<<std::endl;
                    for(int i=0;i< RESPONSE_HEADER_LENGTH + 2 + quantity * 2;i++)
                    {
                        std::cout<<"<"<<(int)response[i]<<"> ";
                    }
                    std::cout<<std::endl;
                    send(sd, response, RESPONSE_HEADER_LENGTH + 2 + quantity * 2, 0);
                } else if (valread == 0) {
                    // Client disconnected
                    std::cout << "Client disconnected" << std::endl;
                    close(sd);
                    client_sockets[i] = 0;
                }
            }
        }
        std::cout << "======= modbus server select loop end =======" << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;

        usleep(20000); // you should always has sleep in select loop ( maybe in milliseconds in real application situation ) or it may overheat and shutdown your computer especially in raspberry pi
    }

    close(server_fd);
    return 0;
}
