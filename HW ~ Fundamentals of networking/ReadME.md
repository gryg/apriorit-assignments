# Simple C++ TCP Client-Server

This is a basic implementation of a TCP client-server application in C++ for Linux environments using POSIX sockets (`socket.h`) and `std::thread`.

## Description

* **Server:** Listens on a specified port (default 8080) for incoming TCP connections. It can handle multiple clients concurrently, each in its own thread.
    * If a client sends the exact message "hello", the server responds with "world".
    * For any other message, the server echoes the message back to the client.
* **Client:** Connects to the server at a specified IP address (default 127.0.0.1) and port. It allows the user to type messages and send them to the server. Responses from the server are displayed. The connection remains active until the user types "disconnect".

## Features

* TCP/IP communication.
* Multi-client server using `std::thread` for concurrency.
* Specific "hello" -> "world" message handling.
* General message echoing.
* Resource management (sockets are closed).
* Graceful server shutdown on SIGINT (Ctrl+C).
* Graceful client disconnect using the "disconnect" command.
* Makefile for easy building.
* Written in modern C++ (C++17).

## Requirements

* A Linux environment (or compatible POSIX system with socket support).
* A C++ compiler supporting C++17 (e.g., g++).
* `make` build utility.
* Standard C/C++ libraries and POSIX threads (`pthread`).

## Building

Use the provided Makefile to build the server and client executables:

```bash
# Build both server and client
make all

# Build only the server
make server

# Build only the client
make client

# Remove built files
make clean