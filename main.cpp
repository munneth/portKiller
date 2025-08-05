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
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

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

class PortInfo {
private:
    int port;
    std::string protocol;
    std::string state;
    std::string process;
    int pid;

public:
    PortInfo(int p, const std::string& prot, const std::string& st, const std::string& proc, int processId)
        : port(p), protocol(prot), state(st), process(proc), pid(processId) {}
    
    int getPort() const { return port; }
    const std::string& getProtocol() const { return protocol; }
    const std::string& getState() const { return state; }
    const std::string& getProcess() const { return process; }
    int getPid() const { return pid; }
};

std::vector<PortInfo> getActivePorts() {
    std::vector<PortInfo> ports;
    
    // Read /proc/net/tcp for TCP ports
    std::ifstream tcpFile("/proc/net/tcp");
    std::string line;
    
    // Skip header line
    std::getline(tcpFile, line);
    
    while (std::getline(tcpFile, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 4) {
            // Extract local address and port
            std::string localAddr = tokens[1];
            std::string state = tokens[3];
            
            // Parse port (hex to decimal)
            size_t colonPos = localAddr.find(':');
            if (colonPos != std::string::npos) {
                std::string portHex = localAddr.substr(colonPos + 1);
                int port = std::stoi(portHex, nullptr, 16);
                
                // Only include listening ports
                if (state == "0A") { // LISTEN state
                    int processId = -1;
                    std::string processName = "Unknown";
                    
                    // Try to get process info
                    std::string cmd = "lsof -i :" + std::to_string(port) + " -t 2>/dev/null";
                    FILE* pipe = popen(cmd.c_str(), "r");
                    if (pipe) {
                        char buffer[128];
                        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                            processId = std::stoi(buffer);
                            
                            // Get process name
                            std::string cmd2 = "ps -p " + std::to_string(processId) + " -o comm= 2>/dev/null";
                            FILE* pipe2 = popen(cmd2.c_str(), "r");
                            if (pipe2) {
                                char buffer2[128];
                                if (fgets(buffer2, sizeof(buffer2), pipe2) != nullptr) {
                                    processName = buffer2;
                                    // Remove newline
                                    processName.erase(std::remove(processName.begin(), processName.end(), '\n'), processName.end());
                                }
                                pclose(pipe2);
                            }
                        }
                        pclose(pipe);
                    }
                    
                    PortInfo info(port, "TCP", "LISTEN", processName, processId);
                    ports.push_back(info);
                }
            }
        }
    }
    
    return ports;
}

void displayPorts(const std::vector<PortInfo>& ports) {
    std::cout << "\n=== Active Ports on Localhost ===\n";
    std::cout << "Port\tProtocol\tState\t\tPID\tProcess\n";
    std::cout << "----\t--------\t-----\t\t---\t-------\n";
    
    for (size_t i = 0; i < ports.size(); ++i) {
        const auto& port = ports[i];
        std::cout << i + 1 << ". " << port.getPort() << "\t" 
                  << port.getProtocol() << "\t\t" 
                  << port.getState() << "\t\t"
                  << port.getPid() << "\t"
                  << port.getProcess() << "\n";
    }
}

bool killProcess(int pid) {
    std::string cmd = "kill -9 " + std::to_string(pid);
    int result = system(cmd.c_str());
    return result == 0;
}
int main() {
    std::string osName = getOsName();
    std::cout << "Port Killer - Running on " << osName << std::endl;
    
    while (true) {
        std::vector<PortInfo> ports = getActivePorts();
        
        if (ports.empty()) {
            std::cout << "No active ports found.\n";
            return 0;
        }
        
        displayPorts(ports);
        
        std::cout << "\nEnter the number of the port to kill (0 to exit): ";
        int choice;
        std::cin >> choice;
        
        if (choice == 0) {
            std::cout << "Exiting...\n";
            break;
        }
        
        if (choice > 0 && choice <= static_cast<int>(ports.size())) {
            const auto& selectedPort = ports[choice - 1];
            std::cout << "Killing process on port " << selectedPort.getPort() 
                      << " (PID: " << selectedPort.getPid() << ", Process: " << selectedPort.getProcess() << ")\n";
            
            if (killProcess(selectedPort.getPid())) {
                std::cout << "Successfully killed process!\n";
            } else {
                std::cout << "Failed to kill process. You may need sudo privileges.\n";
            }
        } else {
            std::cout << "Invalid choice. Please try again.\n";
        }
        
        std::cout << "\nPress Enter to refresh port list...";
        std::cin.ignore();
        std::cin.get();
    }
    
    return 0;
}