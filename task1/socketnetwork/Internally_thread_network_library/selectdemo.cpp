#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <functional>

class NetworkLibrary {
public:
    using Callback = std::function<void(int)>;

    NetworkLibrary() : running(false) {}

    ~NetworkLibrary() {
        stop();
    }

    // 启动网络库
    void start() {
        running = true;
        workerThread = std::thread(&NetworkLibrary::eventLoop, this);
    }

    // 停止网络库
    void stop() {
        if (running) {
            running = false;
            if (workerThread.joinable()) {
                workerThread.join();
            }
        }
    }

    // 添加监听的 socket
    void addSocket(int fd, Callback readCallback, Callback writeCallback = nullptr) {
        std::lock_guard<std::mutex> lock(mutex);
        socketMap[fd] = {readCallback, writeCallback};
        if (fd > maxFd) maxFd = fd;
    }

    // 移除 socket
    void removeSocket(int fd) {
        std::lock_guard<std::mutex> lock(mutex);
        socketMap.erase(fd);
        close(fd);
    }

private:
    struct SocketCallbacks {
        Callback readCallback;
        Callback writeCallback;
    };

    std::map<int, SocketCallbacks> socketMap; // 管理所有的 socket
    std::mutex mutex;                         // 用于保护 socketMap 的线程安全
    std::thread workerThread;                 // 内部线程
    bool running;                             // 控制循环的运行状态
    int maxFd = 0;                            // 当前最大文件描述符

    void eventLoop() {
        while (running) {
            fd_set readSet, writeSet;
            FD_ZERO(&readSet);
            FD_ZERO(&writeSet);

            {
                std::lock_guard<std::mutex> lock(mutex);
                for (const auto &entry : socketMap) {
                    FD_SET(entry.first, &readSet);
                    if (entry.second.writeCallback) {
                        FD_SET(entry.first, &writeSet);
                    }
                }
            }

            timeval timeout = {1, 0}; // 超时时间：1秒
            int activity = select(maxFd + 1, &readSet, &writeSet, nullptr, &timeout);

            if (activity < 0 && running) {
                std::cerr << "Select error: " << strerror(errno) << std::endl;
                continue;
            }

            std::vector<int> toRemove;
            {
                std::lock_guard<std::mutex> lock(mutex);
                for (const auto &entry : socketMap) {
                    int fd = entry.first;
                    const auto &callbacks = entry.second;

                    if (FD_ISSET(fd, &readSet)) {
                        callbacks.readCallback(fd);
                    }
                    if (FD_ISSET(fd, &writeSet) && callbacks.writeCallback) {
                        callbacks.writeCallback(fd);
                    }
                }
            }
        }
    }
};

void handleRead(int fd) {
    char buffer[1024] = {0};
    int bytesRead = recv(fd, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::cout << "Received: " << buffer << std::endl;
    } else if (bytesRead == 0) {
        std::cout << "Client disconnected: " << fd << std::endl;
    } else {
        std::cerr << "Read error: " << strerror(errno) << std::endl;
    }
}

void handleAccept(int serverFd, NetworkLibrary &network) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, (sockaddr *)&clientAddr, &clientLen);
    if (clientFd >= 0) {
        std::cout << "New connection: " << clientFd << std::endl;
        network.addSocket(clientFd, handleRead);
    } else {
        std::cerr << "Accept error: " << strerror(errno) << std::endl;
    }
}

int main() {
    NetworkLibrary network;
    network.start();

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return -1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return -1;
    }

    if (listen(serverFd, 5) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        return -1;
    }

    std::cout << "Server listening on port 8080..." << std::endl;

    network.addSocket(serverFd, [&network, serverFd](int fd) { handleAccept(fd, network); });

    // 主线程继续运行其他逻辑
    while (true) {
        std:: cout << "Main thread is running..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    network.stop();
    close(serverFd);
    return 0;
}