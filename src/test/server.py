import socket
import ctypes
import sys
import logging

#  @brief An simple test for package data in c and python.
#  server.py is the server side and client.c is client.
#  memory alignment is 1 bit, default is 4 bits.

class MyStruct(ctypes.Structure):
    _fields_ = [('len', ctypes.c_int32),
                ('type', ctypes.c_uint8),
                ('c', ctypes.c_uint8),
                ('d', ctypes.c_uint16),
                ('e', ctypes.c_uint16)
                # data[] is a 0-length array, so it doesn't take any space in the struct
                ]
    _pack_ = 1

    def __str__(self):
        return f"MyStruct(len={self.len}, b={self.type}, c=0x{self.c:04x}, d=0x{self.d:04x}, e=0x{self.e:04x})"

def receive_struct(sock):
    # Get the size of the structure
    struct_size = ctypes.sizeof(MyStruct)

    # Receive exact number of bytes
    buffer = bytearray(sock.recv(struct_size, socket.MSG_WAITALL))
    if len(buffer) < struct_size:
        logging.info(f"Received less than expected: {len(buffer)}")
        return None

    # Convert buffer to structure
    return MyStruct.from_buffer(buffer)

def start_server(host='0.0.0.0', port=10086):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Set SO_REUSEADDR option
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
    except socket.error as e:
        logging.info(f"Bind failed: {e}")
        sys.exit(1)

    try:
        server_socket.listen()
        logging.info(f"Server listening on {host}:{port}")

        while True:
            try:
                conn, addr = server_socket.accept()
                with conn:
                    logging.info(f"Connected by {addr}")
                    my_recv = receive_struct(conn)
                    if my_recv is None:
                        logging.info("Failed to receive data")
                        continue
                    logging.info(f"Received: {my_recv}")
                    my_struct = MyStruct(1, 2, 8, 9, 13)
                    data = bytes([0x01, 0x02, 0x0F, 0x04])
                    my_struct.len = ctypes.sizeof(my_struct)
                    byte_data = memoryview(my_struct).toreadonly().tobytes() + data
                    conn.sendall(byte_data)
                    logging.info(f"Sent data: {len(byte_data)}")
            except ConnectionError as e:
                logging.error(f"Connection error: {e}")
            except KeyboardInterrupt:
                logging.error("\nShutting down server...")
                break
    finally:
        try:
            server_socket.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass  # Socket already closed
        server_socket.close()
        logging.info("Server closed")


def main():
    logging.basicConfig(level=logging.INFO, datefmt='%m-%d %H:%M:%S', format='[%(asctime)s.%(msecs)03d] %(levelname)s %(message)s')
    try:
        start_server()
    except KeyboardInterrupt:
        logging.error("\nServer terminated by user")
    finally:
        return 0

if __name__ == "__main__":
    sys.exit(main())
