# Network Programs Repository

This repository contains three separate network programs developed for various networking tasks. Each program serves a different purpose and can be used independently based on your networking requirements.

## Programs

### 1. Chat Client

The **Chat Client** program allows users to connect to a chat server using either UDP or TCP protocols and communicate with other users. It provides a simple interface for sending and receiving messages in real-time.

- **Usage**: Run the chat client executable and specify the chat server's IP address and port number. Choose the desired protocol (UDP or TCP) for communication.
- **Features**:
  - Connect to a chat server via UDP or TCP.
  - Send and receive messages in real-time.
  - Simple command-line interface.

### 2. Packet Sniffer

The **Packet Sniffer** program is designed to capture and analyze network packets flowing through a network interface. It provides the ability to filter packets based on various criteria, such as source/destination IP addresses, protocols, ports, etc.

- **Usage**: Execute the packet sniffer program and specify the network interface to monitor. Optionally, apply filters to capture specific types of packets.
- **Features**:
  - Capture and analyze network packets.
  - Filter packets based on specific criteria.
  - View packet details, including headers and payload.

### 3. Radix Server

The **Radix Server** program, developed using Codecraftersd, is a lightweight and efficient server implementation based on the radix tree data structure. It provides a scalable solution for handling key-value pair storage and retrieval over a network.

- **Usage**: Run the radix server executable and configure the server settings as required. Use client programs to interact with the server for storing and retrieving data.
- **Features**:
  - Efficient storage and retrieval of key-value pairs.
  - Scalable server architecture using radix tree.
  - Support for concurrent client connections.
