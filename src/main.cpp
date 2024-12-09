#include <cstdint>
#include <chrono>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <stdexcept>

constexpr std::size_t BUFFER_SIZE = 1024;
constexpr uint16_t DEFAULT_PORT = 23;
constexpr const char* DEFAULT_IP = "192.168.0.1";
constexpr const char* DEFAULT_PASSWORD = "password";

std::size_t send_data(std::int32_t sockfd, const std::string& data)
{
    const std::string data_to_send = data + "\r\n";
    return send(sockfd, data_to_send.c_str(), data_to_send.length(), 0);
}

std::string receive_data(std::int32_t sockfd)
{
    char buffer[BUFFER_SIZE] = {0};
    std::int32_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0)
    {
        return "";
    }
    std::string response(buffer, static_cast<std::size_t>(bytes_received));
    return response;
}

[[nodiscard]] std::int32_t connect_socket(const std::string& server_address, const std::uint16_t server_port)
{
    std::int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_address.c_str(), &server.sin_addr) <= 0)
    {
        close(sockfd);
        throw std::runtime_error("Invalid IP address");
    }

    if (::connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Connection failed");
    }

    return sockfd;
}

void print_usage(const char* program_name) 
{
    std::cerr << "Usage: " << program_name << " [-i IP_ADDRESS] [-p PORT] [-P PASSWORD]" << std::endl;
    std::cerr << "  -i: IP address (default: " << DEFAULT_IP << ")" << std::endl;
    std::cerr << "  -p: Port number (default: " << DEFAULT_PORT << ")" << std::endl;
    std::cerr << "  -P: Password (default: " << DEFAULT_PASSWORD << ")" << std::endl;
}

std::int32_t main(std::int32_t argc, char* argv[]) 
{
    std::string server_address = DEFAULT_IP;
    uint16_t server_port = DEFAULT_PORT;
    std::string password = DEFAULT_PASSWORD;

    for (int i = 1; i < argc; i++) 
    {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) 
        {
            server_address = argv[++i];
        }
        else if (arg == "-p" && i + 1 < argc) 
        {
            try 
            {
                server_port = std::stoi(argv[++i]);
            }
            catch (const std::exception&) 
            {
                std::cerr << "Invalid port number" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }
        else if (arg == "-P" && i + 1 < argc) 
        {
            password = argv[++i];
        }
        else if (arg == "-h" || arg == "--help") 
        {
            print_usage(argv[0]);
            return 0;
        }
        else 
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    const std::chrono::milliseconds delay_after_connect(100);

    std::int32_t sockfd;

    try 
    {
        sockfd = connect_socket(server_address, server_port);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return 1;
    }

    try 
    {
        // slight delay, telnet is slow
        std::this_thread::sleep_for(delay_after_connect);

        // send the password
        send_data(sockfd, password);

        // slight delay, telnet is slow
        std::this_thread::sleep_for(delay_after_connect);

        // choose to restart the server
        send_data(sockfd, "s");

        // slight delay, telnet is slow
        std::this_thread::sleep_for(delay_after_connect);

        // confirm the restart
        send_data(sockfd, "y");

        // slight delay, telnet is slow
        std::this_thread::sleep_for(delay_after_connect);

        // clean the response junk
        while (!receive_data(sockfd).empty());
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Operation error: " << e.what() << std::endl;
        close(sockfd);
        return 1;
    }

    close(sockfd);

    return 0;
}
