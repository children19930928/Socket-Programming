#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/if.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>

#define DATABUF 1024*10
#define MAXBUF 1024
#define MSGSIZE 256
#define TIMEBUF 64
#define NAMESIZE 32
#define PERMSIZE 10
#define CLIENTNUM 1

/*const variables*/
typedef enum LOGIN{
      UNAUTHORIZE, NAMEREADY, LOGGED
} login;

typedef enum CONNMODE{
      NORMALMODE, PASSIVEMODE, PORTMODE
}connmode;

typedef enum CMDLIST{
      USER, PASS, RETR, STOR, QUIT, SYST, TYPE, PASV, PORT,
      CWD, CDUP, DELE, LIST, MKD, PWD, RMD, RNFR, RNTO
} cmdList;

/* TCP port for IP address */
typedef struct Port
{
      int p1;
      int p2;
} Port;

/* The command and argument */
typedef struct Directive
{
      char cmd[5];
      char arg[MAXBUF];
} Directive;

typedef struct Logtime
{
      char curdate[MSGSIZE];
      char curtime[MSGSIZE];
} Logtime;

/* Current server status */
typedef struct Status
{
      /* Connection */
      int conn;

      /* Connection state: NORMAL, PASSIVE, PORT */
      int mode;

      /* Login status: unauthorize, nameready, logged */
      int loginStatus;

      /* Record the current username */
      char username[NAMESIZE];

      /* Messages response to client */
      char message[MSGSIZE];

      /* root directory */
      char root[MSGSIZE];

      /* current directory */
      char curdir[MSGSIZE];

      /* old path for rename */
      char oldpath[MSGSIZE];

      /* the name of host */
      char hostname[NAMESIZE];

      /* the name of log file */
      char logname[NAMESIZE];

      /* Passive socket */
      int pasvSock;

      /* Port socket */
      int portSock;

      /* port used in Port mode */
      int portValue;

} Status;
/*end: const variables*/

/* Function declaration */

void startServer(int);
int createSocket(int);
int acceptSocket(int);
int connectSocket(char *, int);

void setMessage(char*, char*);
void writeMessage(int, char*);
void catMessage(char* , char* );

void getIpAddr(int , int *);
void generatePort(Port*);

int search(char*, const char**, int);
void handleCommand(Directive*, Status*);

void parentwait(int);
void transferPermission(int, char*);


void cmdUSER(char*, Status*);
void cmdPASS(char*, Status*);
void cmdRETR(char*, Status*);
void cmdSTOR(char*, Status*);
void cmdQUIT(Status*);
void cmdSYST(Status*);
void cmdTYPE(char*, Status*);
void cmdPASV(Status*);
void cmdPORT(char*, Status*);

void cmdCWD(char*, Status*);
void cmdCDUP(Status*);
void cmdDELE(char*, Status*);
void cmdLIST(char*, Status*);
void cmdMKD(char*, Status*);
void cmdPWD(Status*);
void cmdRMD(char*, Status*);
void cmdRNFR(char*, Status*);
void cmdRNTO(char*, Status*);
