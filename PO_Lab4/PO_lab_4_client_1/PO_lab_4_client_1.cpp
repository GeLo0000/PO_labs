#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

#define NUM_ROWS 1000
#define NUM_COLUMNS 1000
#define MAX_VALUE 10000
#define NUM_TREADS 5


enum Tags : uint8_t {
    CONFIG = 0x01,
    DATA = 0x02,
    START = 0x03,
    STATUS_REQUEST = 0x04,
    RESULT_REQUEST = 0x05,
    STATUS_RESPONSE = 0x06,
    RESULT_RESPONSE = 0x07,
    ERROR_TAG = 0x08
};

void writeLength(char* buf, uint32_t len) {
    buf[0] = (len >> 24) & 0xFF;
    buf[1] = (len >> 16) & 0xFF;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
}

void sendTLV(SOCKET sock, uint8_t tag, const std::vector<char>& value) {
    uint32_t len = value.size();
    std::vector<char> buffer(5 + len);
    buffer[0] = tag;
    writeLength(buffer.data() + 1, len);
    std::copy(value.begin(), value.end(), buffer.begin() + 5);
    send(sock, buffer.data(), buffer.size(), 0);
}

bool recvAll(SOCKET sock, char* buffer, int total) {
    int received = 0;
    while (received < total) {
        int r = recv(sock, buffer + received, total - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}

void sendConfig(SOCKET sock, int rows, int cols, int threads) {
    std::vector<char> value(12);
    int net_rows = htonl(rows);
    int net_cols = htonl(cols);
    int net_threads = htonl(threads);
    memcpy(&value[0], &net_rows, 4);
    memcpy(&value[4], &net_cols, 4);
    memcpy(&value[8], &net_threads, 4);
    sendTLV(sock, CONFIG, value);
}

void sendData(SOCKET sock, const std::vector<int>& matrix) {
    std::vector<char> value(matrix.size() * 4);
    for (size_t i = 0; i < matrix.size(); ++i) {
        int net_val = htonl(matrix[i]);
        memcpy(&value[i * 4], &net_val, 4);
    }
    sendTLV(sock, DATA, value);
}

void sendStart(SOCKET sock) {
    sendTLV(sock, START, {});
}

void sendStatusRequest(SOCKET sock) {
    sendTLV(sock, STATUS_REQUEST, {});
}

void sendResultRequest(SOCKET sock) {
    sendTLV(sock, RESULT_REQUEST, {});
}

void receiveResponse(SOCKET sock) {
    char header[5];
    if (!recvAll(sock, header, 5)) {
        std::cerr << "Connection closed\n";
        return;
    }

    uint8_t tag = header[0];
    uint32_t length = (unsigned char)header[1] << 24 |
        (unsigned char)header[2] << 16 |
        (unsigned char)header[3] << 8 |
        (unsigned char)header[4];

    std::vector<char> value(length);
    if (!recvAll(sock, value.data(), length)) {
        std::cerr << "Failed to receive value\n";
        return;
    }

    std::string content(value.begin(), value.end());

    if (tag == STATUS_RESPONSE || tag == RESULT_RESPONSE) {
        std::cout << "[SERVER]: " << content << "\n";
    }
    else if (tag == ERROR_TAG) {
        std::cerr << "[ERROR]: " << content << "\n";
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed\n";
        return 1;
    }

    int rows = NUM_ROWS, cols = NUM_COLUMNS, threads = NUM_TREADS;
    std::vector<int> matrix(rows * cols);
    for (auto& x : matrix) x = rand() % MAX_VALUE;

    sendConfig(sock, rows, cols, threads);
    receiveResponse(sock);

    sendData(sock, matrix);
    receiveResponse(sock);

    sendStart(sock);
    receiveResponse(sock);

   while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sendStatusRequest(sock);
        char header[5];
        if (!recvAll(sock, header, 5)) break;
        uint8_t tag = header[0];
        uint32_t len = (unsigned char)header[1] << 24 | (unsigned char)header[2] << 16 |
            (unsigned char)header[3] << 8 | (unsigned char)header[4];
        std::vector<char> val(len);
        if (!recvAll(sock, val.data(), len)) break;
        std::string status(val.begin(), val.end());
        std::cout << "[STATUS]: " << status << "\n";
        if (status == "DONE") break;
    }

    sendResultRequest(sock);
    receiveResponse(sock);

    closesocket(sock);
    WSACleanup();
    return 0;
}
