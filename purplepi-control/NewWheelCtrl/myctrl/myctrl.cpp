#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>

#define DEFAULT_SERVER_IP "192.168.230.173"
#define SERVER_PORT 5001
#define MAX_BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];
    int bytesSent;
    std::string ipInput;
    std::string inputMessage;

    // 获取用户输入的服务器IP地址
    std::cout << "Enter server IP address (default: " << DEFAULT_SERVER_IP << "): ";
    std::getline(std::cin, ipInput);

    const char* serverIP = ipInput.empty() ? DEFAULT_SERVER_IP : ipInput.c_str();

    // 创建UDP套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Error: Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_aton(serverIP, &serverAddr.sin_addr) == 0) {
        std::cerr << "Error: Invalid IP address format" << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "UDP client started. Sending to " << serverIP << ":" << SERVER_PORT << std::endl;

    while (true) {
        std::cout << "Enter message to send (or 'exit' to quit): ";
        std::getline(std::cin, inputMessage);

        if (inputMessage == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        strncpy(buffer, inputMessage.c_str(), MAX_BUFFER_SIZE);
        buffer[MAX_BUFFER_SIZE - 1] = '\0'; // 确保结尾

        bytesSent = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (bytesSent == -1) {
            perror("Error: Failed to send data");
            break;
        }

        std::cout << "Sent message to server: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}
