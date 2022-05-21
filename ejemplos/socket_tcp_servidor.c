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



int define_socket_TCP(int port) {
    struct sockaddr_in sin;
   
    int     s, type;
    
    s = socket(AF_INET,SOCK_STREAM, 0);
    
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
    
    if (listen(s, 5) < 0)
        errexit("Fallo en el listen: %s\n", strerror(errno));
    
    return s;

}


void cuenta_atras (int fd) {
   int i;
   for (i = 10; i >=0; i--) {
      int cuenta = htonl(i);
      send(fd,&cuenta, sizeof(int), 0);
      printf("Contando %d\n", i);
      sleep(1);
   }
}


int main(int argc, char *argv[ ])

{
    struct sockaddr_in fsin;
    int msock, ssock;
    int alen;
    msock = define_socket_TCP(3300);
    printf("Comienza a atender peticiones\n");
    while (1) {
        ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
	printf("Acepta conexión\n");
        if(ssock < 0)
            errexit("Fallo en el accept: %s\n", strerror(errno));
	
	cuenta_atras(ssock);
        close(ssock);
    }

}