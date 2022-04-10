#include <cstdlib>
#include "Server.h"
#include "iostream"

#define PORT 8888
#define MAX_CLIENTS 10000

using namespace std;

int main() {
    Server httpServer = Server(PORT, MAX_CLIENTS);
    cout << "starting the http server on port " << PORT << endl;
    httpServer.serve();
    return EXIT_SUCCESS;
}