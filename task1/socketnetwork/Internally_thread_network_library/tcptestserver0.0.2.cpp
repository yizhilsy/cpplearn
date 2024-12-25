#include <iostream>
#include "tcp_implement2.h"
int main()
{
    Itcp_manager_implement tcp_manager;
    Ilisten_event *listen_event = new Ilisten_event_implement();
    tcp_manager.init(4);
    tcp_manager.listen(8080, listen_event, nullptr);
    // 主线程阻塞
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    // tcp_manager.release();
    return 0;
}