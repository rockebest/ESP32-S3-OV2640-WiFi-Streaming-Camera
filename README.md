# ESP32-S3 OV2640 Wi-Fi Streaming Camera

ESP32-S3 WROOM N16R8 CAM + OV2640 카메라를 이용하여 JPEG 프레임을 Wi-Fi TCP로 전송하고, PC의 Python/OpenCV에서 실시간으로 표시하는 간단한 무선 카메라 스트리밍 프로젝트입니다.

This project turns an ESP32-S3 WROOM N16R8 CAM with an OV2640 camera module into a simple Wi-Fi streaming camera.  
The ESP32-S3 captures JPEG frames and sends them to a PC over Wi-Fi using a TCP socket. A Python/OpenCV viewer receives and displays the stream in real time.

---

## System Overview

```text
ESP32-S3 CAM OV2640
        ↓ Wi-Fi TCP
Python / OpenCV Viewer
        ↓
PC screen
```

The stream format is:

```text
"FRAM" + 4-byte JPEG size + JPEG binary data
```

UDP discovery is also included, so the Python viewer can automatically find the ESP32-S3 camera on the same network.

---

## Features

- ESP32-S3 + OV2640 camera support
- JPEG frame capture
- Wi-Fi TCP streaming
- Python/OpenCV real-time viewer
- UDP discovery for automatic IP detection
- Manual IP fallback option
- QVGA/VGA configurable image quality

---

## Hardware

- ESP32-S3 WROOM N16R8 CAM
- OV2640 camera module
- 2.4 GHz Wi-Fi network
- PC or Mac with Python

> Note: ESP32-S3 generally supports 2.4 GHz Wi-Fi, not 5 GHz Wi-Fi.  
> Make sure the ESP32-S3 and the PC are on the same network.
images/esp32_s3_ov2640_board.png
 
---

## Arduino Requirements

Install the ESP32 board package in Arduino IDE.

The sketch uses:

- `WiFi.h`
- `WiFiUdp.h`
- `esp_camera.h`

Recommended Arduino IDE settings:

```text
Board: ESP32S3 Dev Module
Flash Size: 16MB
PSRAM: OPI PSRAM / Enabled
USB CDC On Boot: Enabled
Partition Scheme: Huge APP or 16MB Flash scheme
```

---

## Python Requirements

Install the required Python packages:

```bash
python3 -m pip install opencv-python numpy
```

On Windows:

```bash
python -m pip install opencv-python numpy
```

---

## Usage

### 1. Upload ESP32-S3 camera firmware

Open:

```text
esp32_s3_cam_stream/esp32_s3_cam_stream.ino
```

Set your 2.4 GHz Wi-Fi SSID and password:

```cpp
const char* ssid = "YOUR_2_4GHz_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

Upload the sketch to the ESP32-S3 CAM.

Open Serial Monitor at `115200 baud`. You should see something like:

```text
Wi-Fi connected.
IP address: 192.168.x.x
Camera TCP port: 3333
UDP discovery started.
Ready.
```

### 2. Run Python viewer

Run:

```bash
python3 python_viewer/esp32_wifi_camera_viewer.py
```

On Windows:

```bash
python python_viewer/esp32_wifi_camera_viewer.py
```

If discovery works, the viewer will automatically connect to the ESP32-S3 camera.

---

## Manual IP mode

If UDP discovery does not work on your network, open:

```text
python_viewer/esp32_wifi_camera_viewer.py
```

and set:

```python
MANUAL_IP = "192.168.x.x"
MANUAL_PORT = 3333
```

Use the IP address shown in the ESP32 Serial Monitor.

---

## Image quality settings

In the Arduino sketch, you can adjust:

```cpp
config.frame_size   = FRAMESIZE_QVGA;  // 320x240
config.jpeg_quality = 10;              // lower = better quality
```

Recommended settings:

```cpp
// Balanced
config.frame_size   = FRAMESIZE_QVGA;
config.jpeg_quality = 10;

// Higher quality but slower
config.frame_size   = FRAMESIZE_VGA;
config.jpeg_quality = 12;

// Faster but lower quality
config.frame_size   = FRAMESIZE_QQVGA;
config.jpeg_quality = 14;
```

---

## Troubleshooting

### ESP32 cannot connect to Wi-Fi

- Use a 2.4 GHz Wi-Fi network.
- Check SSID and password.
- Make sure the board is close enough to the router.

### Python cannot find ESP32-S3 camera

- Make sure PC and ESP32-S3 are on the same network.
- Some school/company Wi-Fi networks block UDP broadcast.
- Use manual IP mode if discovery fails.

### OpenCV window does not appear

Install dependencies again:

```bash
python3 -m pip install opencv-python numpy
```

### Camera image is blurry

- Adjust the OV2640 lens focus manually.
- Try VGA mode if Wi-Fi speed is sufficient.
- Improve lighting.

---

## Roadmap

Possible future extensions:

- Face detection with OpenCV
- Face-ratio based distance estimation
- ESP32 display receiver
- Robot or smart-home device integration
- Web browser MJPEG viewer

---

## License

MIT License
