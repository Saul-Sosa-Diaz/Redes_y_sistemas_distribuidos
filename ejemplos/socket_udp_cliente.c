
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


int conectarUDP(char *host, int port) {
    struct sockaddr_in sin;
    struct hostent *hent;
    int s;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    
    if(hent = gethostbyname(host)) 
        memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
    else if ((sin.sin_addr.s_addr = inet_addr((char*)host)) == INADDR_NONE)
         errexit("No puedo resolver el nombre \"%s\"\n", host);
    
     
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0)
       errexit("No se puede crear el socket: %s\n", strerror(errno));

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
       errexit("No se puede conectar con %s: %s\n", host, strerror(errno));

    return s;

}

int main(int argc, char *argv[ ])

{

   char byte=0;
   int s = -1;
   time_t  now;
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
    s = conectarUDP(host, puerto);
    
    // Escibimos un byte para pedir la hora al servidor
    send(s,&byte, 1, 0);
    
    // Leemos el entero con la hora del servidor
    n = recv(s, (char *)&now, sizeof(now),0);
    if(n < 0)
        errexit("Error de lectura: %s\n", strerror(errno));

    // Reordenamos los bytes.
    now = ntohl((u_long)now);

    // Imprimimos la hora por pantalla.
    printf("%s", ctime(&now));
    close(s);

    exit(0);

}