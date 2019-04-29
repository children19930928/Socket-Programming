#include "server.h" 

static const char *usertable[] = 
{
      "anonymous", "a", "qhj"
};
/***************"Handle cmd"*****************/

void cmdUSER(char* arg, Status *status)
{
        if(search(arg, usertable, sizeof(usertable)/sizeof(char* )) >= 0)
        {
                status->loginStatus = NAMEREADY;
                setMessage(status->username, arg);
                setMessage(status->message,"331 Password required for ");
                catMessage(status->message, arg);
                catMessage(status->message, "\n");
        }
        else
        {
                setMessage(status->message,"530 Invalid username\n");    
        }
        writeMessage(status->conn, status->message);
}

void cmdPASS(char* arg, Status *status)
{
        if(status->loginStatus == NAMEREADY)
        {
                status->loginStatus = LOGGED;
                setMessage(status->message,"230 User ");
                catMessage(status->message, status->username);
                catMessage(status->message, " logged in\n");
        }
        else
        {
                setMessage(status->message,"530 Username or Password incorrect\n");
        }
        writeMessage(status->conn, status->message);
}

void cmdRETR(char* arg, Status* status)
{
        int conn, fd;    
        int readbytes = 0, sendtotal = 0;
        char buf[MSGSIZE];
        char dataBuf[MAXBUF];
        char dirBuf[MSGSIZE];

        memset(buf, 0, MSGSIZE);
        memset(dataBuf, 0, MAXBUF);
        
        if(status->loginStatus == LOGGED)
        {
              if(status->mode == PASSIVEMODE)
              {
                      sprintf(dirBuf, "%s", arg);
                      fd = open(dirBuf,O_RDONLY);
                      if(access(dirBuf,R_OK)==0 && (fd>=0))
                      {
                              conn = acceptSocket(status->pasvSock);
                              close(status->pasvSock);

                              setMessage(status->message, "150 Opening BINARY mode data connection\n");
                              writeMessage(status->conn, status->message);
                            
                              //send
                              while((readbytes = read(fd, dataBuf, MAXBUF)) > 0)
                              {
                                      sendtotal += readbytes;
                                      write(conn, dataBuf, readbytes);
                                      memset(dataBuf, 0, sizeof(dataBuf));  
                              }

                              //printf("%d bytes has been sent\n", sendtotal);
                              sprintf(buf, "226 %d bytes has been sent\n", sendtotal);
                              setMessage(status->message, buf);

                              close(fd);
                              close(conn);
                      }
                      else
                              setMessage(status->message, "550 Failed to get file\n");
              }
              else if(status->mode == PORTMODE)
              {
                      sprintf(dirBuf, "%s", arg);
                      fd = open(dirBuf,O_RDONLY);
                      if(access(dirBuf,R_OK)==0 && (fd>=0))
                      {
                              /* connect to client */
                              conn = connectSocket(status->hostname, status->portValue);

                              if(conn<0)
                              {
                                setMessage(status->message, "550 The connection cannot be established properly\n");
                                writeMessage(status->conn, status->message);
                                return;
                                //exit(EXIT_SUCCESS);          
                              }

                              setMessage(status->message, "150 Opening BINARY mode data connection\n");
                              writeMessage(status->conn, status->message);

                              //send
                              memset(dataBuf, 0, sizeof(dataBuf)); 
                              while((readbytes = read(fd, dataBuf, MAXBUF)) > 0) 
                              {
                                sendtotal += readbytes;
                                write(conn, dataBuf, readbytes);
                                memset(dataBuf, 0, sizeof(dataBuf));  
                              }

                              sprintf(buf, "226 %d bytes has been sent\r\n", sendtotal);
                              setMessage(status->message, buf);
                              writeMessage(status->conn, status->message);
                              
                              close(conn);
                              close(fd);

                              return;
                      }
                      else
                              setMessage(status->message, "550 Failed to get file\n");
              }
              else
                      setMessage(status->message, "550 Please switch the mode to PASV\n");
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message);
        return;
}

