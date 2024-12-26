// 第一版设计的tcp网络库的实现文件，现已废弃，未处理报错，请移步第二版引入内部线程的tcp网络库实现文件
#include <iostream>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <random>
#include <unordered_set>
#include "tcp_interface.h"
#include "SocketException.h"
#include "SocketUtils.h"
#include "ThreadPool.h"
#include "lru_interface.h"

#define FD_SETSIZE 1024 // 设置select模型中描述符集合的大小

// 线程池中执行的任务函数声明
void READCLIENTREQUEST(int client_fd, std::vector<char*>& buffer_vector, std::mutex* buffer_locks, Ilru_mgr<int, sockaddr_in>* client_fd_address_lru, fd_set& allfds);

// tcp_interface中的抽象基类接口的派生类实现
// 事件类
class Ilisten_event_implement : public Ilisten_event 
{
public:
    void on_accept(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort) override;
	void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
	// void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) override;
	virtual ~Ilisten_event_implement() override = default;
};

// class Iconnect_event_implement : public Iconnect_event
// {
// public:
// 	void on_connect(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
// 	void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
// 	void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) override;
// 	virtual ~Iconnect_event_implement() override = default;
// };

// class Ipack_parser_implement : public Ipack_parser
// {
// public:
// 	int32_t on_parse(const void* pData,uint32_t dwDataSize) override;
// 	virtual ~Ipack_parser_implement() override = default;
// };

class Itcp_manager_implement : public Itcp_manager
{
private:
    // 内部线程
    std::thread workerThread;  // 内部主线程，可以分发任务到线程池中
    bool running;              // 控制事件循环的运行状态

    // 与服务器端相关的成员变量
    Ilisten_event* listen_event;                        // 服务器监听事件回调函数
                                                        // 客户端连接，发送数据事件回调函数
    int server_fd;                                      // 服务器套接字描述符
    sockaddr_in server_address;                         // 服务器地址
    Ilru_mgr<int, sockaddr_in>* client_fd_address_lru;  // 客户端socket为键，客户端的发送地址为值的lru缓存基类指针
    Ilru_event<int, sockaddr_in> *lru_event;            // lru事件抽象基类指针
    fd_set readfds, allfds;                             // select模型中的描述符集合

    // 与线程池相关的成员变量
    ThreadPool thread_pool;                             // 线程池类
    std::vector<char*> buffer_vector;                   // 指向堆区缓冲区的指针向量，会被线程池内的工作线程调用
    std::mutex* buffer_locks;                           // buffer锁向量，保护buffer_vector中每个buffer指针指向的缓冲区

    // 与客户端相关的成员变量
    int client_connect_fd;                              // 客户端连接套接字描述符
    sockaddr_in target_server_address;                  // 客户端连接的目标服务器地址

    // 内部线程事件循环函数

public:
    void release() override;
	bool init(int32_t nThreadCount) override;
	int32_t listen(uint16_t wPort,Ilisten_event* pEvent,Ipack_parser* pParser) override;
	int32_t connect(const char* pRemoteIp,uint16_t wPort,Iconnect_event* pEvent,Ipack_parser* pParser) override;
	int32_t send(int32_t nId,const void* pData,uint32_t dwDataSize) override;
	void close(int32_t nId) override;
    virtual ~Itcp_manager_implement() override;
};

// Ilisten_event_implement成员函数实现
void Ilisten_event_implement:: on_accept(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort) {
    std::cout << "accept new client connect successfully, allocated client_fd is: " << nId << ", client RemoteIp: "
              << pRemoteIp << ", client RemotePort: " << wRemotePort << std::endl;
}

void Ilisten_event_implement:: on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) {    
    if(pErrorMsg == nullptr) {  // 此时成功关闭客户端套接字
        std::cout << "client close successfully, client_fd is: " << nId << ", client RemoteIp: " << pRemoteIp << ", client RemotePort: "
              << wRemotePort << std::endl;
    }
    else {  // 此时关闭客户端套接字失败
        std::cout << "client close failed, client_fd is: " << nId << ", client RemoteIp: " << pRemoteIp << ", client RemotePort: "
              << wRemotePort << ", error message: " << pErrorMsg << std::endl;
    }
}

