# Advanced-HTTP-Server

# HTTP Server Implementation

This project aims to develop an HTTP server with increasing functionality through successive iterations. It focuses on socket programming and threading to handle HTTP requests, implementing features such as handling various file types, supporting conditional GET requests, persistent connections, and pipelining.

## Requirements

- Python 3.x
- Flask (for testing purposes)

## Introduction

This project consists of three main tasks:

1. **HTTP Server:** Implement an HTTP 1.0 server capable of handling GET method requests for files with specific extensions (.html, .css, .js, .txt, .jpg). It should also support conditional GET requests and handle MIME types such as text/html, text/plain, text/css, text/javascript, and image/jpeg.

2. **Adding Persistence:** Extend the HTTP server to support both HTTP 1.0 and 1.1 protocols, allowing persistent connections to be maintained. Additionally, efficient memory management is required to handle open connections effectively.

3. **Adding Pipelining:** Introduce pipelining functionality to the server, enabling users to send pipelined requests that will be responded to in a pipelined fashion.

## Implementation

Three separate programs will be developed to fulfill the requirements:

- **SimpleServer.py:** Implements a basic HTTP server.
- **PersistentServer.py:** Extends the basic server to support persistent connections.
- **PipelinedServer.py:** Adds pipelining functionality to the server.

Each server will take two command-line arguments:

1. **Port #:** TCP port number for server communication.
2. **HTTP root path:** Relative or absolute file system path for serving HTTP content.

The servers must not have port numbers or HTTP root paths hardcoded, ensuring flexibility and compatibility with different environments.

## Testing

For testing, a set of sample webpages can be downloaded for evaluating the server's functionality and performance. Additionally, Flask can be used to simulate HTTP requests during development and testing stages.

## Performance Testing

The performance of each server version will be evaluated and compared, including:

- Simple HTTP server
- Persistent HTTP server
- Pipelined HTTP server
- Apache server (for reference)

httperf will be utilized for performance testing, allowing the configurable-volume of HTTP requests to be sent to the servers. The results will be analyzed and documented in a formal technical report, highlighting the differences in server performance and how the additional features affect their efficiency.

## Conclusion

This project provides an opportunity to enhance skills in socket programming, threading, and web server development while gaining insights into performance optimization and testing methodologies.


