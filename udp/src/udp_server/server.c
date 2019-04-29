#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */
#include "string.h"

#define MAXBUF 1024*1024

void uppercase(char *p) {
  for ( ; *p; ++p) *p = toupper(*p);
}

void echo(int sd) {
    char bufin[MAXBUF];
    struct sockaddr_in remote;
    int num = 1;
    char tmp[2];

    /* need to know how big address struct is, len must be set before the
       call to recvfrom!!! */
    socklen_t len = sizeof(remote);

    while (1) {
      memset(tmp , 0, strlen(tmp));
      memset(bufin , 0, strlen(bufin));
      //strcpy(tmp, "");
      //strcpy(bufin, "");
      /* read a datagram from the socket (put result in bufin) */
      int n = recvfrom(sd, bufin, MAXBUF, 0, (struct sockaddr *) &remote, &len);

      if (n < 0) {
        perror("Error receiving data");
      } else {
	printf("The server got No.%d message '%s' from client.\n",num,bufin);
        // uppercase(bufin);
        sprintf(tmp, "%d", num++);
	strcat(bufin, " VS ");
        strcat(bufin, tmp);
	strcat(bufin, " in server\n");
        /* Got something, just send it back */
        sendto(sd, bufin, n+strlen(bufin), 0, (struct sockaddr *)&remote, len);
      }
    }
}

/* server main routine */

int main() {
  int ld;
  struct sockaddr_in skaddr;
  socklen_t length;

  /* create a socket
     IP protocol family (PF_INET)
     UDP protocol (SOCK_DGRAM)
  */

  if ((ld = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  /* establish our address
     address family is AF_INET
     our IP address is INADDR_ANY (any of our IP addresses)
     the port number is 9876
  */

  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(9876);

  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
    printf("Problem binding\n");
    exit(0);
  }

  /* find out what port we were assigned and print it out */

  length = sizeof(skaddr);
  if (getsockname(ld, (struct sockaddr *) &skaddr, &length) < 0) {
    printf("Error getsockname\n");
    exit(1);
  }

  /* Go echo every datagram we get */
  echo(ld);
  return 0;
}
