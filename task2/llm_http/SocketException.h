#include <exception>
#include <iostream>

// 定义socket异常类
class SocketException : public std::runtime_error 
{
private:
    int socket_fd;

public:
    explicit SocketException(const std::string& message, const int error_socket_fd = 0)
    : std::runtime_error(message), socket_fd(error_socket_fd){}
    int get_socket_fd() const { return socket_fd; }
};