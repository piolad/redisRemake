# Stage 2: Length-Prefixed Protocol + Connection Reuse

Compared to Stage 1, the client and server now reuse a single TCP connection for multiple request/response exchanges and a simple length-prefixed protocol is used to frame messages.

## Goal

Introduce **message framing** and handle TCP behavior (partial reads/writes) by implementing a simple **length-prefixed protocol** and I/O helper functions.

## What changed from Stage 1

- **Persistent connection:** the client sends multiple requests over the same TCP connection.
- **Framing:** each message is delimited by an explicit length header.
- **Partial I/O handling:** `read()` / `write()` may return fewer bytes than requested, so we use wrappers to read/write exactly N bytes.

## Protocol

Each message is encoded as:

```
+-----------------+-----------------+
| 4-byte length N | N bytes payload |
+-----------------+-----------------+
```

- The first 4 bytes represent the length of the payload in bytes. Typically, this is encoded as a big-endian unsigned integer (network byte order), but this project uses little-endian encoding for simplicity.
- The next N bytes are the actual message payload.
- The client can send multiple messages over the same TCP connection, and the server responds to each message in turn.

## Framing details
TCP is a stream protocol, so there are no inherent boundaries. The technique to close connection each time is not efficient.

Moreover, TCP may split messages into multiple segments, so a single `read()` call may retrieve:
- only a part of a message,
- a full single message,
- or multiple messages at once

Framing ensures reliable message reconstruction.

## Build & run

```bash
gcc -O2 -Wall -Wextra server.c -o server
gcc -O2 -Wall -Wextra client.c -o client

# terminal 1
./server

# terminal 2
./client
```