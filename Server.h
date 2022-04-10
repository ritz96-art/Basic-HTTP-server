//
// Created by Kunal on 25-10-2020.
//

#ifndef SERVER_EPOLL_SERVER_H
#define SERVER_EPOLL_SERVER_H

#include <map>
#include <set>

#include <fcntl.h>
#include <error.h>

#define MAX_BUFFER_SIZE 1024
using namespace std;

class Server {
private:
    int serverSocket;
    int epollId;
    std::set<int> clientSockets;
    std::map<int, std::string> clientIps;
    int serverPort, maxClients;

    static int set_nonblock(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) flags = 0;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    static void error_exit(const char *msg) {
        error(EXIT_FAILURE, errno, "%s\n", msg);
    }

    void configure_server_socket() const;

    void register_in_epoll(int sock) const;

    void register_new_client();

    void serve_client(int sock);

    string get_http_response(string msg);
public:
    Server(int port, int clients);

    [[noreturn]] void serve();
};

#endif //SERVER_EPOLL_SERVER_H
