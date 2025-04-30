#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <vector>

constexpr const char* SERVER_IP = "127.0.0.1"; // Localhost
constexpr int SERVER_PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

// Flag to control the receiver thread loop
std::atomic<bool> g_connected = true;

// Function to receive messages from the server
void receive_messages(int client_socket) {
    char buffer[BUFFER_SIZE];
    while (g_connected) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (!g_connected) break; // Check flag again in case disconnect happened while blocked

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                std::cout << "\nServer disconnected." << std::endl;
            } else if (errno != ETIMEDOUT && errno != EAGAIN && errno != EWOULDBLOCK) {
                // Only print error if it's not a non-blocking timeout/retry situation (though we use blocking recv here)
                std::cerr << "\nError receiving from server: " << strerror(errno) << std::endl;
            }
             g_connected = false; // Signal main thread to stop
             // Forcefully unblock std::getline in main thread if stuck
             // This is a bit of a hack; a better way involves select/poll or pipes.
             // For simplicity, we rely on the user hitting Enter after seeing disconnect.
             // Alternatively, close the socket here, which might cause send in main thread to fail.
             // close(client_socket); // Could cause issues if main thread tries to send after this
            break;
        }
        buffer[bytes_received] = '\0'; // Null-terminate
        std::cout << "\nServer response: " << buffer; // Print response (server should add \n)
        std::cout << "Enter message ('disconnect' to quit): " << std::flush; // Re-prompt user
    }
     std::cout << "Receiver thread finished." << std::endl;
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr;

    // 1. Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    // 2. Prepare server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported: " << SERVER_IP << std::endl;
        close(client_fd);
        return 1;
    }

    // 3. Connect to the server
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed: " << strerror(errno) << std::endl;
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to server " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    // 4. Start receiver thread
    std::thread receiver_thread(receive_messages, client_fd);

    // 5. Main loop to send messages
    std::string input_line;
    while (g_connected) {
        std::cout << "Enter message ('disconnect' to quit): " << std::flush;
        if (!std::getline(std::cin, input_line)) {
            // Handle EOF (e.g., Ctrl+D) or input error
             std::cout << "\nInput stream closed. Disconnecting..." << std::endl;
             g_connected = false; // Signal receiver thread
            break;
        }

        if (!g_connected) break; // Check if receiver thread detected disconnect

        if (input_line == "disconnect") {
            g_connected = false; // Signal receiver thread
            break;
        }

        // Add newline because server might expect line-based messages
        input_line += "\n";

        // Send message to server
        ssize_t bytes_sent = send(client_fd, input_line.c_str(), input_line.length(), 0);
        if (bytes_sent < 0) {
             std::cerr << "Error sending message: " << strerror(errno) << std::endl;
             g_connected = false; // Signal receiver thread
             break;
        } else if (static_cast<size_t>(bytes_sent) != input_line.length()) {
             std::cerr << "Warning: Partial send occurred." << std::endl;
             // Handle partial send if necessary
        }
    }

    // 6. Cleanup
     std::cout << "Disconnecting..." << std::endl;
     g_connected = false; // Ensure flag is false

    // Shutdown the connection gracefully (optional but good practice)
    // SHUT_WR: Stop sending, allow receiving pending data
    // SHUT_RDWR: Stop both sending and receiving
    if (shutdown(client_fd, SHUT_WR) < 0) {
        if (errno != ENOTCONN) { // Ignore error if socket already disconnected
             std::cerr << "Error shutting down socket: " << strerror(errno) << std::endl;
        }
    }


    // Wait for the receiver thread to finish
    if (receiver_thread.joinable()) {
        receiver_thread.join();
    }

    // Close the socket file descriptor
    close(client_fd);

    std::cout << "Connection closed." << std::endl;
    return 0;
}