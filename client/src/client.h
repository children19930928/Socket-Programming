#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h>


#define MAXBUF 1024
#define MSGSIZE 256
#define NAMESIZE 32
#define CMDSIZE 16
#define SUCCESS 0
#define ERROR -1
#define CLIENTNUM 1

/*const variables*/
typedef enum LOGIN{
	UNAUTHORIZE, NAMEREADY, LOGGED
} login;

typedef enum CONNMODE{
	NORMALMODE, PASSIVEMODE, PORTMODE
}connmode;

typedef enum CMDLIST{
	OPEN,
	USER, PASS, RETR, STOR, QUIT, SYST, TYPE, PASV, PORT,
	CWD, CDUP, DELE, LIST, MKD, PWD, RMD, RNFR, RNTO
} cmdList;


/* The command and argument */
typedef struct Directive
{
	char cmd[CMDSIZE];
	char arg[MAXBUF];
	char value[NAMESIZE];
} Directive;

/* TCP port for IP address */
typedef struct Port
{
	int p1;
	int p2;
} Port;

/* Current status */
typedef struct Status
{
	/* socket for command transmission */
	int cmdSock;

	/* socket for data transmission */
	int dataSock;

	/* connection with server */
	int conn;

	/* current mode */
	int mode;

	/* port for cmd transmission */
	int cmdport;

	/* port for data transmission */
	int dataport;

	/* server cmd address */
	struct sockaddr_in cmdAddr;

	/* server data address */
	struct sockaddr_in dataAddr;	

	/* Messages response to client */
	char message[MSGSIZE];

	/* Login status: unauthorize, nameready, logged */
	int loginStatus;

	/* whether current mode is passive */
	int isPassive;

	/* Record the current username */
	char username[NAMESIZE];
	
} Status;

int sWriteMessage(int sk, char *buf, int size);
int sReadMessage(int sk, char *buf, int size);
int sendMessage(int sock, char* cmd, char* arg);
int recvMessage(int sock, char *message);
int acceptSocket(int sock);
int createSocket(int port);
void getIpAddr(int cmdSock, int *ipAddr);
void generatePort(Port* port);
int search(char* str, const char** strList, int length);
void handleCommand(Directive* directive, Status* status);
void startClient();


int cmdOPEN(char*, char*, Status*);
int cmdCONNECTION(char*, Status* status);

int cmdUSER(char*, Status*);//1
int cmdPASS(char*, Status*);//2
int cmdRETR(char*, Status*);//3
int cmdSTOR(char*, Status*);//4
int cmdQUIT(Status* status);//5
int cmdSYST(Status*);//6
int cmdTYPE(char*, Status*);//7
int cmdPASV(Status*);//8
int cmdPORT(Status*);//9
int cmdCWD(char*, Status*);//10
int cmdCDUP(Status*);//11
int cmdDELE(char*, Status*);//12
int cmdLIST(char*, Status*);//13
int cmdMKD(char*, Status*);//14
int cmdPWD(Status*);//15
int cmdRMD(char*, Status*);//16
int cmdRNFR(char*, Status*);//17
int cmdRNTO(char*, Status*);//18

/* print message to client */
void printUNKONWN();
void printCONN();
void printUNCONN(); 
void printUNAUTH();
void printPASSWORD();
void printLOGGED();
void printMode();
void printIpFormat();
void printPortError();
void printSysError();
void printCmdError();

int checkState(Status* status);

