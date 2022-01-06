#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <string>

class server{
    public:
    int socket_fd;
    virtual ~server(){
            close(socket_fd);
        }
    void serverBuild(){
        int status;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *hostname = "ups";
        const char *port     = "12346";

        memset(&host_info, 0, sizeof(host_info));

        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags    = AI_PASSIVE;

        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0) {
            std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
            return;
        } //if

        socket_fd = socket(host_info_list->ai_family, 
                    host_info_list->ai_socktype, 
                    host_info_list->ai_protocol);
        if (socket_fd == -1) {
            std::cerr << "Error: cannot create socket" << std::endl;
            std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
            return;
        } //if

        int yes = 1;
        status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int));
        status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            std::cerr << "Error: cannot bind socket, " + std::string(strerror(errno)) << std::endl;
            std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
            return;
        } //if

        status = listen(socket_fd, 100);//need to change listen method
        if (status == -1) {
            std::cerr << "Error: cannot listen on socket" << std::endl; 
            std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
            return;
        } //if
        freeaddrinfo(host_info_list);
        
    }

    int as_server(){
        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd;
        // std::cout << "waiting to accept frontend connection" << std::endl;
        client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        // std::cout << "accept frontend connection successfully" << std::endl;
        if (client_connection_fd == -1) {
            std::cerr << "Error: cannot accept connection on socket" << std::endl;
            return -1;
        } //if
        return client_connection_fd;
    }



};