void cmdSTOR(char* arg, Status* status)
{
        int conn, fd;
        int fsize = 0;
        int recv = 0;
        char dataBuf[DATABUF], msgBuf[MSGSIZE];
        char dirBuf[MSGSIZE];

        memset(msgBuf, 0 , MSGSIZE);

        if(status->loginStatus == LOGGED)
        {
              if(status->mode == PASSIVEMODE)
              {
                      // connection for data transmission
                      conn = acceptSocket(status->pasvSock);
                      close(status->pasvSock);

                      setMessage(status->message, "150 Data connection already open.Transfer starting...\n");
                      writeMessage(status->conn, status->message);

                      //create the file
                      sprintf(dirBuf, "%s",  arg);
                      FILE *fp = fopen(dirBuf,"w");  

                      if(fp==NULL)
                      {
                              close(conn);
                              perror("ftp_stor:fopen");
                              setMessage(status->message, "The file cannot be opened properly\n");
                              writeMessage(status->conn, status->message);
                              //exit(EXIT_SUCCESS);
                              return;
                      }

                      fd = fileno(fp);

                      memset(dataBuf, 0, sizeof(dataBuf));
                      while((fsize = read(conn, dataBuf, DATABUF)) > 0) 
                      {
                              recv += fsize;
                              write(fd, dataBuf, fsize);
                              memset(dataBuf, 0, sizeof(dataBuf));
                      }

                      //printf("Receive %d bytes\r\n", recv);
                      sprintf(msgBuf,"226 %d bytes has been received\n", recv);
                      setMessage(status->message, msgBuf);
                      close(conn);
                      close(fd);
              }
              else if(status->mode == PORTMODE)
              {
                      conn = connectSocket(status->hostname, status->portValue);

                      if(conn<0)
                      {
                              setMessage(status->message, "The connection cannot be established properly\n");
                              writeMessage(status->conn, status->message);
                              //exit(EXIT_SUCCESS);
                              return;          
                      }

                      setMessage(status->message, "150 Data connection already open.Transfer starting...\n");
                      writeMessage(status->conn, status->message);

                      //create the file
                      sprintf(dirBuf, "%s", arg);
                      FILE *fp = fopen(dirBuf,"w");  
                      if(fp==NULL)
                      {
                              close(conn);
                              perror("ftp_stor:fopen");
                              setMessage(status->message, "The file cannot be opened properly\n");
                              writeMessage(status->conn, status->message);
                              //exit(EXIT_SUCCESS);
                              return;
                      }

                      fd = fileno(fp);

                      while((fsize = read(conn, dataBuf, DATABUF)) > 0) 
                      {
                              recv += fsize;
                              write(fd, dataBuf, fsize);
                              memset(dataBuf, 0, sizeof(dataBuf));
                      }

                      printf("Receive %d bytes\r\n", recv);

                      sprintf(msgBuf,"226 %d bytes has been received\n", recv);
                      setMessage(status->message, msgBuf);

                      close(conn);
                      close(fd);
              }
              else
                      setMessage(status->message, "550 Please switch the mode to PASV\n");  
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message);
        return;
}

void cmdQUIT(Status* status)
{
        setMessage(status->message,"221 Thank you for using qhj FTP. Goodbye! \n");
        writeMessage(status->conn, status->message);
        close(status->conn);
        exit(0);
}

void cmdSYST(Status* status)
{
        setMessage(status->message, "215 UNIX Type: L8\n");
        writeMessage(status->conn, status->message);  
}

void cmdTYPE(char* arg, Status* status)
{
       if(status->loginStatus == LOGGED)
       {
              if(arg[0]=='I' || arg[0]=='i' )
                      setMessage(status->message, "200 Type set to I.\n"); 
              else
                      setMessage(status->message, "504 Data connection mode can only be switched TO BINARY\n");
       }
       else
              setMessage(status->message, "530 Please login first\n");
              writeMessage(status->conn, status->message);
}

