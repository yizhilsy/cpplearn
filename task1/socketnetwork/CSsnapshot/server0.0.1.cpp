// demo echo回显服务器
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include "SocketException.h"
#include "SocketUtils.h"

#define BUFFER_SIZE 4096   // 指定缓冲区大小
#define DEFAULT_PORT 8011    // 指定服务器监听的端口为8011
#define MAXLINK 128    // 指定服务器的最大连接数

// 结束服务器监听端口的函数
// void stopServerRunning(int p);

int main()
{   
    // server_fd是服务器的socket描述符
    int server_fd, client_socket;
    // sockaddr_in是存储 网络协议, 网络地址, 端口信息的类
    sockaddr_in server_address;
    sockaddr_in client_address;
    int addrlen = sizeof(client_address);
    // 声明缓冲区空间4KB
    char buffer[BUFFER_SIZE + 1] = {0};

    // 1. 创建服务器端的socket套接字
    try {
        // 声明创建使用tcp协议的套接字
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw SocketException("create tcp type socket failed");
        }
        else {
            std::cout << "create tcp type socket success, server_fd is: " << server_fd << std::endl;
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

    // 赋值给server_address变量，绑定服务器地址和要监听的端口
    server_address.sin_family = AF_INET;
    // 声明监听服务器上所有的网络接口
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(DEFAULT_PORT);

    // 2. 将1中创建的socket套接字绑定到设置的server_address变量
    try {
        int bind_result = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
        if (bind_result == -1) {
            throw SocketException("socket bind server address failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " socket bind server address success" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "socket_fd: " << e.get_socket_fd() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 3. 监听连接，设置最大允许连接数为MAXLINK
    try {
        int listen_result = listen(server_fd, MAXLINK);
        if (listen_result == -1) {
            throw SocketException("socket listen failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " listen success" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "socket_fd: " << e.get_socket_fd() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 监听建立连接成功
    std::cout << "Server is listening on port " << DEFAULT_PORT << "...\n";

    // 4. 接受客户端连接
    try {
        client_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&addrlen);
        if (client_socket == -1) {
            throw SocketException("accept client connection failed", server_fd);
        }
        else {
            std::cout << "accept client connection success, client socket_fd: " << client_socket << " ,below is client address" << std::endl;
            printSockAddrIn(client_address);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "socket_fd: " << e.get_socket_fd() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Client connected." << std::endl;

    // 服务器功能: 读取客户端消息并回显

    // 读入客户端发送的数据到缓冲区中
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    buffer[bytes_read] = '\0';

    std::cout << "Received: " << buffer;
    send(client_socket, buffer, bytes_read, 0);
    memset(buffer, 0, BUFFER_SIZE);

    close(client_socket);
    close(server_fd);
    return 0;
}

// void stopServerRunning(int p) {
//     close(sockfd);
//     std::cout << "Close Server!" << std::endl;
// }