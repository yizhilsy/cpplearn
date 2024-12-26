#include <iostream>
#include <thread>
#include <chrono>
#include "tcp_implement.h"
int main()
{
    Itcp_manager_implement tcp_manager;
    tcp_manager.init(1);
    int32_t client_fd = tcp_manager.connect("127.0.0.1", 8080, nullptr, nullptr);
    int ct = 1;
    while (ct <= 10) {
        tcp_manager.send(client_fd, "hello, it's my life, amazing!", 29);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ct++;
    }

    std::cout << "send done!" << std::endl;
    close(client_fd);
    return 0;
}