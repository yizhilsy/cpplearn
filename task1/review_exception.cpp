#include <iostream>
#include <string>
#include <exception>

class SocketException : public std::runtime_error {
public:
    SocketException(const std::string& message)
        : std::runtime_error(message) {}
};

void handleError(const SocketException& e) {
    std::cout << "Error: " << e.what() << std::endl;
}

int main() {
    try {
        throw SocketException("test exception");
    } catch (const SocketException& e) {
        handleError(e);
    }
    SocketException e("no try catch exception");
    std::cerr << e.what() << std::endl;
    return 0;
}