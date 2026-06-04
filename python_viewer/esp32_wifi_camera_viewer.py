import socket
import struct
import cv2
import numpy as np
import time


# ============================================================
# Settings
# ============================================================

DISCOVERY_PORT = 4210
DISCOVERY_MESSAGE = b"DISCOVER_ESP32_CAM"

# If UDP discovery does not work, set the ESP32 IP manually.
# Example:
# MANUAL_IP = "192.168.0.35"
MANUAL_IP = None
MANUAL_PORT = 3333

# Maximum allowed incoming JPEG size
MAX_JPEG_SIZE = 1_000_000


# ============================================================
# UDP Discovery
# ============================================================

def get_local_ip():
    """
    Get the current PC's local network IP address.
    Example: 192.168.0.12
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = None
    finally:
        s.close()

    return ip


def make_subnet_broadcast(ip):
    """
    Convert 192.168.0.12 to 192.168.0.255.
    This assumes a common /24 local network.
    """
    parts = ip.split(".")

    if len(parts) != 4:
        return None

    return ".".join(parts[:3]) + ".255"


def discover_esp32_camera(timeout=5):
    """
    Find the ESP32 camera by UDP broadcast.
    The ESP32 should reply with:
    ESP32CAM,<ip>,<port>
    """
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    udp.settimeout(1)

    local_ip = get_local_ip()
    targets = ["255.255.255.255"]

    if local_ip:
        subnet_broadcast = make_subnet_broadcast(local_ip)
        if subnet_broadcast:
            targets.append(subnet_broadcast)

    print("Local IP:", local_ip)
    print("Searching for ESP32-S3 camera...")

    start = time.time()

    while time.time() - start < timeout:
        for target in targets:
            try:
                udp.sendto(DISCOVERY_MESSAGE, (target, DISCOVERY_PORT))
            except Exception as e:
                print("Discovery send failed:", target, e)

        try:
            data, addr = udp.recvfrom(1024)
            text = data.decode(errors="ignore")

            print("Discovery response:", text, "from", addr)

            if text.startswith("ESP32CAM,"):
                parts = text.split(",")

                if len(parts) >= 3:
                    ip = parts[1]
                    port = int(parts[2])

                    udp.close()
                    return ip, port

        except socket.timeout:
            pass

    udp.close()
    return None, None


# ============================================================
# TCP receive helpers
# ============================================================

def read_exact(sock, n):
    """
    Read exactly n bytes from TCP socket.
    Return None if the connection fails or times out.
    """
    data = b""

    while len(data) < n:
        try:
            packet = sock.recv(n - len(data))
        except socket.timeout:
            return None
        except Exception:
            return None

        if not packet:
            return None

        data += packet

    return data


def find_header(sock):
    """
    Find the 'FRAM' header in the byte stream.
    """
    buffer = b""

    while True:
        b = read_exact(sock, 1)

        if b is None:
            return False

        buffer += b

        if len(buffer) > 4:
            buffer = buffer[-4:]

        if buffer == b"FRAM":
            return True


def receive_jpeg_frame(sock):
    """
    Receive one JPEG frame.
    Format:
    'FRAM' + 4-byte little-endian JPEG size + JPEG binary data
    """
    if not find_header(sock):
        return None

    size_bytes = read_exact(sock, 4)

    if size_bytes is None:
        return None

    size = struct.unpack("<I", size_bytes)[0]

    if size <= 0 or size > MAX_JPEG_SIZE:
        print("Invalid frame size:", size)
        return None

    jpg = read_exact(sock, size)

    if jpg is None:
        return None

    return jpg


# ============================================================
# Main
# ============================================================

def main():
    if MANUAL_IP is not None:
        esp32_ip = MANUAL_IP
        esp32_port = MANUAL_PORT
        print("Using manual IP:", esp32_ip, esp32_port)
    else:
        esp32_ip, esp32_port = discover_esp32_camera(timeout=5)

    if esp32_ip is None:
        print("ESP32-S3 camera not found.")
        print("Check Wi-Fi connection and make sure PC and ESP32 are on the same network.")
        print("If UDP discovery is blocked, set MANUAL_IP in this file.")
        return

    print("Found ESP32-S3 camera!")
    print("IP:", esp32_ip)
    print("Port:", esp32_port)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)

    print("Connecting to camera stream...")
    sock.connect((esp32_ip, esp32_port))
    print("Connected!")

    frame_count = 0
    start_time = time.time()

    try:
        while True:
            jpg = receive_jpeg_frame(sock)

            if jpg is None:
                print("Frame receive failed.")
                break

            img_array = np.frombuffer(jpg, dtype=np.uint8)
            frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

            if frame is None:
                print("JPEG decode failed.")
                continue

            frame_count += 1
            elapsed = time.time() - start_time
            fps = frame_count / elapsed if elapsed > 0 else 0

            cv2.putText(
                frame,
                f"FPS: {fps:.1f}",
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2
            )

            cv2.imshow("ESP32-S3 OV2640 Wi-Fi Camera", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    except KeyboardInterrupt:
        print("Stopped by user.")

    finally:
        sock.close()
        cv2.destroyAllWindows()
        print("Closed.")


if __name__ == "__main__":
    main()
