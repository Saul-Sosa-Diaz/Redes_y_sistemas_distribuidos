#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "common.h"

int conectarTCP(char *host, int port) {
  struct sockaddr_in sin;
  struct hostent *hent;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  if (hent = gethostbyname(host))
    memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
  else if ((sin.sin_addr.s_addr = inet_addr((char *)host)) == INADDR_NONE)
    errexit("No puedo resolver el nombre \"%s\"\n", host);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    errexit("No se puede crear el socket: %s\n", strerror(errno));

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("No se puede conectar con %s: %s\n", host, strerror(errno));

  return s;
}

int main(int argc, char *argv[]) {
  int s = -1;
  int cuenta = -1;
  char *host;
  int puerto;
  int n;

  if (argc != 3) {
    printf("Número de argumentos incorrecto:\n");
    printf("%s <host> <puerto>\n", argv[0]);
    exit(-1);
  }

  host = argv[1];
  puerto = atoi(argv[2]);
  // Conectamos con el servidor usando un socket UDP
  s = conectarTCP(host, puerto);

  while (1) {
    if (recv(s, (char *)&cuenta, sizeof(int), 0) != sizeof(int)) {
      errexit("Error de lectura\n");
    }
    int cuenta_reord = ntohl(cuenta);
    printf("\r%02d", cuenta_reord);
    fflush(stdout);

    if (cuenta_reord == 0) {
      close(s);
      break;
    }
  }
}