void cmdPORT(char* arg, Status* status)
{
        if(status->loginStatus == LOGGED)
        {

               int portValue, ipAddr[4];
               char buf[MAXBUF];
               Port* port = malloc(sizeof(Port));
               /*Parse arg to ip address*/
               if(sscanf(arg,"%d,%d,%d,%d,%d,%d", &ipAddr[0], &ipAddr[1], &ipAddr[2], &ipAddr[3], &(port->p1), &(port->p2)) <= 0)
               {
                      status->mode = NORMALMODE;
                      setMessage(status->message, "550 Failed to parse the Ip address\n");
                      writeMessage(status->conn, status->message);  
                      return;
               }
               
               portValue = 256*port->p1 + port->p2;
               //printf("New data sock port is: %d\n", portValue);

               sprintf(status->hostname,"%d.%d.%d.%d", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
               status->portValue = portValue;
               status->mode = PORTMODE;

               sprintf(buf,"%d,%d,%d,%d,%d,%d",ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], port->p1, port->p2);
               setMessage(status->message,"200 PORT command successful(");
               catMessage(status->message,buf);
               catMessage(status->message,")\n");
        }
        else
        {
               setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message);  
}

void cmdPASV(Status* status)
{
        if(status->loginStatus == LOGGED)
        {
               int portValue, ipAddr[4];
               char buf[MAXBUF];
               Port* port = malloc(sizeof(Port));
               generatePort(port);
               portValue = 256*port->p1 + port->p2;
               //printf("New data sock port is: %d\n", portValue);
               // use command connection sock to get ip address
               getIpAddr(status->conn, ipAddr);

               close(status->pasvSock);
               // new socket port user to transfer data
               status->pasvSock = createSocket(portValue);
               status->mode = PASSIVEMODE;
               sprintf(buf,"%d,%d,%d,%d,%d,%d",ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], port->p1, port->p2);
               setMessage(status->message,"227 Entering Passive Mode (");
               catMessage(status->message,buf);
               catMessage(status->message,")\n");
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message); 
}

void cmdCWD(char* arg, Status* status)
{ 
        char buf[MSGSIZE];
        memset(buf, 0 , MSGSIZE);

        if(status->loginStatus == LOGGED)
        {
              if((strcmp(arg,"../") == 0) || (strcmp(arg,"..") == 0))
              {
                      //cmdCDUP(status);
                      memset(status->curdir, 0 ,MSGSIZE);
                      if(getcwd(status->curdir, MSGSIZE) != NULL)
                      {
                        sprintf(buf, "250 CWD command successful.'%s' is current directory.\n", status->curdir);
                        setMessage(status->message, buf);             
                      }
              }
              if(chdir(arg) == 0)
              {
                      memset(status->curdir, 0 ,MSGSIZE);
                      if(getcwd(status->curdir, MSGSIZE) != NULL)
                      {
                        sprintf(buf, "250 CWD command successful.'%s' is current directory.\n", status->curdir);
                        setMessage(status->message, buf);  
                      }
              }
              else
                      setMessage(status->message,"550 Fail to change the directory\n");
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message); 
}

void cmdCDUP(Status* status)
{
        char buf[MSGSIZE];
        memset(buf, 0 , MSGSIZE);

        if(status->loginStatus == LOGGED)
        {
              int i, len = strlen(status->curdir);

              // check whether the current directory is root directory 
              for (i = 1; i < len; ++i)
              {
                      if(status->curdir[i] == '/')
                        break;
              }
              if(i != len)
              {
                      if(chdir("../") == 0)
                      {
                              memset(status->curdir, 0 ,MSGSIZE);
                              if(getcwd(status->curdir, MSGSIZE) != NULL)
                              {
                                      sprintf(buf, "250 CDUP command successful.'%s' is current directory.\n", status->curdir);
                                      setMessage(status->message, buf);             
                              }
                      }
                      else
                              setMessage(status->message,"550 Fail to change the directory\n");
              }
              else
              {
                      sprintf(buf, "250 CDUP command successful.'%s' is current directory.\n", status->root);
                      setMessage(status->message, buf);    
              }
        }
        else
        {
                setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message); 
}

void cmdDELE(char* arg, Status* status)
{
        if(status->loginStatus == LOGGED)
        {
               if(unlink(arg)==-1)
                       setMessage(status->message, "550 Failed to delete the file\n");
               else
                       setMessage(status->message, "250 Succeed to delete the file\\n");
        }
        else
        {
               setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message);  
}

