#include "server.h"


static const char *strCmdList[] = 
{
      "USER", "PASS", "RETR", "STOR", "QUIT", "SYST", "TYPE", "PASV", "PORT",
      "CWD", "CDUP", "DELE", "LIST", "MKD", "PWD", "RMD", "RNFR", "RNTO"
};

int createSocket(int port)
{
	int sock;
	int reuse = 1;

	struct sockaddr_in server_addr = (struct sockaddr_in){
		AF_INET,
		htons(port),
		(struct in_addr){INADDR_ANY}
	};
  
  //create the socket
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0)
	{
		printf("The server cannot open socket.\n");
		exit(1);
	}
	//Set address mode that can be reused right after program exits
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	//bind socket to server address
	if(bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
	{
		printf("The server cannot bind socket to the given address.\n");
		exit(1);
	}
      listen(sock, CLIENTNUM);
	return sock;
}

int acceptSocket(int sock)
{
      int conn;
      struct sockaddr_in client_addr;
      socklen_t addrlen = sizeof(client_addr);
      /* connection for data transmission */
      conn = accept(sock,(struct sockaddr*) &client_addr,&addrlen);
      return conn;
}

int connectSocket(char *hostname, int port)
{
      int dataSock;
      struct hostent *hp;

      struct sockaddr_in serveraddr;
      //create the data socket
      if ((dataSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
      {
              printf("Problem Creating Data Socket\r\n--Download Terminated\r\n");
              return -1;
      }

      if ((hp = gethostbyname(hostname)) == NULL)
             return -1;

      memset(&serveraddr, 0, sizeof(serveraddr));
      memcpy(&serveraddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
      serveraddr.sin_family = AF_INET;
      serveraddr.sin_port = htons(port);

      //connect data socket
      if (connect(dataSock, (struct sockaddr *)(&serveraddr), sizeof(struct sockaddr_in)) < 0)
      {
              printf("Data Transportation Connection Failure !\r\n");
              return -1;
      }

      return dataSock;
}


void setMessage(char* message, char* value)
{
      memset(message, 0, strlen(message));
      strcpy(message, value);   
}

void catMessage(char* message, char* value)
{
       strcat(message,value);   
}

void writeMessage(int conn, char* msg)
{
        write(conn, msg, strlen(msg));
}



void generatePort(Port* port)
{
      srand(time(NULL));
      port->p1 = 0x80 + (rand() % 0x40);//0x80:128, 0x40:64
      port->p2 = rand() % 0xff;         //0xff:255
}

int search(char* str, const char** strList, int length)
{
      int i;
      for(i = 0 ; i < length; i++)
      {
            if(strcmp(str, strList[i]) == 0)
                    return i;
      }
      return -1;
}

void handleCommand(Directive* directive, Status* status)
{
      switch(search(directive->cmd, strCmdList, sizeof(strCmdList)/sizeof(char* )))
      {
             case USER:
                cmdUSER(directive->arg, status); break;
             case PASS:
                cmdPASS(directive->arg, status); break;
             case RETR:
                 cmdRETR(directive->arg, status); break;
             case STOR:
                cmdSTOR(directive->arg, status); break;
             case QUIT:
                cmdQUIT(status); break;
             case SYST:
                cmdSYST(status); break;
             case TYPE:
                cmdTYPE(directive->arg, status); break;
             case PORT:
                cmdPORT(directive->arg, status); break;
             case PASV:
                cmdPASV(status); break;


             case CWD:
                 cmdCWD(directive->arg, status); break;
             case CDUP:
                cmdCDUP(status); break;
             case DELE:
                cmdDELE(directive->arg, status); break;
             case LIST:
                cmdLIST(directive->arg, status); break;
             case MKD:
                cmdMKD(directive->arg, status); break;
             case PWD:
                cmdPWD(status); break;
             case RMD:
                cmdRMD(directive->arg, status); break;
             case RNFR:
                cmdRNFR(directive->arg, status); break;
             case RNTO:
                cmdRNTO(directive->arg, status); break;

             default:
                setMessage(status->message,"500 Unknown command\n");   

                writeMessage(status->conn, status->message);
                break;
      }
}


void parentwait(int sig)
{
      int signum;
      wait(&signum);
}


void getIpAddr(int cmdSock, int *ipAddr)
{
      socklen_t lenAddr = sizeof(struct sockaddr_in);
      struct sockaddr_in addr;
      getsockname(cmdSock, (struct sockaddr *)&addr, &lenAddr);
      int host,i;
      host = (addr.sin_addr.s_addr);
      for(i=0; i<4; i++)
      {
            ipAddr[i] = (host>>i*8) & 0xff;//0xff: 255
      }
}

/** 
 * Converts permissions to string. e.g. rwxrwxrwx 
 * @param perm Permissions mask
 * @param str_perm Pointer to string representation of permissions
 */
void transferPermission(int perm, char *str_perm)
{
      char buf[3];
      int i, curperm = 0;
      int read = 0, write = 0, exec = 0;
      
      for(i = 6; i>=0; i-=3)
      {
              memset(buf,0,3);
              /* Explode permissions of user, group, others; starting with users */
              curperm = ((perm & ALLPERMS) >> i ) & 0x7;
            
              /* Check rwx flags for each*/
              read = (curperm >> 2) & 0x1;
              write = (curperm >> 1) & 0x1;
              exec = (curperm >> 0) & 0x1;

              sprintf(buf,"%c%c%c",read?'r':'-' ,write?'w':'-', exec?'x':'-');
              strcat(str_perm, buf);
      }
}

void startServer(int port)
{
      int fd = createSocket(port);
      struct sockaddr_in client_addr;
      socklen_t len = sizeof(client_addr);
      int conn, pid, readbytes;

      while(1)
      {
              conn = accept(fd, (struct sockaddr*) &client_addr, &len);
              if(conn < 0)
              {
                    printf("error\n");
                    continue;
              }

              char buf[MAXBUF];
              pid = fork();
              memset(buf, 0, MAXBUF);

              if(pid == 0)
              {
                    close(fd);
                    printf("The server is ready for incoming command\n");

                    Directive* directive = malloc(sizeof(Directive));
                    Status* status = malloc(sizeof(Status));

                    status->conn = conn;

                    setMessage(status->message,"220 Welcome to use qhj ftp server\n");
                    writeMessage(status->conn, status->message);

                    while((readbytes=read(conn, buf, MAXBUF)))
                    {
                          signal(SIGCHLD, parentwait);
                          if(readbytes <= MAXBUF)
                          {
                                  // parse buf
                                  sscanf(buf,"%s %s",directive->cmd, directive->arg);

                                  handleCommand(directive, status);

                                  memset(buf, 0, MAXBUF);
                                  memset(directive, 0, sizeof(*directive));
                          }
                          else
                          {
                                perror("The ftp server cannot read such large bytes at one time!");
                          }
                    }
                          printf("The Client is disconnected.\n");
                          exit(0);
              }
              else if(pid < 0)
              {
                    printf("The ftp server cannot create child process\n");
                    exit(1);
              }
              else
              {
                    printf("The parent process is running...\n");
                    close(conn);
              }
      }
}

int main(int argc, char* argv[])
{
      int port = 8889;

      startServer(port);
      return 0;
}
