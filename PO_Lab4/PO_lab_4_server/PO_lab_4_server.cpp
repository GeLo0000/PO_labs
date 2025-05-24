#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <algorithm>
#include <climits>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

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

enum Status { WAITING, IN_PROGRESS, DONE };

struct TaskContext {
    int rows = 0, cols = 0;
    int threads = 0;
    std::vector<int> matrix;
    int minVal = INT_MAX;
    int maxVal = INT_MIN;
    Status status = WAITING;
    std::mutex mtx;
};

std::map<SOCKET, TaskContext> clientTasks;

uint32_t readLength(const char* data) {
    return (unsigned char)data[0] << 24 | (unsigned char)data[1] << 16 |
        (unsigned char)data[2] << 8 | (unsigned char)data[3];
}

void writeLength(char* buf, uint32_t len) {
    buf[0] = (len >> 24) & 0xFF;
    buf[1] = (len >> 16) & 0xFF;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
}

void sendTLV(SOCKET client, uint8_t tag, const std::string& value) {
    uint32_t len = value.size();
    std::vector<char> buffer(5 + len);
    buffer[0] = tag;
    writeLength(buffer.data() + 1, len);
    memcpy(buffer.data() + 5, value.data(), len);
    send(client, buffer.data(), buffer.size(), 0);
}

void calculateMinMax(TaskContext& ctx) {
    ctx.status = IN_PROGRESS;
    std::vector<std::thread> threads;
    int chunkSize = ctx.matrix.size() / ctx.threads;
    for (int i = 0; i < ctx.threads; ++i) {
        threads.emplace_back([&, i]() {
            int localMin = INT_MAX;
            int localMax = INT_MIN;
            int start = i * chunkSize;
            int end = (i == ctx.threads - 1) ? ctx.matrix.size() : start + chunkSize;
            for (int j = start; j < end; ++j) {
                localMin = min(localMin, ctx.matrix[j]);
                localMax = max(localMax, ctx.matrix[j]);
            }
            std::lock_guard<std::mutex> lock(ctx.mtx);
            ctx.minVal = min(ctx.minVal, localMin);
            ctx.maxVal = max(ctx.maxVal, localMax);
            });
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for (auto& t : threads) t.join();
    ctx.status = DONE;
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

void handleClient(SOCKET client) {
    char header[5];
    while (true) {
        if (!recvAll(client, header, 5)) break;

        uint8_t tag = header[0];
        uint32_t length = readLength(header + 1);
        std::vector<char> value(length);
        if (!recvAll(client, value.data(), length)) break;

        TaskContext& ctx = clientTasks[client];

        switch (tag) {
        case CONFIG: {
            if (length != 12) {
                sendTLV(client, ERROR_TAG, "Invalid CONFIG length");
                break;
            }
            ctx.rows = ntohl(*(int*)&value[0]);
            ctx.cols = ntohl(*(int*)&value[4]);
            ctx.threads = ntohl(*(int*)&value[8]);
            sendTLV(client, STATUS_RESPONSE, "CONFIG OK");
            break;
        }
        case DATA: {
            int count = ctx.rows * ctx.cols;
            if (length != count * sizeof(int)) {
                sendTLV(client, ERROR_TAG, "Invalid DATA length");
                break;
            }
            ctx.matrix.resize(count);
            for (int i = 0; i < count; ++i) {
                int val;
                memcpy(&val, &value[i * 4], 4);
                ctx.matrix[i] = ntohl(val);
            }
            sendTLV(client, STATUS_RESPONSE, "DATA OK");
            break;
        }
        case START: {
            std::thread([&ctx]() { calculateMinMax(ctx); }).detach();
            sendTLV(client, STATUS_RESPONSE, "STARTED");
            break;
        }
        case STATUS_REQUEST: {
            std::string status = (ctx.status == DONE) ? "DONE" : "IN_PROGRESS";
            sendTLV(client, STATUS_RESPONSE, status);
            break;
        }
        case RESULT_REQUEST: {
            if (ctx.status != DONE) {
                sendTLV(client, ERROR_TAG, "Not ready");
                break;
            }
            std::string result = "min=" + std::to_string(ctx.minVal) + "; max=" + std::to_string(ctx.maxVal);
            sendTLV(client, RESULT_RESPONSE, result);
            break;
        }
        default:
            sendTLV(client, ERROR_TAG, "Unknown tag");
        }
    }

    closesocket(client);
    clientTasks.erase(client);
    std::cout << "Client disconnected.\n";
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Server started on port " << PORT << "\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        std::cout << "New client connected.\n";
        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
