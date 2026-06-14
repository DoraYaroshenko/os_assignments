# Operating Systems Projects

A collection of Linux systems programming projects developed in C as part of the Operating Systems course at Tel Aviv University.

The repository covers core operating-system concepts including process management, networking, synchronization, device drivers, and low-level file operations.

## Projects

### Unix Shell

Implementation of a Unix-like shell supporting process creation, command execution, pipes, and signal handling.

**Topics:** Process Management, Signals, IPC

### TCP Client-Server Application

Network application implemented using POSIX sockets for reliable communication between clients and a server.

**Topics:** Networking, Sockets, Client-Server Architecture

### Linux Kernel Message Slot Driver

Linux kernel character-device driver supporting communication through configurable message channels.

**Topics:** Kernel Development, Device Drivers, ioctl

### Concurrent Queue

Thread-safe queue implementation supporting concurrent access by multiple threads.

**Topics:** Synchronization, Multithreading, Concurrency

### File Copy Utility

File-copy implementation using low-level Linux system calls together with performance measurements.

**Topics:** File Systems, System Calls, I/O Performance

## Technologies

* C
* Linux
* POSIX APIs
* Linux Kernel APIs
* Sockets
* Bash
* Make


## Repository Structure

```text
.
├── shell/
├── tcp-server-client/
├── kernel-message-slot/
├── concurrent-queue/
├── file-copy/
└── README.md
```

## Academic Context

Projects completed as part of the B.Sc. Computer Science program at Tel Aviv University.
