#include <array>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include "Server.h"

#define MAX_BUFFER_SIZE 1024

using namespace std;

void Server::configure_server_socket() const {
    struct sockaddr_in addr{};
    int opt = 1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0)
        error_exit("error while setting opt for socket");

    if (bind(serverSocket, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        error_exit("error while binding the master socket");

    if (set_nonblock(serverSocket) < 0)
        error_exit("cannot unblock the master socket");

    if (listen(serverSocket, SOMAXCONN) == -1)
        error_exit("error while listening on the master socket");
}

void Server::register_in_epoll(int sock) const {
    struct epoll_event event{};
    event.data.fd = sock;
    event.events = EPOLLIN;
    if (epoll_ctl(epollId, EPOLL_CTL_ADD, sock, &event) < 0)
        error_exit("cannot register in epoll");
}

void Server::register_new_client() {
    socklen_t addrLen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr{};
    int clientSock = accept(serverSocket, (struct sockaddr *) &addr, &addrLen);
    if (clientSock < 0)
        error_exit("cannot accept new client");

    if (set_nonblock(clientSock))
        error_exit("cannot unblock");

    clientSockets.insert(clientSock);
    string ip = inet_ntoa(addr.sin_addr);
    clientIps[clientSock] = ip;
    register_in_epoll(clientSock);
}

void Server::serve_client(int sock) {
    array<char, MAX_BUFFER_SIZE> buffer = {};
    int bytes_read = read(sock, buffer.data(), MAX_BUFFER_SIZE);
    if (bytes_read == 0) {
        close(sock);
        clientIps.erase(sock);
        clientSockets.erase(sock);
    } else {
        cout << "client ip: " << clientIps[sock] << "; message: " << buffer.data() << endl;
        string response = get_http_response(buffer.data());
        write(sock, response.c_str(), response.size());
    }
}

Server::Server(int port, int clients) {
    serverPort = port;
    maxClients = clients;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) error_exit("error while creating socket");
    configure_server_socket();
    epollId = epoll_create1(0);
    if (epollId < 0) error_exit("error while creating epoll instance");
    register_in_epoll(serverSocket);
}

[[noreturn]] void Server::serve() {
    cout << "Waiting for connections.." << endl;
    while (true) {
        struct epoll_event currentEvents[maxClients];
        int nEvents = epoll_wait(epollId, currentEvents, maxClients, -1);
        for (int i = 0; i < nEvents; ++i) {
            int sock = currentEvents[i].data.fd;
            if (sock == serverSocket) {
                // server socket gets request to join
                register_new_client();
            } else {
                // client socket is ready to something
                serve_client(sock);
            }
        }
    }
    if (shutdown(serverSocket, SHUT_RDWR) < 0)
        error_exit("error in shutdown()");

    if (close(serverSocket) < 0)
        error_exit("error in close()");
}

string Server::get_http_response(string msg) {
    string response = """HTTP/1.1 200 OK\n"""
                      "Date: Sun, 25 Oct 2020 12:28:53 GMT"
                      "Server: Apache/2.2.14 (Win32)"
                      "Content-Length: 88\n"
                      "Content-Type: text/html\n"
                      "Connection: Closed\n"
                      "<html>\n"
                      "   <body>\n"
                      "\n"
                      "   <h1>hello from epoll based http server!</h1>\n"
                      "\n"
                      "   </body>\n"
                      "</html>""";
    return response;
}