//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//                        Main class of the FTP server
//
//****************************************************************************

#include "FTPServer.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <list>

#include "ClientConnection.h"
#include "common.h"

int define_socket_TCP(int port) {
  struct sockaddr_in sin;
  int s, type;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    errexit("No puedo crear el socket: %s\n", strerror(errno));
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    errexit("No puedo hacer el bind con el puerto: %s\n", strerror(errno));
  }

  if (listen(s, 5) < 0)
    errexit("Fallo en el listen: %s\n", strerror(errno));

  return s;
}

// This function is executed when the thread is executed.
void *run_client_connection(void *c) {
  ClientConnection *connection = (ClientConnection *)c;
  connection->WaitForRequests();

  return NULL;
}

FTPServer::FTPServer(int port) {
  this->port = port;
}

// Parada del servidor.
void FTPServer::stop() {
  close(msock);
  shutdown(msock, SHUT_RDWR);
}

// Starting of the server
void FTPServer::run() {
  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);
  msock = define_socket_TCP(port);  // This function must be implemented by you.
  while (1) {
    pthread_t thread;
    ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
    if (ssock < 0)
      errexit("Fallo en el accept: %s\n", strerror(errno));

    ClientConnection *connection = new ClientConnection(ssock);

    // Here a thread is created in order to process multiple
    // requests simultaneously
    pthread_create(&thread, NULL, run_client_connection, (void *)connection);
  }
}
