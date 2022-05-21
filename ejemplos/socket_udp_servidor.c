
#include <stdio.h>

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 

#include "common.h"



int define_socket_UDP(int port) {
    struct sockaddr_in sin;
   
    int     s, type;
    
    s = socket(AF_INET,SOCK_DGRAM, 0);
    
    if(s < 0) {
            errexit("No puedo crear el socket: %s\n", strerror(errno));
    }
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    
    if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	    errexit("No puedo hacer el bind con el puerto: %s\n", strerror(errno));
    }
    
    return s;

}

int main(int argc, char **argv) {
  
  int port = -1;
  char buf[1];
  time_t now;
  struct sockaddr_in fsin;
  
  // Si se pasa un número de puerto por parámetro se usa ese.
  // Si no usamos el 3300.
  if (argc == 2) {
      port == atoi(argv[1]);
  }
  else {
      port = 3300;
  }
  
  printf("Esperando conexiones por el puerto: %d\n", port);
  
  // Definimos el socket
  int s = define_socket_UDP(port);
  
  while (1) {
       socklen_t alen = sizeof(fsin);
       // Espera a que el cliente envíe un byte.
       if(recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen) < 0)
	  errexit("recvfrom: %s\n", strerror(errno));
       
        // Leemos el tiemp del reloj del sismtema
        time(&now);
        now = htonl((ulong) now);
       
        // Enviamos el texto con la hora, fecha, etc al cliente a través del socket.
        sendto(s, &now, sizeof(now), 0, (struct sockaddr *)&fsin, sizeof(fsin));
  }
  
}
