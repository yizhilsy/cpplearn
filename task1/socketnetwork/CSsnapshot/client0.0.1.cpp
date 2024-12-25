#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "SocketException.h"

#define SERVER_IP "127.0.0.1"  // 服务器 IP 地址
#define SERVER_PORT 8011       // 服务器端口号
#define BUFFER_SIZE 4096       // 缓冲区大小

int main() {
    int client_fd;
    sockaddr_in server_address;
    int server_address_size = sizeof(server_address);
    char buffer[BUFFER_SIZE + 1] = {0};

    // 创建tcp客户端套接字
    try {
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd < 0) {
            throw SocketException("create tcp type socket failed");
        }
        else {
            std::cout << "create tcp type socket success, client_fd is: " << client_fd << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址信息
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // 将字符串形式的 IP 转换为网络字节序的二进制形式
    try {
        int transfer_inet_pton_result = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
        if (transfer_inet_pton_result <= 0) {
            throw SocketException("Invalid address/ Address not supported");
        }
        else {
            std::cout << "transfer inet_pton success" << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 连接到服务器
    try {
        int connect_result = connect(client_fd, (struct sockaddr*)&server_address, server_address_size);
        if (connect_result == -1) {
            throw SocketException("Connection to the server failed", client_fd);
        }
        else {
            std::cout << "client socket_fd: " << client_fd << " connected to the server at " << SERVER_IP << ":" << SERVER_PORT << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "client_socket_fd: " << e.get_socket_fd() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 向服务器发送消息
    std::string message = "Hello, Server!";
    send(client_fd, message.c_str(), message.size(), 0);
    std::cout << "Message sent to the server: " << message << std::endl;

    // 接收服务器响应
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::cout << "Message received from server: " << buffer << std::endl;
    } else {
        std::cout << "No response from server or connection closed" << std::endl;
    }

    // 关闭套接字
    close(client_fd);
    return 0;
}