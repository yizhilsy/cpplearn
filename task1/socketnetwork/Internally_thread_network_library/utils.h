#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

// 创建指定名称的文件夹，允许原先存在
bool create_directory(const std:: string& directory_name) {
    std::string mk_directory_name = "./" + directory_name;
    struct stat st = {0};
    // 检查目录是否存在
    if (stat(mk_directory_name.c_str(), &st) == -1) {
        // 如果目录不存在，则创建目录
        if (mkdir(mk_directory_name.c_str(), 0744) == -1) {
            std::cerr << "Create directory failed: " << strerror(errno) << std::endl;
            return false;
        }
    }
    else {
        std::cout << "Directory: " << mk_directory_name << " already exists!" << std::endl;
    }
    return true;
}

// 工具函数实现
bool isSocketValid(int socket_fd) {
    int socket_status = fcntl(socket_fd, F_GETFD);
    return (socket_status != -1 && errno != EBADF);
}