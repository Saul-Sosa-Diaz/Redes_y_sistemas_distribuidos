//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//              This class processes an FTP transactions.
//
//****************************************************************************

#include "ClientConnection.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "FTPServer.h"
#include "common.h"

ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL) {
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return;
  }

  ok = true;
  data_socket = -1;
  parar = false;
  passive = false;
};

ClientConnection::~ClientConnection() {
  fclose(fd);
  close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port) {
  // Implement your code to define a socket here
  struct sockaddr_in sin;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  auto tamano_direccion = sizeof(address);
  memcpy(&sin.sin_addr, &address, tamano_direccion);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    errexit("No se puede crear el socket: %s\n", strerror(errno));

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("No se puede conectar con %s: %s\n", address, strerror(errno));

  return s;  // You must return the socket descriptor.
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd) == 0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }

  fprintf(fd, "220 Service ready\n");

  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {  //<-----------------------------------------------------------------------
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    } else if (COMMAND("PWD")) {  //<-----------------------------------------------------------------------
      auto path = get_current_dir_name();
      fprintf(fd, "257 %s current working directory\n", path);
    } else if (COMMAND("PASS")) {  //<-----------------------------------------------------------------------
      fscanf(fd, "%s", arg);
      if (strcmp(arg, "1234") == 0) {
        fprintf(fd, "230 User logged in\n");
      } else {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    } else if (COMMAND("PORT")) {  //<-----------------------------------------------------------------------
      /*The argument is a HOST-PORT specification for the data port
            to be used in data connection. The fields are separated by commas.
            A port command would be:
            PORT h1,h2,h3,h4,p1,p2
            */
      int h1, h2, h3, h4, p1, p2;

      fscanf(fd, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

      uint32_t address = h4 << 24 | h3 << 16 | h2 << 8 | h1;
      uint16_t port = p1 << 8 | p2;

      data_socket = connect_TCP(address, port);

      fprintf(fd, "200 OK.\n");

    } else if (COMMAND("PASV")) {  //<-----------------------------------------------------------------------
      int s = define_socket_TCP(0);
      struct sockaddr_in socket;
      socklen_t longitud = sizeof(socket);
      int socket_client = getsockname(s, (struct sockaddr *)&socket, &longitud);

      uint16_t port = socket.sin_port;

      int p1 = port >> 8;    // pick the last 8 bits
      int p2 = port & 0xFF;  // pick the first 8 bits

      if (socket_client < 0) {
        fprintf(fd, "421 Service not available, closing control connection.\n");
        fflush(fd);
        return;
      }

      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p2);
      fflush(fd);

      data_socket = accept(s, (struct sockaddr *)&socket, &longitud);
      passive = true;

    } else if (COMMAND("STOR")) {  //<-----------------------------------------------------------------------
      fscanf(fd, "%s", arg);
      FILE *file = fopen(arg, "wb");
      if (!file) {
        fprintf(fd, "450 Requested file action not taken.\n");
        close(data_socket);
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);
        struct sockaddr_in sa;
        socklen_t sa_len = sizeof(sa);
        char buffer[MAX_BUFF];
        int bytes_recived;

        if (parar)
          data_socket = accept(data_socket, (struct sockaddr *)&sa, &sa_len);
        do {
          bytes_recived = recv(data_socket, buffer, MAX_BUFF, 0);
          fwrite(buffer, sizeof(char), bytes_recived, file);
          // Menor que el máximo es el último paquete
        } while (bytes_recived == MAX_BUFF);
        fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
        fflush(fd);
        fclose(file);
        close(data_socket);
      }

    } else if (COMMAND("RETR")) {  //<-----------------------------------------------------------------------
      fscanf(fd, "%s", arg);
      FILE *file = fopen(arg, "rb");
      if (!file) {
        fprintf(fd, "450 Requested file action not taken.\nFile unavailable.\n");
        close(data_socket);
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);

        size_t bytes_recived = 0;
        char buffer[MAX_BUFF];

        do {
          bytes_recived = fread(buffer, 1, MAX_BUFF, file);
          // Mandar al cliente el archivo
          send(data_socket, buffer, bytes_recived, 0);

        } while (bytes_recived == MAX_BUFF);

        fprintf(fd, "226  Closing data connection.\n");
        fclose(file);
        close(data_socket);
      }

    } else if (COMMAND("LIST")) {  //<-----------------------------------------------------------------------

      fprintf(fd, "125 Data connection already open; transfer starting.\n");
      fflush(fd);

      DIR *directory = opendir(get_current_dir_name());
      struct dirent *directory_element;
      char buffer[MAX_BUFF];
      size_t size;

      if (directory == NULL) {
        fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
        close(data_socket);
      } else {
        // Ir metiendo contenido del directorio en un buffer y luego mandarlo
        while ((directory_element = readdir(directory)) != NULL) {
          size = sprintf(buffer, "%s\n", directory_element->d_name);
          send(data_socket, buffer, size, 0);
        }
      }

      fprintf(fd, "250 List completed successfully.\n");
      fflush(fd);
      closedir(directory);
      close(data_socket);

    } else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");
    } else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK\n");
    } else if (COMMAND("QUIT")) {
      fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
      close(data_socket);
      parar = true;
      break;
    } else {
      fprintf(fd, "502 Command not implemented.\n");
      fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }

  fclose(fd);

  return;
};