// void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) {

// }

void Itcp_manager_implement:: release() {

}

void Itcp_manager_implement:: close(int32_t nId) {

}

// tcp网络库类
bool Itcp_manager_implement::init(int32_t nThreadCount) {
    // 在堆区声明缓冲区，并将相应的缓冲区指针存入buffer_vector中，缓冲区的数量是声明的线程数量
    for (int i = 0; i < nThreadCount; i++) {
        char *buffer = new char[BUFFER_SIZE + 1];
        memset(buffer, 0, sizeof(char)*(BUFFER_SIZE + 1));
        buffer_vector.push_back(buffer);
    }
    buffer_locks = new std::mutex[nThreadCount];

    // 设置线程池中的线程数量
    try {
        bool init_pool_result = thread_pool.init(nThreadCount);
        if (init_pool_result == false) {
            throw std::invalid_argument("Invalid thread count or threads' num surpass 16!");
        }
        else {return true;}
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

int32_t Itcp_manager_implement::listen(uint16_t wPort,Ilisten_event* pEvent,Ipack_parser* pParser) {
    // 0. 初始化与服务端有关的类成员变量
    int addrlen = sizeof(server_address);
    // 构造监听事件类指针
    listen_event = new Ilisten_event_implement();

    // 构造lru缓存表及lru事件
    client_fd_address_lru = create_lru_mgr<int, sockaddr_in>();
    lru_event = new Ilru_event_implement<int, sockaddr_in>;
    client_fd_address_lru->init(MAX_SESSION_COUNT, lru_event);
    

    // 1. 创建服务器套接字，并设置为非阻塞类型
    try {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
           throw SocketException("create tcp type socket failed"); 
        }
        else {
            // 设置服务器套接字为非阻塞
            int server_flag = fcntl(server_fd, F_GETFL, 0);
            if (server_flag == -1) {
                throw SocketException("fcntl failed");
            }
            if (fcntl(server_fd, F_SETFL, server_flag | O_NONBLOCK) == -1) {
                throw SocketException("fcntl failed to set non-blocking");
            }
            std::cout << "create tcp type socket success, server_fd is: " << server_fd << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << std::endl;
        close(server_fd);
        return -1;
    }

    // 2. 设置服务器监听的地址，端口参数
    memset(&server_address, 0, addrlen);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(wPort);
    
    // 3. 将1中创建的socket套接字绑定到设置的server_address变量
    try {
        int bind_result = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
        if (bind_result == -1) {
            throw SocketException("socket bind server address failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " socket bind server address success, binded address is below" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << " socket_fd: " << e.get_socket_fd() << std::endl;
        return -1;
    }

    // 4. 监听连接，设置最大允许连接数为MAX_SESSION_COUNT
    try {
        int listen_result = ::listen(server_fd, MAX_SESSION_COUNT);
        if (listen_result == -1) {
            throw SocketException("socket listen failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " listen success, listened address is below" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch(const SocketException& e) {
        std::cerr << e.what() << " socket_fd: " << e.get_socket_fd() << std::endl;
        return -1;
    }

    // 5. 设置select函数的参数
    FD_ZERO(&allfds);
    FD_SET(server_fd, &allfds);
    // 刚开始初始化为最大fd为server_fd
    int max_fd = server_fd;
    // 防止重复分发任务的哈希表
    std::unordered_set<int> in_process_client_fd;

    // 6. 进入select循环
    try {
        while(true) {
            // 设置timeout为0，为阻塞类型
            // timeval timeout;
            // timeout.tv_sec = 3;
            // timeout.tv_usec = 0;

            // readfds同步更新为此时server_fd和已知的client_fd的集合
            readfds = allfds;
            // activity变量指明有多少个描述符准备好了
            int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
            if (activity < 0) {
                throw std::runtime_error("select error");
            }
            else if(activity == 0) {    // 此时没有可读的描述符，继续循环检索
                continue;
            }
            else {  // 此时有可读的描述符
                // 检查客户端socket查看是否有新的数据到达
                int client_size = client_fd_address_lru->get_size();
                int client_fd_array[client_size];
                // 获取lru缓存中所有的客户端socket
                client_fd_address_lru->listAllKeys(client_fd_array);
                for (int i = 0; i < client_size; i++) {
                    int client_fd = client_fd_array[i];
                    if (FD_ISSET(client_fd, &readfds)) {    // 此时clientfd客户端有新的数据到达
                        // std::cout << "client_fd: " << client_fd << " has new data to read" << std::endl;
                        if (in_process_client_fd.find(client_fd) != in_process_client_fd.end()) {    // 此时表明该客户端套接字已经被一个工作线程处理，不再重复分发
                            continue;
                        }
                        // 声明此时任务已分发，避免重复分发
                        in_process_client_fd.insert(client_fd);
                        // 调用线程池进行读取数据的任务
                        thread_pool.enqueueTask([client_fd, this, &in_process_client_fd]() { 
                            try {
                                std::cout<<"thread pool now do it!"<<std::endl;
                                READCLIENTREQUEST(client_fd, this->buffer_vector, this->buffer_locks, this->client_fd_address_lru, this->allfds);
                            }
                            catch (const SocketException& e) {
                                std::cerr << e.what() << " client_fd: " << e.get_socket_fd() << std::endl;
                            }
                            in_process_client_fd.erase(client_fd);
                        });
                    }
                }

                // 检查服务器socket是否有新的客户端连接
                if (FD_ISSET(server_fd, &readfds)) {    // 此时有新的客户端连接
                    while (true) {  // 将此时accept队列的所有客户端连接处理
                        sockaddr_in client_address;
                        int client_addrlen = sizeof(client_address);
                        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);
                        if (client_fd < 0) {
                            if (errno == EWOULDBLOCK || errno == EAGAIN) { // 此时表明accept队列中无新的客户端连接
                                std::cout << "has accepted all new client connections" << std::endl;
                                break;
                            }
                            else {
                                throw SocketException("accept client connection failed", server_fd);
                            }
                        }
                        else if (client_fd == 0) { // 此时表明无新的客户端连接
                            std:: cout << "no new client connection" << std::endl;
                            break;
                        }
                        else { // 此时表明有新的客户端连接
                            listen_event->on_accept(client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            // 更新max_fd
                            max_fd = std::max(max_fd, client_fd);
                            // 检查lru缓存是否已满
                            int lru_max_size = client_fd_address_lru->get_max_size();
                            int size = client_fd_address_lru->get_size();
                            if (size < lru_max_size) { // 此时lru缓存未满
                                // 插入新client_fd到lru缓存
                                client_fd_address_lru->add(client_fd, &client_address);
                                // 插入新client_fd到allfds套接字集合
                                FD_SET(client_fd, &allfds);
                            }
                            else { // 此时lru缓存已满，需要同步更新allfds套接字集合
                                // 获取lru缓存中最近最久未使用的客户端socket键
                                int rear_client_fd = *(client_fd_address_lru->getRearKey());
                                // 关闭最近最久未使用的客户端socket
                                try {
                                    int close_result = ::close(rear_client_fd);
                                    if (close_result == -1) {
                                        throw SocketException("close rear client_fd failed", rear_client_fd);
                                    }
                                    else {
                                        listen_event->on_close(rear_client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), 0, nullptr);
                                    }
                                }
                                catch (const SocketException& e) {
                                    listen_event->on_close(rear_client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), errno, strerror(errno));
                                }
                                // 从allfds中消除最近最久未使用的客户端socket
                                FD_CLR(rear_client_fd, &allfds);
                                // 插入新client_fd到lru缓存
                                client_fd_address_lru->add(client_fd, &client_address);
                                // 插入新client_fd到allfds套接字集合
                                FD_SET(client_fd, &allfds);
                            }
                        }
                    }
                }
            }
            // select进入下一轮循环
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    delete listen_event;
    return server_fd;
}

int32_t Itcp_manager_implement:: connect(const char* pRemoteIp,uint16_t wPort,Iconnect_event* pEvent,Ipack_parser* pParser) {
    // 1. 创建tcp客户端连接套接字
    try {
        client_connect_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_connect_fd < 0) {
            throw SocketException("create tcp type socket failed");
        }
        else {
            std::cout << "create tcp type socket success, client_fd is: " << client_connect_fd << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    
    // 2. 设置服务器地址信息
    target_server_address.sin_family = AF_INET;
    inet_pton(AF_INET, pRemoteIp, &target_server_address.sin_addr);
    target_server_address.sin_port = htons(wPort);

    // 3. 连接到服务器
    try {
        int connect_result = ::connect(client_connect_fd, (struct sockaddr*)&target_server_address, sizeof(target_server_address));
        if (connect_result == -1) {
            throw SocketException("Connection to the server failed", client_connect_fd);
        }
        else {
            std::cout << "client socket_fd: " << client_connect_fd << " connected to the server at " << pRemoteIp << ":" << wPort << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << " client_socket_fd: " << e.get_socket_fd() << std::endl;
        return -1;
    }
    return client_connect_fd;
}

int32_t Itcp_manager_implement:: send(int32_t nId,const void* pData,uint32_t dwDataSize) {
    ssize_t bytesSent;
    try {
        bytesSent = ::send(nId, pData, dwDataSize, 0);
        if (bytesSent < 0) {
            throw SocketException("send data to server failed", nId);
        }
        else {
            std::cout << "send data to server success, sent bytes: " << bytesSent << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "client_socket_fd: " << e.get_socket_fd() << std::endl;
        return -1;
    }
    return bytesSent;
}

Itcp_manager_implement:: ~Itcp_manager_implement() {
    delete this->client_fd_address_lru;
    delete this->lru_event;
    for (int i = 0; i < (int)buffer_vector.size(); i++) {
        delete[] buffer_vector[i];
    }
    delete[] buffer_locks;
}

// 线程池中执行的任务函数定义
void READCLIENTREQUEST(int client_fd, std::vector<char*>& buffer_vector, std::mutex* buffer_locks, Ilru_mgr<int, sockaddr_in>* client_fd_address_lru, fd_set& allfds) {
    int buffer_index = -1;
    {
        // 检索是否还有空余的buffer缓冲区，并申请加锁
        for (int i = 0; i < (int)buffer_vector.size(); i++) {
            std::unique_lock<std::mutex> lock_guard(buffer_locks[i], std::try_to_lock);
            if (lock_guard.owns_lock()) {   // 如果成功加锁
                buffer_index = i;
                break;
            }
        }
        // 如果没有空余的buffer缓冲区，随机选择一个buffer缓冲区申请加锁
        if (buffer_index == -1) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, (int)buffer_vector.size() - 1);
            buffer_index = dis(gen); // 随机选择一个索引
            std::unique_lock<std::mutex> lock_guard(buffer_locks[buffer_index]);
        }

        // 此时获取了buffer_index对应的buffer缓冲区的访问权限
        char *buffer = buffer_vector[buffer_index];
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            throw SocketException("read client request failed", client_fd);
        }
        else if (bytes_read == 0) { // 此时客户端关闭连接
            if(::close(client_fd) == -1) {
                throw SocketException("close client socket failed", client_fd);
            }
            // 清除lru缓存中的客户端socket及FD_ALL集合中的客户端socket
            client_fd_address_lru->erase(client_fd);
            FD_CLR(client_fd, &allfds);
            std::cout << "Client disconnected successfully, socket FD: " << client_fd << std::endl;
        }
        else {
            buffer[bytes_read] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            memset(buffer, 0, BUFFER_SIZE);
        }
        // 花括号结束时，lock_guard对象被销毁，buffer_locks[buffer_index]解锁，可以被其他线程加锁
    }
}
