import socket
import struct
import time
import random

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

NUM_ROWS = 1000
NUM_COLUMNS = 1000
MAX_VALUE = 10000
NUM_THREADS = 5

CONFIG = 0x01
DATA = 0x02
START = 0x03
STATUS_REQUEST = 0x04
RESULT_REQUEST = 0x05
STATUS_RESPONSE = 0x06
RESULT_RESPONSE = 0x07
ERROR_TAG = 0x08


def send_tlv(sock: socket.socket, tag: int, value: bytes):
    length = len(value)
    header = struct.pack('!BI', tag, length)
    sock.sendall(header + value)


def recv_all(sock: socket.socket, length: int) -> bytes:
    data = b''
    while len(data) < length:
        more = sock.recv(length - len(data))
        if not more:
            raise ConnectionError("Socket closed unexpectedly")
        data += more
    return data


def send_config(sock: socket.socket, rows: int, cols: int, threads: int):
    value = struct.pack('!III', rows, cols, threads)
    send_tlv(sock, CONFIG, value)


def send_data(sock: socket.socket, matrix: list[int]):
    packed = b''.join(struct.pack('!I', val) for val in matrix)
    send_tlv(sock, DATA, packed)


def send_start(sock: socket.socket):
    send_tlv(sock, START, b'')


def send_status_request(sock: socket.socket):
    send_tlv(sock, STATUS_REQUEST, b'')


def send_result_request(sock: socket.socket):
    send_tlv(sock, RESULT_REQUEST, b'')


def receive_response(sock: socket.socket):
    header = recv_all(sock, 5)
    tag, length = struct.unpack('!BI', header)
    value = recv_all(sock, length)
    content = value.decode('utf-8', errors='replace')
    if tag in (STATUS_RESPONSE, RESULT_RESPONSE):
        print(f"[SERVER]: {content}")
    elif tag == ERROR_TAG:
        print(f"[ERROR]: {content}")


def main():
    matrix = [random.randint(0, MAX_VALUE) for _ in range(NUM_ROWS * NUM_COLUMNS)]

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((SERVER_IP, SERVER_PORT))

        send_config(sock, NUM_ROWS, NUM_COLUMNS, NUM_THREADS)
        receive_response(sock)

        send_data(sock, matrix)
        receive_response(sock)

        send_start(sock)
        receive_response(sock)

        while True:
            time.sleep(1)
            send_status_request(sock)
            try:
                header = recv_all(sock, 5)
            except ConnectionError:
                print("Connection closed while waiting for status")
                break

            tag, length = struct.unpack('!BI', header)
            val = recv_all(sock, length)
            status = val.decode('utf-8', errors='replace')
            print(f"[STATUS]: {status}")
            if status == "DONE":
                break

        send_result_request(sock)
        receive_response(sock)


if __name__ == "__main__":
    main()
