#include <iostream>
#include "tcp_implement.h"
#include "util.h"
int main()
{
    // 创建服务端存储日志的文件夹
    std::string log_directory = "server_log";
    create_directory(log_directory);

    Itcp_manager_implement tcp_manager;
    Ilisten_event *listen_event = new Ilisten_event_implement();
    tcp_manager.init(8);
    tcp_manager.listen(8080, listen_event, nullptr);
    // 主线程阻塞
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    // tcp_manager.release();
    return 0;
}