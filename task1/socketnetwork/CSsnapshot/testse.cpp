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
int main()
{
    // try {
    //     std::string str = "test exception";
    //     throw SocketException(str);
    // }
    // catch (const SocketException& e) {
    //     std::cout << e.what() << std::endl;
    // }
    sockaddr_in server_address;
    std:: cout << sizeof(server_address) << std::endl;

    return 0;
}