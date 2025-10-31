#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> // superset of previous

// ------ Helpers ------

// msg prints a message
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// die prints OS error info (errno) and exits
void die(const char *msg) {
    perror(msg);
    exit(1);
}

// ------ Read & Write ------

// connfd is a new file descriptor representing an established TCP connection
static void do_something(int connfd) {
    char rbuf[64] = {};

    // read() is a blocking syscall by default
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";

    // Copies bytes from buffer into the kernel’s send buffer for that connection and returns how many were queued
    write(connfd, wbuf, strlen(wbuf));
}

// ------ Creating a TCP Server ------

int main() {

    // ------ Obtaning a Socket Handle ------

    /* socket() syscalls (system calls) have 3 integer arguments.
     * AF_INET is for IPv4. (AF_INET6 for IPv6 or dual-stack sockets)
     * SOCK_STREAM is for TCP. (SOCK_DGRAM for UDP)
     * 3rd arguments is 0 and  useless for this purpose.
     */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket");

    // ------ Setting Socket Options ------

    // Passing a parameter to the OS since the actual socket hasn't been created yet.
    // 2nd and 3rd argument specify which option to set
    // 4th. is the option value (needed)
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // ------ Binding to an Address ------

    // We’ll bind to the wildcard address 0.0.0.0:1234. This is just a parameter for listen()
    /* struct sockaddr_in holds an IPv4:port pair stored as big-endian numbers, converted by htons() and htonl(). 
     * For example, 1.2.3.4 is represented by htonl(0x01020304).
     */
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);        // port
    addr.sin_addr.s_addr = htonl(0);    // wildcard IP 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    // Listen
    // Should be written after bind() if not the socket won't accepet connections
    rv = listen(fd, SOMAXCONN);
    if (rv) { die("listen()"); }

    // ------ Accept Connections ------

    while(true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue;   // error
        }

        do_something(connfd);
        close(connfd);
    }
    return 0;
}