void cmdLIST(char* arg, Status* status)
{
        if(status->loginStatus == LOGGED)
        {
              struct dirent *entry;
              struct stat statbuf;
              struct passwd *userdata;
              struct group *groupdata;
              struct tm *time;
              char timebuff[TIMEBUF];
              int conn;
              time_t rawtime;

              char cwd[MAXBUF], cwd_orig[MAXBUF];
              memset(cwd,0,MAXBUF);
              memset(cwd_orig,0,MAXBUF);
              
              // Later we want to go to the original path
              getcwd(cwd_orig,MAXBUF);
              
              // Just chdir to specified path 
              if(strlen(arg)>0 && arg[0]!='-')
                    chdir(arg);
              
              getcwd(cwd,MAXBUF);
              DIR *dp = opendir(cwd);

              if(dp)
              {
                    if(status->mode == PASSIVEMODE)
                    {
                            conn = acceptSocket(status->pasvSock);
                            close(status->pasvSock);

                            setMessage(status->message, "150 The directory is ready to be listed\n");
                            writeMessage(status->conn, status->message);

                            while((entry=readdir(dp)))
                            {
                                  if(stat(entry->d_name,&statbuf)==-1)
                                  {
                                        fprintf(stderr, "FTP: Error reading file stats...\n");
                                        setMessage(status->message, "550 Failed to read file stats\n");
                                  }
                                  else
                                  {
                                        char *perms = malloc(PERMSIZE);
                                        memset(perms,0,PERMSIZE);

                                        transferPermission((statbuf.st_mode & ALLPERMS), perms);
                                        /* Convert st_uid to passwd struct */
                                        userdata = getpwuid(statbuf.st_uid);
                                        /* Convert st_gid to group struct */
                                        groupdata = getgrgid(statbuf.st_gid);
                                        /* Convert time_t to tm struct */
                                        rawtime = statbuf.st_mtime;
                                        time = localtime(&rawtime);
                                        strftime(timebuff,TIMEBUF,"%b %d %H:%M",time);
                                        dprintf(
                                              conn,
                                              "%c%s %5ld %s %s %8ld %s %s\r\n", 
                                              (entry->d_type==DT_DIR)?'d':'-',
                                              perms,statbuf.st_nlink,
                                              userdata->pw_name, 
                                              groupdata->gr_name,
                                              statbuf.st_size,
                                              timebuff,
                                              entry->d_name);
                                  }
                            }
                            setMessage(status->message, "226  Transfer complete\n");
                            status->mode = NORMALMODE;
                            close(conn);
                    }
                    else if(status->mode == PORTMODE)
                    {
                          /* connect to client */
                          //printf("hostname:%s\n", status->hostname);
                          conn = connectSocket(status->hostname, status->portValue);

                          if(conn<0)
                          {
                                setMessage(status->message, "550 The connection cannot be established properly\n");
                                writeMessage(status->conn, status->message);
                                exit(EXIT_SUCCESS);          
                          }

                          setMessage(status->message, "150 The directory is ready to be listed\n");
                          writeMessage(status->conn, status->message);

                          while((entry=readdir(dp)))
                          {
                                if(stat(entry->d_name,&statbuf)==-1)
                                {
                                      fprintf(stderr, "FTP: Error reading file stats...\n");
                                      setMessage(status->message, "550 Failed to read file stats\n");
                                }
                                else
                                {
                                        char *perms = malloc(PERMSIZE);
                                        memset(perms,0,PERMSIZE);

                                        transferPermission((statbuf.st_mode & ALLPERMS), perms);
                                         //Convert st_uid to passwd struct
                                        userdata = getpwuid(statbuf.st_uid);
                                         // Convert st_gid to group struct 
                                        groupdata = getgrgid(statbuf.st_gid);
                                        // Convert time_t to tm struct 
                                        rawtime = statbuf.st_mtime;
                                        time = localtime(&rawtime);
                                        strftime(timebuff,TIMEBUF,"%b %d %H:%M",time);
                                        dprintf(
                                              conn,
                                              "%c%s %5ld %s %s %8ld %s %s\r\n", 
                                              (entry->d_type==DT_DIR)?'d':'-',
                                              perms,statbuf.st_nlink,
                                              userdata->pw_name, 
                                              groupdata->gr_name,
                                              statbuf.st_size,
                                              timebuff,
                                              entry->d_name);
                                }
                           }
                          setMessage(status->message, "226 Directory info has been sent\n");
                          status->mode = NORMALMODE;
                          close(conn);
                    }
                    else
                    {
                      setMessage(status->message, "425 Please use PASV or PORT command first\n");
                    }
              }
              else
              {
                    setMessage(status->message, "550 Failed to open directory\n");      
              }
              closedir(dp);
              chdir(cwd_orig);
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message);
}


