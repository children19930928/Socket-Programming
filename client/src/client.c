
#include "client.h"

static const char *strCmdList[] = 
{
       "OPEN",
       "USER", "PASS", "RETR", "STOR", "QUIT", "SYST", "TYPE", "PASV", "PORT",
       "CWD", "CDUP", "DELE", "LIST", "MKD", "PWD", "RMD", "RNFR", "RNTO"
};

int sWriteMessage(int sock, char *buf, int size) 
{
       int n, total = 0, offset = 0, left = size;

       while (1) 
       {
              n = write(sock, buf + offset, left);
              if (n < 0) 
              {
                      if (errno == EINTR)
                             continue;
                      return ERROR;
              } 
              else if (n > 0) 
              {
                      left -= n;
                      offset += n;
                      total += n;
                      continue;
              } 
              else if(n == 0) 
              {
                      return total;
              }
        }
}

int sReadMessage(int sock, char *buf, int size) 
{
        int n, total = 0, offset = 0, left = size;

        while (1) 
        {
                n = read(sock, buf + offset, left);
                if (n < 0) 
                {
                      if (errno == EINTR)
                              continue;
                      return ERROR;
                } 
                else if (n > 0) 
                {
                      left -= n;
                      offset += n;
                      total += n;
                      continue;
                } 
                else if(n == 0) 
                {
                      return total;
                }
        }
}

void getIpAddr(int cmdSock, int *ipAddr)
{
        socklen_t lenAddr = sizeof(struct sockaddr_in);
        struct sockaddr_in addr;
        getsockname(cmdSock, (struct sockaddr *)&addr, &lenAddr);
        int host,i;
        host = (addr.sin_addr.s_addr);
        for(i=0; i<4; i++){
              ipAddr[i] = (host>>i*8) & 0xff;//0xff: 255
        }
}

int acceptSocket(int sock)
{
        int conn;
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        // connection for data transmission 
        conn = accept(sock,(struct sockaddr*) &client_addr,&addrlen);
        return conn;
}

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
        listen(sock,CLIENTNUM);
        return sock;
}

void setMessage(char* message, char* value)
{
        memset(message, 0, strlen(message));
        strcpy(message, value);   
}

void writeMessage(int sock, char* msg)
{
        write(sock, msg, strlen(msg));
}

int sendMessage(int sock, char* cmd, char* arg) 
{
        char sendStr[MAXBUF];
        memset(sendStr, 0 , MAXBUF);
        strcpy(sendStr, cmd);
        if(arg != 0) 
        {
                strcat(sendStr, arg);  
        }
        strcat(sendStr, "\r\n");

        if ((write(sock, sendStr, sizeof(sendStr))) < 0) 
        {
                printf("Failed to send the command to server\r\n");
                return ERROR;
        }

        memset(sendStr, 0 , MAXBUF);

        return SUCCESS;
}

int recvMessage(int sock, char *message) 
{
        int res;
        memset(message, 0, MSGSIZE);

        if ((res = read(sock, message, MSGSIZE) < 0)) 
        {
              printf("Failed to read the data \r\n");
              return ERROR;
        } 
        printf("--Server Message:\r\n%s", message);
        
        return res; 
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
        //printf("search\n");
        for(i = 0 ; i < length; i++)
        {
              //if(strcmp(str, strList[i]) == 0)
              if(strcasecmp(str, strList[i]) == 0)
                      return i;
        }
        return -1;
}


void handleCommand(Directive* directive, Status* status)
{
        //printf("handlecommand\n");
        int returnVal = search(directive->cmd, strCmdList, sizeof(strCmdList)/sizeof(char* ));
        //printf("%d\n", returnVal);
        switch(returnVal)
        {
               case OPEN:
                      cmdOPEN(directive->arg, directive->value, status); break;

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
               case PASV:
                      cmdPASV(status); break;
               case PORT:
                      cmdPORT(status); break;

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
                      printf("Invalid command\n"); 
                      printf("========= CMD  HELP ==========\n");
                      printf("  OPEN <IP, port>\n");
                      printf("  USER <username>\n");
                      printf("  PASS <password>\n" );
                      printf("  RETR <username>\n" );
                      printf("  STOR <password>\n" );
                      printf("  QUIT <noparameter>\n" );
                      printf("  SYST <noparameter>\n" );
                      printf("  TYPE <type>\n");
                      printf("  PASV <noparameter>\n");
                      printf("  PORT <noparameter>\n");
                      printf("  CWD  <directory>\n");
                      printf("  CDUP <noparameter>\n");
                      printf("  DELE <filename>\n");
                      printf("  LIST <filename/directory>\n");
                      printf("  MKD  <directory>\n");
                      printf("  PWD  <noparameter>\n");
                      printf("  RMD  <directory>\n");
                      printf("  RNFR <filename/directory>\n");
                      printf("  RNTO <filename/directory>\n");
                      printf("==============================\n");

                      break;
        }
}

void startClient()
{
	char buf[MAXBUF];
	Directive* directive = malloc(sizeof(Directive));
        Status* status = malloc(sizeof(Status));

        /* unConnected */
        status->conn = -1;
      	
        while(1)
      	{
               printf("ftp> ");

               memset(buf, 0, MAXBUF);
               memset(directive->cmd, 0, CMDSIZE);
               memset(directive->arg, 0, MAXBUF);
               memset(directive->value, 0, NAMESIZE);

              	setbuf(stdin, NULL);
              	fgets(buf, MAXBUF, stdin);

               if(strcmp(buf,"\n") == 0)
                      continue;

               if(sscanf(buf,"%s %s %s",directive->cmd, directive->arg, directive->value) <= 0)
               {
                      printf("Invalid command format\n");
               }
               else
               {
                      //printf("%s\n", buf);
                      handleCommand(directive, status);
               }

               printf("\r\n");
      	}
}

int main()
{
	startClient();
	return 0;
}