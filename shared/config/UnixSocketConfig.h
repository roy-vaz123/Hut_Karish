#pragma once

#include <string>
#include <sys/socket.h> // socket functions (accept, bind) and constants
#include <sys/un.h>// unix domain socket adress structure (sockaddr_un)
#include <cstring> // string functions
#include <iostream>// debug, might move later
#include <unistd.h>// files functions (close, unlink, read, write) 
#include <cstdint> // for uint16_t
#include <sys/types.h> // for pid_t

// The stardard location for unix domain sockets used by daemons
// cleaned on boot if not cleaned by the daemon
#define SOCKET_FILE_ADRESS "/var/run/hut_karish-daemon.sock"