#include <cstdint>
#include <chrono>
#include <string>
#include <thread>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

constexpr std::size_t BUFFER_SIZE = 1024;

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

[[nodiscard]] std::int32_t connect(const std::string server_address, const std::uint16_t server_port)
{
    std::int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        throw std::errc::io_error;
    }

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_address.c_str(), &server.sin_addr) <= 0)
    {
        close(sockfd);
        throw std::errc::address_family_not_supported;
    }

    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        throw std::errc::connection_refused;
    }

    return sockfd;
}

std::int32_t main()
{
    const std::string server_address = "10.22.49.160";
    const std::int16_t server_port = 23;
    const std::string password = "moxa";
    const std::chrono::milliseconds delay_after_connect(100);

    std::int32_t sockfd;

    try
    {
        sockfd = connect(server_address, server_port);
    }
    catch (...)
    {
        return 1;
    }

    // slight delay, telnet is slow
    std::this_thread::sleep_for(delay_after_connect);

    // send the password
    send_data(sockfd, password);

    // choose to restart the server
    send_data(sockfd, "s");

    // confirm the restart
    send_data(sockfd, "y");

    // clean the response junk
    while (!receive_data(sockfd).empty());

    close(sockfd);

    return 0;
}