void cmdMKD(char*arg, Status* status)
{
      if(status->loginStatus == LOGGED)
      {
              char dir[MAXBUF], res[MAXBUF];
              memset(dir, 0, MAXBUF);
              memset(res, 0, MAXBUF);
              getcwd(dir,MAXBUF);
              
              if((access(arg, 0 )) != -1)
              {
                     setMessage(status->message, "550 The directory already exists\n");
              }
              else
              {
                    // Absolute path
                    if(arg[0] == '/')
                    {
                          if(mkdir(arg, S_IRWXU) == 0)
                          {
                                  setMessage(status->message, "257 \"");
                                  catMessage(status->message, arg);
                                  catMessage(status->message, "\" new directory has been created\n");     
                          }
                          else
                                  setMessage(status->message, "550 Failed to create directory\n");
                    }
                    // Relative path 
                    else
                    {
                      if(mkdir(arg, S_IRWXU) == 0)
                      {
                            setMessage(status->message, "257 \"");
                            catMessage(status->message, dir);
                            catMessage(status->message, "/");
                            catMessage(status->message, arg);
                            catMessage(status->message, "\" new directory has been created\n");
                      }
                      else
                            setMessage(status->message, "550 Failed to create directory\n");
                    }
              }
      }
      else
      {
        setMessage(status->message, "530 Please login first\n");
      }
      writeMessage(status->conn, status->message);
}

void cmdPWD(Status* status)
{
      if(status->loginStatus == LOGGED)
      {
              char dir[MAXBUF], res[MAXBUF];
              memset(res, 0, MAXBUF);
              if(getcwd(dir, MAXBUF) != NULL)
              {
                      setMessage(status->message, "257 \"");
                      catMessage(status->message, dir);
                      catMessage(status->message, "\"\n");
              }
              else
              { 
                      setMessage(status->message,"550 Fail to get the current directory\n");
              }
      }
      else
      {
              setMessage(status->message, "530 Please login first\n");
      }
      writeMessage(status->conn, status->message); 
}

void cmdRMD(char* arg, Status* status)
{
      if(status->loginStatus == LOGGED)
      {
             if((access(arg, 0 )) != -1)
             {
                    if(rmdir(arg)==0)
                            setMessage(status->message, "250 The directory has been deleted\n");
                    else
                            setMessage(status->message, "550 Failed to delete directory\n");
             }
             else
                    setMessage(status->message, "550 The directory does not exist\n");
      }
      else
      {
              setMessage(status->message, "530 Please login first\n");
      }
      writeMessage(status->conn, status->message);
}

void cmdRNFR(char* arg, Status* status)
{
      if(status->loginStatus == LOGGED)
      {
              char buf[MSGSIZE];
              memset(buf, 0 , MSGSIZE);
              // check whether the path exists 
              if(access(arg,0) == -1)
              {
                    sprintf(buf, "550 '%s':No such file or directory.\n", arg);
                    setMessage(status->message, buf);
              }
              else
              {
                    setMessage(status->oldpath, arg);
                    setMessage(status->message, "350 File exists, ready for destination name.\n");
              }
      }
      else
      {
              setMessage(status->message, "530 Please login first\n");
      }
      writeMessage(status->conn, status->message); 
}

void cmdRNTO(char* arg, Status* status)
{
        //rename the file
        if(status->loginStatus == LOGGED)
        {
              char buf[MSGSIZE];
              memset(buf, 0 , MSGSIZE);
              if(rename(status->oldpath, arg) == -1)
              {
                    sprintf(buf, "550 '%s':File or directory can not be renamed.\n", arg);
                    setMessage(status->message, buf);
              }
              else
                     setMessage(status->message, "250 Rename successful.\n");
        }
        else
        {
              setMessage(status->message, "530 Please login first\n");
        }
        writeMessage(status->conn, status->message); 
}