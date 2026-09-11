 QNX IMX708 Camera Streaming

A QNX-based camera streaming application for capturing frames from an IMX708 camera and transmitting them over UDP to a remote receiver.

Overview

The application uses the QNX Camera API to capture frames from the IMX708, copies the camera buffers into application-managed frame buffers, queues them for processing, packetizes the frames, and sends them over UDP.

```text
IMX708
  ↓
QNX Camera API
  ↓
Frame Capture
  ↓
Frame Queue
  ↓
Worker
  ↓
UDP Packetization
  ↓
Network
  ↓
Receiver

Project Structure

qnx_camera/
├── src/
│   ├── camera_example1_callback.c   # Camera initialization and capture
│   ├── Frame.h                      # Frame definition
│   ├── FrameQueue.c/.h               # Thread-safe frame queue
│   ├── FrameUtils.c/.h               # Frame buffer handling
│   ├── Packet.c/.h                   # UDP packet format
│   ├── UdpSender.c/.h                # UDP transmission
│   └── worker.c/.h                   # Frame processing thread
├── build/                            # Build output
├── Makefile
├── .project
└── .cproject

Requirements

QNX SDP 8.x

QNX Momentics

AArch64 QNX target

Raspberry Pi with IMX708 camera

Network connection between sender and receiver


Build

The project is configured for the aarch64le QNX target.

make

For a clean rebuild:

make clean
make

The executable is generated under:

build/aarch64le-debug/

Running

Copy the executable to the QNX target and select the available camera unit:

./imx

Then run:

./imx -u 1

-u 1 selects CAMERA_UNIT_1.

UDP Streaming

The sender transmits captured frames using UDP.

Default configuration:

Destination: 10.0.0.2
Port:        5000
Payload:     1400 bytes

The receiver must listen on the configured UDP port and be reachable from the QNX target.

Current Status

IMX708 camera capture: Working

QNX camera integration: Working

Frame queue and processing: Working

UDP streaming: Working

Multiple receivers: Planned


License

This project is intended for development and experimental use.
