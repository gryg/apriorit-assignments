#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <atomic>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr int BACKLOG = 5; // Max pending connections queue size

// Global flag to signal server shutdown
std::atomic<bool> g_running = true;

// Function to handle client connections
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    // Convert client IP to string for logging
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    std::cout << "Connection accepted from " << client_ip << ":" << client_port << std::endl;

    while (g_running) {
        memset(buffer, 0, BUFFER_SIZE);
        // Receive data from client
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                std::cout << "Client " << client_ip << ":" << client_port << " disconnected." << std::endl;
            } else {
                std::cerr << "Error receiving from client " << client_ip << ":" << client_port << ": " << strerror(errno) << std::endl;
            }
            break; // Exit loop on error or disconnect
        }

        // Null-terminate the received data to treat as string
        buffer[bytes_received] = '\0';
        std::string message(buffer);

        // Trim potential trailing newline characters from client input
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
             message.pop_back();
        }
        std::cout << "Received from " << client_ip << ":" << client_port << ": " << message << std::endl;


        std::string response;
        if (message == "hello") {
            response = "world\n";
        } else {
            response = message + "\n"; // Echo back with newline
        }

        // Send response back to client
        ssize_t bytes_sent = send(client_socket, response.c_str(), response.length(), 0);
        if (bytes_sent < 0) {
            std::cerr << "Error sending to client " << client_ip << ":" << client_port << ": " << strerror(errno) << std::endl;
            break; // Exit loop on send error
        } else if (static_cast<size_t>(bytes_sent) != response.length()) {
             std::cerr << "Warning: Partial send to client " << client_ip << ":" << client_port << std::endl;
             // In a real-world scenario, you might loop here to send remaining bytes
        }
    }

    // Cleanup: Close the client socket
    close(client_socket);
    std::cout << "Closed connection for " << client_ip << ":" << client_port << std::endl;
}

// Signal handler for graceful shutdown
void signal_handler(int signum) {
    std::cout << "\nCaught signal " << signum << ". Shutting down server..." << std::endl;
    g_running = false;
    // Might need additional mechanisms to interrupt the accept() call if required,
    // like closing the listening socket here or using non-blocking accept with timeout.
    // For simplicity, this example relies on the g_running flag checked after accept returns.
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signal_handler);

    // 1. Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    // Set socket options - Allow reuse of local addresses
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }
    // SO_REUSEPORT is also useful sometimes but SO_REUSEADDR is more common
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    //     perror("setsockopt(SO_REUSEPORT) failed");
    // }


    // 2. Prepare the sockaddr_in structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    server_addr.sin_port = htons(PORT);     // Port number (host-to-network short)

    // 3. Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }

    // 4. Listen for incoming connections
    if (listen(server_fd, BACKLOG) < 0) {
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    // 5. Accept incoming connections in a loop
    while (g_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if (!g_running) break; // Check flag again after accept returns (e.g., if interrupted by signal)

        if (client_socket < 0) {
             // Check if accept was interrupted by our signal handler
            if (errno == EINTR && !g_running) {
                 break; // Graceful shutdown requested
            }
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue; // Continue listening for other connections
        }

        // Create a new thread to handle the client connection
        // Detach the thread: the thread is responsible for closing its own socket.
        // The main thread doesn't need to join it.
        std::thread client_thread(handle_client, client_socket, client_addr);
        client_thread.detach();
    }

    // Cleanup: Close the listening socket when loop terminates
    std::cout << "Closing listening socket." << std::endl;
    close(server_fd);

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}