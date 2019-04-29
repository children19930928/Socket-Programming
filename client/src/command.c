#include "client.h"

int cmdOPEN(char* arg, char* value, Status* status)
{
        int port;
        int i;
        char buf[NAMESIZE];
        memset(buf, 0 , NAMESIZE);

        for(i =  0; i < 2; i++)
        {
              if((arg[0] != 0) && (value[0] != 0))
              {
                  sscanf(value, "%d", &port);
                  if(port > 0 && port < 65535)
                  {
                    status->cmdport = port;
                    if(cmdCONNECTION(arg, status) == SUCCESS)
                        return SUCCESS;
                    else
                        return ERROR;
                  }
                  else
                  {
                      printPortError();
                      return ERROR;
                  }
              } 
              else if ((arg[0] != 0) && (value[0] == 0))
              {
                  status->cmdport = 21;
                  if(cmdCONNECTION(arg, status) == SUCCESS)
                      return SUCCESS;
                  else
                      return ERROR;
              }
              else
              {
                  printf("(to) ");
                  setbuf(stdin, NULL);
                  fgets(buf, NAMESIZE, stdin);
                  sscanf(buf,"%s %s", arg, value);
                  continue;
              }
        }
        printSysError();
        return ERROR;
}

int cmdCONNECTION(char* arg, Status* status)
{
  
        //host information
        struct hostent *hp; 
        char strIp[NAMESIZE], username[NAMESIZE], password[NAMESIZE];  

        memset(strIp, 0 , NAMESIZE); 
        memset(username, 0 , NAMESIZE); 
        memset(password, 0 , NAMESIZE); 

        //create socket
        if ((status->cmdSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
        {
              printf("Problem Creating Socket \r\n");
              return ERROR;
        }

        //get server information
        if ((hp = gethostbyname(arg)) == NULL) 
        {
              printf("Failed to get IP From: %s \r\n" ,arg);
              return ERROR;
        }

        //establish connection
        memset(&status->cmdAddr, 0, sizeof(struct sockaddr_in));
        memcpy(&status->cmdAddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
        status->cmdAddr.sin_family = AF_INET;
        status->cmdAddr.sin_port = htons(status->cmdport);

        inet_ntop(AF_INET, (void *)&(status->cmdAddr.sin_addr), strIp, 16);

        printf("Connection Info:\r\nhostname: %s, address: %s, port: %d\r\n", 
              hp->h_name,
              strIp,
              status->cmdport);

        if ((connect(status->cmdSock, (struct sockaddr*)(&status->cmdAddr), sizeof(status->cmdAddr))) < 0) 
        {
               printf("Failed to connect server \r\n");
               return ERROR;
        }

        memset(&status->dataAddr, 0, sizeof(struct sockaddr_in));
        memcpy(&status->dataAddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
        status->dataAddr.sin_family = AF_INET;
        status->dataAddr.sin_port = htons(status->dataport);

        printf("Connected to %s \r\n", arg);

        recvMessage(status->cmdSock, status->message);
        if ((strstr((const char*)(status->message), "220")) == 0) 
        {
               printCmdError(); 
               return ERROR;
        }
        
        /* connect to server successfully */
        status->conn = 1;

        /* require for user name */
        printf("Name: ");
        setbuf(stdin, NULL);
        fgets(username, NAMESIZE, stdin);
        if(cmdUSER(username, status) == ERROR)
              return ERROR;

        /* require for password */
        printf("Password: ");
        setbuf(stdin, NULL);
        fgets(password, NAMESIZE, stdin);
        if(cmdPASS(password, status) == ERROR)
              return ERROR;

        cmdSYST(status);

        status->isPassive = 0;
        
        return SUCCESS;
}

int cmdUSER(char* arg, Status* status)
{
        if(status->conn < 0)
        {
            printUNCONN();
            return ERROR;
        }
        if (sendMessage(status->cmdSock, "USER ", arg) == ERROR) 
            return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
            return ERROR;

        if ((strstr((const char*)(status->message), "331")) == 0) {
             printf("Wrong Response \r\n");
             return ERROR;
        }

        status->loginStatus = NAMEREADY;

        return SUCCESS;
}

int cmdPASS(char* arg, Status* status)
{
        if(status->conn < 0)
        {
            printUNCONN();
            return ERROR;
        }
        if (status->loginStatus == UNAUTHORIZE)
        {
            printUNAUTH();
            return ERROR;
        }
        if (status->loginStatus == LOGGED)
        {
            printLOGGED();
            return ERROR;
        }
        if (sendMessage(status->cmdSock, "PASS ", arg) == ERROR) {
            return ERROR;
        }
        if (recvMessage(status->cmdSock, status->message) == ERROR) {
           return ERROR;
        }

        if ((strstr((const char*)(status->message), "230")) == 0) {
             printCmdError();
             return ERROR;
        }

        status->loginStatus = LOGGED;

        return SUCCESS;
}

int cmdRETR(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
        return ERROR;
        if(status->isPassive == 0)
        {
          if(cmdPORT(status) == ERROR)
          return ERROR ;
        }
        else
        {
          if(cmdPASV(status) == ERROR)
          return ERROR;   
        }

        int fp;
        int tsize = 0;
        int brecv = 0;
        char dataBuf[MAXBUF];
        memset(dataBuf, 0 , MAXBUF);

        if (status->mode == PASSIVEMODE)
        {

              if (sendMessage(status->cmdSock, "RETR ", arg) < 0 ) 
              {
                recvMessage(status->cmdSock, status->message);
                printf("Failed to download the file\r\n");
                return ERROR;
              }

              //create the data socket
              if ((status->dataSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
              {
                printf("Problem Creating Data Socket\r\n--Download Terminated\r\n");
                return ERROR;
              }

              //connect data socket
              if (connect(status->dataSock, (struct sockaddr *)(&(status->dataAddr)), sizeof(status->dataAddr)) < 0)
              {
                printf("Data Transportation Connection Failure !\r\n");
                return ERROR;
              }

              recvMessage(status->cmdSock, status->message);

              //create the file
              fp = open(arg, O_WRONLY|O_CREAT);
              if (fp < 0)
              {
                printf("Failed to create the file: %s\r\n", arg);
                close(status->dataSock);   
                recvMessage(status->cmdSock, status->message);
                return ERROR;
              }

              //download
              while((tsize = read(status->dataSock, dataBuf, MAXBUF)) > 0) 
              {
                brecv += tsize;
                write(fp, dataBuf, tsize);
                memset(dataBuf, 0, sizeof(dataBuf));
              }
              printf("Receive %d Bytes\r\n", brecv);

              // close the data socket you created
              close(status->dataSock);
              close(fp);
              recvMessage(status->cmdSock, status->message);
        }
        else if (status->mode == PORTMODE)
        {
            /* Port mode */
              int dataSock;

              if (sendMessage(status->cmdSock, "RETR ", arg) == ERROR) 
              {
                return ERROR;
              }

              dataSock = acceptSocket(status->dataSock);
              close(status->dataSock);

              recvMessage(status->cmdSock,status->message);

              //create the file
              fp = open(arg, O_WRONLY|O_CREAT);
              if (fp < 0)
              {
                printf("Failed to create the file: %s\r\n", arg);
                close(dataSock);   
                recvMessage(status->cmdSock, status->message);
                return ERROR;
              }

              //download
              while((tsize = read(dataSock, dataBuf, MAXBUF)) > 0) 
              {
                brecv += tsize;
                write(fp, dataBuf, tsize);
                memset(dataBuf, 0, sizeof(dataBuf));
              }
              printf("Receive %d Bytes\r\n", brecv);

              close(dataSock);
              close(fp);
              recvMessage(status->cmdSock, status->message);
        }
        else
        {
              printMode();
              return ERROR;
        }
        return SUCCESS;
}

int cmdSTOR(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
        return ERROR;
        if(status->isPassive == 0)
        {
          if(cmdPORT(status) == ERROR)
          return ERROR ;
        }
        else
        {
          if(cmdPASV(status) == ERROR)
          return ERROR;   
        }

        int fp;
        int tsize = 0;
        int brecv = 0;
        char dataBuf[MAXBUF];
        memset(dataBuf, 0, MAXBUF);
        
        if (status->mode == PASSIVEMODE)
        {

                if (sendMessage(status->cmdSock, "STOR ", arg) < 0 ) 
                {
                  recvMessage(status->cmdSock, status->message);
                  printf("Upload Failure\r\n");
                  return ERROR;
                }

                //create the data socket
                if ((status->dataSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
                {
                  printf("Failed to create Data Socket\r\n");
                  return ERROR;
                }

                //connect data socket
                if (connect(status->dataSock, (struct sockaddr *)(&status->dataAddr), sizeof(status->dataAddr)) < 0)
                {
                  printf("Failed to connect to the server !\r\n");
                  return ERROR;
                }
                  
                recvMessage(status->cmdSock, status->message);

                //open the file
                fp = open(arg, O_RDONLY);

                if(fp < 0) 
                {
                  printf("Reading %s Failure\r\n", arg);
                  close(status->dataSock);
                  recvMessage(status->cmdSock, status->message);
                  return ERROR;
                }

                while((tsize = read(fp, dataBuf, MAXBUF)) > 0) 
                {
                  brecv += tsize;
                  write(status->dataSock, dataBuf, tsize);
                  memset(dataBuf, 0, sizeof(dataBuf));  
                }
                printf("Send %d Bytes\r\n", brecv);

                close(status->dataSock);
                close(fp);
                recvMessage(status->cmdSock, status->message);
                memset(dataBuf, 0, MAXBUF);

                return SUCCESS;
        }
        else if (status->mode == PORTMODE)
        {
                int dataSock;

                if (sendMessage(status->cmdSock, "STOR ", arg) == ERROR) 
                {
                  recvMessage(status->cmdSock, status->message);
                  return ERROR;
                }

                dataSock = acceptSocket(status->dataSock);
                close(status->dataSock);

                recvMessage(status->cmdSock,status->message);

                //open the file
                fp = open(arg, O_RDONLY);

                if(fp < 0) 
                {
                      printf("Reading %s Failure\r\n", arg);
                      close(dataSock);
                      recvMessage(status->cmdSock, status->message);
                      return ERROR;
                }

              //upload
               while((tsize = read(fp, dataBuf, MAXBUF)) > 0) 
               {
                      brecv += tsize;
                      write(dataSock, dataBuf, tsize);
                      memset(dataBuf, 0, sizeof(dataBuf));  
               }
                printf("Send %d Bytes\r\n", brecv);

                close(dataSock);
                close(fp);
                recvMessage(status->cmdSock, status->message);
        }
        else
        {
              printMode();
              return ERROR;
        }
        return SUCCESS;
}

int cmdQUIT(Status* status) 
{
        if(status->conn < 0)
        {
                printUNCONN();
                return ERROR;
        }
        if (sendMessage(status->cmdSock, "QUIT", 0) == ERROR) 
        {
                return ERROR;
        }
        if (recvMessage(status->cmdSock, status->message) == ERROR) {
               return ERROR;
        }
        status->conn = -1;
        status->loginStatus = LOGGED;
        return SUCCESS;
}

int cmdSYST(Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;

        if (sendMessage(status->cmdSock, "SYST", 0) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        return SUCCESS;
}

int cmdTYPE(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
                return ERROR;

        if (sendMessage(status->cmdSock, "TYPE ", arg) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
                return ERROR;
        
        return SUCCESS;
}

int cmdPASV(Status* status)
{
        Port* port = malloc(sizeof(Port));
        int ip[4];
        char* pIndex;

        if(checkState(status) == ERROR)
               return ERROR;

        // send PASV command and parse the ip address sent back

        if (sendMessage(status->cmdSock, "PASV", 0) == ERROR) 
               return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        if ((strstr((const char*)(status->message), "227")) == 0) {
               printCmdError();
               return ERROR;
        }

        if ((pIndex = strstr(status->message, "(")) != 0) 
        {
                if(sscanf(pIndex + 1, "%d, %d, %d, %d, %d, %d", &ip[0], &ip[1], &ip[2], &ip[3], &(port->p1), &(port->p2)) <=0)
                {
                        printIpFormat();
                        return ERROR;
                }

                status->dataport = port->p1 * 256 + port->p2;
                status->dataAddr.sin_port = htons(status->dataport);
        }
        else
        {
                printIpFormat();
                return ERROR;
        }

        status->mode = PASSIVEMODE;
        status->isPassive = 1;

        return SUCCESS;
}

int cmdPORT(Status* status)
{
         if(checkState(status) == ERROR)
               return ERROR;

          // generate port and send to the server
         int portValue, ipAddr[4];
         char buf[MAXBUF];
         memset(buf, 0 , MAXBUF);

         Port* port = malloc(sizeof(Port));
         generatePort(port);
         portValue = 256*port->p1 + port->p2;
         printf("New data sock port is: %d\n", portValue);

         // use command connection sock to get ip address 
         getIpAddr(status->cmdSock, ipAddr);

         //new socket port user to transfer data
         status->dataSock = createSocket(portValue);
         status->mode = PASSIVEMODE;
         sprintf(buf,"%d,%d,%d,%d,%d,%d",ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], port->p1, port->p2);

         if (sendMessage(status->cmdSock, "PORT ", buf) == ERROR)
                return ERROR;

         if (recvMessage(status->cmdSock, status->message) == ERROR)
               return ERROR;

         status->mode = PORTMODE;
         status->isPassive = 0;

         return SUCCESS;
}

int cmdCWD(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;

        if (sendMessage(status->cmdSock, "CWD ", arg) == ERROR) 
               return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        return SUCCESS;
}

int cmdCDUP(Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;

        if (sendMessage(status->cmdSock, "CDUP", 0) == ERROR)
               return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        return SUCCESS;
}

int cmdDELE(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
                return ERROR;

        if (sendMessage(status->cmdSock, "DELE ", arg) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        return SUCCESS;
}

int cmdLIST(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;

        char dataBuf[MAXBUF];
        memset(dataBuf, 0 , MAXBUF);

        if (status->isPassive == 1)
        {
              if (sendMessage(status->cmdSock, "LIST ", arg) < 0) 
              {
                      return ERROR;
              }
              //create the data socket
              if ((status->dataSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
              {
                      printf("Problem Creating Data Socket\r\n--Download Terminated\r\n");
                      return ERROR;
              }
              //connect data socket
              if (connect(status->dataSock, (struct sockaddr *)(&(status->dataAddr)), sizeof(status->dataAddr)) < 0)
              {
                      printf("Data Transportation Connection Failure !\r\n");
                      return ERROR;
              }
              recvMessage(status->cmdSock,status->message);

              while(read(status->dataSock, dataBuf, MAXBUF) > 0) 
              {
                      //print what received from server to client 
                      printf("%s", dataBuf);
                      memset(dataBuf, 0, sizeof(dataBuf));
              }

              close(status->dataSock);
              recvMessage(status->cmdSock, status->message);
        }
        else if (status->isPassive == 0)
        {
              //Port mode 
              int dataSock;

              if (sendMessage(status->cmdSock, "LIST ", arg) == ERROR) 
              {
                return ERROR;
              }

              dataSock = acceptSocket(status->dataSock);
              close(status->dataSock);

              recvMessage(status->cmdSock,status->message);

              while(read(dataSock, dataBuf, MAXBUF) > 0) 
              {
                //print what received from server to client 
                printf("%s", dataBuf);
                memset(dataBuf, 0, sizeof(dataBuf));
              }

              close(dataSock);
              recvMessage(status->cmdSock, status->message);

        }
        else
        {
              printMode();
              return ERROR;
        }
        return SUCCESS;
}

int cmdMKD(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;

        if (sendMessage(status->cmdSock, "MKD ", arg) == ERROR) 
              return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
              return ERROR;
        
        return SUCCESS;
}

int cmdPWD(Status* status)
{
        if(checkState(status) == ERROR)
               return ERROR;
        if (sendMessage(status->cmdSock, "PWD", 0) == ERROR)
               return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
               return ERROR;
        
        return SUCCESS;
}

int cmdRMD(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
                return ERROR;

        if (sendMessage(status->cmdSock, "RMD ", arg) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
                return ERROR;
        
        return SUCCESS;
}

int cmdRNFR(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
                return ERROR;

        if (sendMessage(status->cmdSock, "RNFR ", arg) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
                return ERROR;
        
        return SUCCESS;
}

int cmdRNTO(char* arg, Status* status)
{
        if(checkState(status) == ERROR)
                return ERROR;

        if (sendMessage(status->cmdSock, "RNTO ", arg) == ERROR) 
                return ERROR;
        
        if (recvMessage(status->cmdSock, status->message) == ERROR) 
                return ERROR;
        
        return SUCCESS;
}

//****************************************************************//

void printUNKONWN() 
{
       printf("Undefined Command, You Can Use 'Help' To Get Help.\r\n");
}

void printCONN() 
{
        printf("Already connect to server.\r\n");
}

void printUNCONN() 
{
        printf("Please connect to server first.\r\n");
}

void printUNAUTH()
{
        printf("Please login in first\n");
}

void printPASSWORD()
{
        printf("Password required for User\n");
}

void printLOGGED()
{
        printf("User has already logined\n");
}

void printMode()
{
        printf("Please choose passive or port mode first\n");
}

void printIpFormat()
{
        printf("The message does not match the format(h1,h2,h3,h4,p1,p2)\n");
}

void printPortError()
{
        printf("Port value range from 0 to 65535\n");
}

void printSysError()
{
        printf("System error\n");
}

void printCmdError()
{
        printf("Bad Response from server \r\n");
}

int checkState(Status* status)
{
        if(status->conn < 0)
        {
               printUNCONN();
               return ERROR;
        }
        if (status->loginStatus == UNAUTHORIZE)
        {
               printUNAUTH();
               return ERROR;
        }
        if (status->loginStatus == NAMEREADY)
        {
               printPASSWORD();
               return ERROR;
        }
        return SUCCESS;
}