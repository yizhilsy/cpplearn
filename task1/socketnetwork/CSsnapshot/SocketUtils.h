#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>

void printSockAddrIn(const sockaddr_in& addr) {
    std::cout << "=====address info=====" << std::endl;
    // 打印 sin_family
    if (addr.sin_family == AF_INET) {
        std::cout << "sin_family: " << "AF_INET (IPv4)" << std::endl;
    } 
    else if (addr.sin_family == AF_INET6) {
        std::cout << "sin_family: " << "AF_INET6 (IPv6)" << std::endl;
    } 
    else {
        std::cout << "sin_family: " << "Unknown (" << addr.sin_family << ")" << std::endl;
    }
    // 打印 sin_addr（需要使用 inet_ntoa 转换为点分十进制）
    std::cout << "sin_addr: " << inet_ntoa(addr.sin_addr) << std::endl;
    // 打印 sin_port（需要使用 ntohs 转换为主机字节序）
    std::cout << "sin_port: " << ntohs(addr.sin_port) << std::endl;
}