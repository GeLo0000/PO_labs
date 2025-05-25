#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

    if (received <= 0) {
        closesocket(client_socket);
        return;
    }

    buffer[received] = '\0';
    std::istringstream request_stream(buffer);
    std::string method, path, version;
    request_stream >> method >> path >> version;

    std::cout << "New client connected. Request: " << method << " " << path << std::endl;

    if (method != "GET") {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_socket, response.c_str(), response.size(), 0);
        closesocket(client_socket);
        return;
    }

    if (path == "/") {
        path = "/index.html";
    }

    std::string file_path = "." + path;
    std::ifstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        std::ifstream not_found_file("./404.html", std::ios::binary);
        std::ostringstream content;
        std::string body;

        if (not_found_file.is_open()) {
            content << not_found_file.rdbuf();
            body = content.str();
        }
        else {
            body = "<html><body><h1>404 Not Found</h1></body></html>";
        }

        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: "
            + std::to_string(body.size()) + "\r\n\r\n" + body;

        send(client_socket, response.c_str(), response.size(), 0);
    }
    else {
        std::ostringstream content;
        content << file.rdbuf();
        std::string body = content.str();
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
            + std::to_string(body.size()) + "\r\n\r\n" + body;

        send(client_socket, response.c_str(), response.size(), 0);
    }

    closesocket(client_socket);
    std::cout << "Client disconnected.\n";
}


int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running on http://localhost:" << PORT << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket != INVALID_SOCKET) {
            std::thread(handle_client, client_socket).detach();
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
