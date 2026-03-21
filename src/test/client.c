#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @brief An simple test for package data in c and python.
 * server.py is the server side and client.c is client.
 * memory alignment is 1 bit, default is 4 bit.
 */
enum MyEnum {
    MSG_CMD = 1,
    MSG_FRAME = 2,
};

#pragma pack(1)
struct MyStruct {
    int32_t len;
    uint8_t type;
    uint8_t c;
    uint16_t d;
    uint16_t e;
    uint8_t payload[0];
};
#pragma pack()

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    struct MyStruct recv_data;
    struct MyStruct send_data;



    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(10086);

    // Convert IPv4 and IPv6 addresses from text to binary form
    const char* ipaddr = "127.0.0.1";
    if (argc > 1)
    {
        ipaddr = argv[1];
    }

    if(inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    memset(&send_data, 0, sizeof(struct MyStruct));
    send_data.len = sizeof(struct MyStruct);
    send_data.type = MyEnum::MSG_CMD;
    send_data.c = 0xF;
    send_data.d = 0xF;
    send_data.e = 0x2f;
    size_t bytes_send = send(sock, &send_data, send_data.len, 0);
    if (bytes_send != sizeof(struct MyStruct)) {
        printf("Send data failed\n");
        return -1;
    }
    printf("Send data: %d\n", bytes_send);

    // Receive data
    ssize_t bytes_received = recv(sock, &recv_data, 1024, 0);
    if (bytes_received > 0) {
        printf("Received data: len=%llu, a=%u, b=%u, c=%u, d=%u, e=%u\n",
            bytes_received, recv_data.len, recv_data.type, recv_data.c, recv_data.d, recv_data.e);
    }

    // Close socket
    close(sock);
    return 0;
}
