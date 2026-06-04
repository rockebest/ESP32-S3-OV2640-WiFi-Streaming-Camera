# System Overview

## Data Flow

```text
ESP32-S3 CAM OV2640
        |
        | JPEG frame
        | "FRAM" + 4-byte size + JPEG data
        v
Wi-Fi TCP socket
        |
        v
Python/OpenCV Viewer
        |
        v
PC Display
```

## Network

- UDP discovery port: `4210`
- TCP camera streaming port: `3333`

## Discovery Protocol

Python sends:

```text
DISCOVER_ESP32_CAM
```

ESP32 replies:

```text
ESP32CAM,<ip>,<port>
```

Example:

```text
ESP32CAM,192.168.0.35,3333
```

## Streaming Protocol

Each frame is sent as:

```text
FRAM
4-byte little-endian JPEG size
JPEG binary data
```

This simple binary protocol is easy to parse from Python, C++, or other microcontrollers.
