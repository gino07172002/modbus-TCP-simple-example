#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define TCP_MAX_ADU_LENGTH  260
#define MAX_READ_REGISTERS   125
#define PORT 5979
#define LENGTH_HEADER_INDEX 4
#define ID_HEADER_INDEX 6
#define FUNC_CODE_HEADER_INDEX 7
#define REGISTER_COUNT_INDEX 8


void setNonBlocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    uint8_t buffer[TCP_MAX_ADU_LENGTH] = {0};


    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    setNonBlocking(sock);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);


    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }


    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    uint8_t request[]={0,1,0,0,0,6,1,3,0,2,0,3}; //you can modify 0 2 0 3 part as other number ( 0 2 : start address , 0 3 : quanity of addresses (smaller than 125) )

    union tid {

        uint16_t output;
        uint8_t input[2];
    } tid;

    while (true) {
        std::cout << "======= modbus client select loop start =======" << std::endl;

        fd_set write_fds, read_fds;
        struct timeval timeout;

        // Clear and set the file descriptors for writing
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);

        timeout.tv_sec = 5;  // Timeout of 5 seconds for writing
        timeout.tv_usec = 0;

        // Check if the socket is ready for writing (send)
        if (select(sock + 1, NULL, &write_fds, NULL, &timeout) > 0) {
            std::cout << "try Sending ...  tid = " << (int)request[0] << "," << (int)request[1] << std::endl;
            for(int i=0;i<(int)(sizeof(request)/sizeof(request[0]));i++)
            {
                std::cout<<"["<<(int)request[i]<<"] ";
            }
            std::cout<<std::endl;

            send(sock, request, sizeof(request)/sizeof(request[0]), 0);
        } else {
            std::cout << "Send timeout, skipping..." << std::endl;
            continue;
        }

        // Clear and set the file descriptors for reading
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        timeout.tv_sec = 5;  // Timeout of 5 seconds for reading
        timeout.tv_usec = 0;

        // Check if the socket is ready for reading (receive)
        if (select(sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
            int valread = read(sock, buffer, TCP_MAX_ADU_LENGTH);
            if (valread > 0) {
                std::cout<<" -------------  "<<std::endl;

                std::cout << "     Receiving query size : " << valread <<" .... "<< std::endl;
                if ((buffer[0]^request[0])==0 && (buffer[1]^request[1])==0)
                {
                    std::cout << "Successful receive : " << std::endl;

                    for(int i=0;i< valread;i++)
                    {
                        std::cout<<"<"<<(int)buffer[i]<<"> ";
                    }
                    std::cout<<std::endl;


                }
                else
                {
                    std::cout<<" bad matching : "<<(int)buffer[0]<<" , "<< (int)request[0]<<" ,  buffer[1] : "<<(int)buffer[1]<<" , "<<(int)request[1]<<std::endl;
                }

                {
                    request[0] = tid.input[1];
                    request[1] = tid.input[0];
                    tid.output++;
                }

            } else if (valread == 0) {
                std::cout << "Server disconnected, reconnecting..." << std::endl;
                close(sock);
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    std::cout << "Socket creation error" << std::endl;
                    return -1;
                }
                setNonBlocking(sock);
                connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            }
        } else {
            std::cout << "Receive timeout or error." << std::endl;
        }

        std::cout << "======= modbus client select loop end =======" << std::endl;
        std::cout<<std::endl;
        std::cout<<std::endl;

        sleep(1); // you should always has sleep in select loop ( maybe in milliseconds in real application situation ) or it may overheat and shutdown your computer especially in raspberry pi

    }

    close(sock);
    return 0;
}
