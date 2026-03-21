import socket
import ctypes
import sys
import logging
import time

# Enum definition
class MyEnum:
    MSG_CMD = 1
    MSG_FRAME = 2

class TT:
    def __init__(self):
        self.a = 0
        self.b = 0

    staticmethod
    def from_buffer_copy(cls, buffer):
        instance = cls()
        instance.a = ctypes.c_uint8.from_buffer_copy(buffer, 0).value
        instance.b = ctypes.c_uint8.from_buffer_copy(buffer, 1).value
        return instance

    def __str__(self):
        return f"TT(a={self.a}, b={self.b})"

# Match the C struct with 1-byte alignment
class MyStruct(ctypes.Structure):
    _fields_ = [
        ('len', ctypes.c_int32),
        ('type', ctypes.c_uint8),
        ('c', ctypes.c_uint8),
        ('d', ctypes.c_uint16),
        ('e', ctypes.c_uint16)
    ]
    _pack_ = 1

    def __str__(self):
        return f"MyStruct(len={self.len}, type={self.type}, c=0x{self.c:02x}, d=0x{self.d:04x}, e=0x{self.e:04x})"

def main():
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='[%(asctime)s.%(msecs)03d] %(levelname)s %(message)s',
        datefmt='%m-%d %H:%M:%S'
    )

    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Get server address from command line or use default
    host = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
    port = 10086

    try:
        # Connect to server
        sock.connect((host, port))
        logging.info(f"Connected to {host}:{port}")

        index = 0
        while index < 10:
            # Prepare and send data
            index += 1
            send_data = MyStruct()
            send_data.len = ctypes.sizeof(MyStruct)
            send_data.type = MyEnum.MSG_CMD
            send_data.c = 0xF
            send_data.d = 0xFF
            send_data.e = 0x2F

            # Convert struct to bytes and send
            bytes_to_send = bytes(send_data)
            bytes_sent = sock.send(bytes_to_send)
            logging.info(f"Sent {bytes_sent} bytes")

            # Receive response
            recv_buffer = sock.recv(1024)
            if recv_buffer:
                # Convert received bytes to struct
                recv_data = MyStruct.from_buffer_copy(recv_buffer[:ctypes.sizeof(MyStruct)])
                logging.info(f"Received: {recv_data}")

                # If there's additional data after the struct
                if len(recv_buffer) > ctypes.sizeof(MyStruct):
                    additional_data = recv_buffer[ctypes.sizeof(MyStruct):]
                    logging.info(f"{index}, Additional data: {additional_data.hex()}")
            time.sleep(1)

    except ConnectionError as e:
        logging.error(f"Connection error: {e}")
        return 1
    except Exception as e:
        logging.error(f"Error: {e}")
        return 1
    finally:
        sock.close()
        logging.info("Connection closed")

    return 0

if __name__ == "__main__":
    sys.exit(main())
