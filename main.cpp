#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <memory>

std::string getOsName() {
    #ifdef _WIN32
        return "Windows 32-bit";
    #elif _WIN64
        return "Windows 64-bit";
    #elif __APPLE__ || __MACH__
        return "Mac OSX";
    #elif __linux__
        return "Linux";
    #elif __FreeBSD__
        return "FreeBSD";
    #else
        return "Unknown OS";
    #endif
}

std::shared_ptr<int> createSocket(){
    auto serverSocket = std::make_shared<int>(socket(AF_INET, SOCK_STREAM, 0));
    return serverSocket;
}

bool bindSocket(std::shared_ptr<int> serverSocket){
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    try{
        bind(*serverSocket, sockaddr* addr, socklen_t addrlen);
        //add unbind here
        return true;
    }
    catch(const std::exception& e){
        //add port name retrieveal
        std::cerr << "Error binding socket: " << e.what() << std::endl;
        return false;
    }
}
int main() {
    std::string osName = getOsName();
    



    return 0;
}