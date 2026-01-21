# Redis Remake in C/C++

This project is a recreation of [Redis](https://redis.io/) - a in-memory database, implemented in C/C++. This repo documents my learning journey of low-level systems programming and network server development. It is structured in stages, and each stage is an extension of the previous one.

## Project Goal

The primary goal of this project is to understand how a high-performance server with non-blocking IO works.

## Project Structure

The project is divided into several stages, each stage builds upon the previous one, introducing new features. The final goal is to have a simple in-memory key-value store server, capable of serving multiple clients concurrently using non-blocking I/O.

### [Stage 1: Simple Client-Server](./01_simple_client-server/)

A basic, iterative TCP server and a client that can exchange single messages.

**Concepts:** Basic socket API (`socket`, `bind`, `listen`, `accept`), blocking I/O, simple request-response loop.


### [Stage 2: Basic Protocol & Connection Reuse](./02_basic_protocol_single_TCP/)

The client and server are improved to handle multiple requests over a single, persistent TCP connection. A simple length-prefixed protocol is introduced to frame messages and handle partial reads/writes.

**Concepts:** Persistent connections, message framing, I/O wrappers (`read_n`, `write_n`).

### [Stage 3: Event-Based Concurrency](./03_event-based_concurrency/)

The server is rewritten in C++ to be fully concurrent without using threads. It uses non-blocking I/O and the `poll()` system call to handle many clients in a single event loop.

**Concepts:** Event-driven architecture, non-blocking I/O, readiness notification API (with `poll()`).

## Useful resources
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Linux Programming Interface](https://man7.org/tlpi/)
