# Stage 1: Simple Client-Server Communication

This directory contains the first stage of the Redis Remake project: a basic client-server application demonstrating simple TCP communication.

The goal is to create programs capable of exchanging data between each other in the simplest way, based on low-level socket programming concepts.

## What it does
- **Server**: listens on a fixed port,handles one client at a time with blocking I/O.
- **Client**: connects to the server, sends a single message, receives a response, closes the connection.
- Each connection uses a **separate TCP connection** - no connection reuse.

## Concepts

*   **Socket Programming:** Usage of the basic socket API (`socket`, `bind`, `listen`, `accept`, `connect`, `read`, `write`) to establish TCP connections.
*   **Blocking I/O:** The client and server use blocking calls to read and write data.
*   **Simple Data Exchange:** A basic protocol where the client sends a single message and the server sends a single response.

## Build & run

```bash
# build
make

# terminal 1: run server
./server

# terminal 2: run client
./client
```

## Limitations (intentional)
- **Single Request/Response:** The  server can only handle one request and one response per connection.
- **One request** per connection
- **No handling** of partial reads/writes.