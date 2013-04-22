/*******************************************/
/*		TFTP Part 2		   */	
/*		Calvin C Chiu		   */	
/*					   */
/*					   */
/*******************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>
#include "csapp.h"

#define PORT 7000

#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERR 5

#define MODE_OCTET "octet"

#define HEADER_LEN 5
#define PACKETSIZE 512
#define MAXBUFFSIZE 1024

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

FILE * file;

struct output{
	char* opcode;
	char* mode;
	char* file;
	char* addr;
	int port;
};

struct output populateOutput(struct output x, char *msg, char *buf)
{
   struct output r = x;
   r.opcode = msg;
   r.mode = buf + 3 + strlen(buf + 2);
   r.file = buf + 2;
   return r;
}

int main (int argc, char **argv){

	// set default port number
	int portnum = PORT;
	
	// file descriptor
	int sockfd;
	ssize_t n;	
	
	// buffer and variable
	char error[PACKETSIZE];
	char buf[MAXBUFFSIZE];
	socklen_t len;
	
	char datapacket[PACKETSIZE];
	size_t byte;
	int bytelen;
	int FLAG;
	int blocknum = 1;
	
	char *putError = "PUT is not supported";
	char *fileNotFound = "File not found";	
	char *modeError = "Only Octet mode is supported";
	
	fd_set rset;
	int nready;
	int maxfd;

	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
   	struct output myoutput;
	
	// check valid input
	if(argc == 2){
		portnum = atoi(argv[1]);
	}
	else if(argc == 1){
		portnum = PORT;
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		printf("Port number not defined. Default port is used.\n");	
	}
	else{
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(0);
	}
	
	// buffers init
	bzero(&error, sizeof(error));
	bzero(&buf, sizeof(buf));
	bzero(&datapacket, sizeof(datapacket));
	
	// server address struct		
	
	bzero((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET; 
    	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    	serveraddr.sin_port = htons((unsigned short)portnum); 
	
	// Setting up and bind socket
	// Using Helper functions from csapp.c with error checking
	sockfd = Socket(AF_INET,SOCK_DGRAM,0);	   	
   	Bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));   	
   	printf("Port: %d\n", ntohs(serveraddr.sin_port));
   	  	
	FD_ZERO(&rset);
	maxfd = sockfd + 1;

	for(;;){
		
		FD_SET(sockfd, &rset);			
		if((nready = select(maxfd, &rset, NULL,NULL,NULL))<0){
			continue;
		}
    		if (FD_ISSET(sockfd, &rset)) {
		
			len = sizeof(clientaddr);
			if((n = recvfrom(sockfd,buf,sizeof(buf),0,(struct sockaddr *)&clientaddr,&len)) < 0){
				fprintf(stderr, "%s\n", strerror(errno));
			}
		
			// check the opcode of the request				
			// GET
			if (buf[1] == OPCODE_RRQ)
			{
				// populate output struct
				myoutput = populateOutput(myoutput, "RRQ", buf);
				if(strcmp(myoutput.mode, MODE_OCTET)!=0){
					error[1] = OPCODE_ERR;
					error[3] = 1;
					strncat((error+4), modeError, strlen(modeError));
			
					if((sendto(sockfd, error, (strlen(error+4)+HEADER_LEN),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))) < 0){
						fprintf(stderr, "%s\n", strerror(errno));					
					}			
					goto cleanup;
				}
				myoutput.addr = inet_ntoa(clientaddr.sin_addr);
				myoutput.port = clientaddr.sin_port;			
			
				blocknum = 1;
				printf("%s %s %s from %s:%d\n", myoutput.opcode, myoutput.mode, myoutput.file, myoutput.addr, myoutput.port); 
		
		
				if((file = fopen(myoutput.file, "r")) == NULL){			
					// generate error packet
				
					error[1] = OPCODE_ERR;
					error[3] = 1;
					strncat((error+4), fileNotFound, strlen(fileNotFound));
			
					if((sendto(sockfd, error, (strlen(error+4)+HEADER_LEN),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))) < 0){
						fprintf(stderr, "%s\n", strerror(errno));					
					}
			
				}
				else{							
					datapacket[1] = OPCODE_DATA;
					datapacket[3] = blocknum;
					if(file != 0 &&(byte = fread(datapacket+4, 1, PACKETSIZE, file))<0){
						fprintf(stderr, "%s\n", strerror(errno));
					}
					else{
						if((write(1, datapacket+4, byte)) < 0){
							fprintf(stderr, "%s\n", strerror(errno));
						}
					}
					if((bytelen = byte) < 0){
						continue;
					}
					FLAG = 1;
					if((sendto(sockfd, datapacket, bytelen+4, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr))) < 0){
						fprintf(stderr, "%s\n", strerror(errno));
					}
				}
			}			
		
				// ACK
				else if(buf[1] == OPCODE_ACK){
					if(FLAG && buf[3] == blocknum){
						datapacket[1] = OPCODE_DATA;
						datapacket[3] = ++blocknum;

						if((byte = fread(datapacket+4, 1, PACKETSIZE, file)) < 0){
							fprintf(stderr, "%s\n", strerror(errno));
						}
						else{
							if((write(1, datapacket+4, byte)) < 0){
								fprintf(stderr, "%s\n", strerror(errno));
							}
						}
						if((bytelen = byte) < 0){
							printf("\n");
						}
						if((sendto(sockfd, datapacket, bytelen+4, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr))) < 0){
							fprintf(stderr, "%s\n", strerror(errno));
						}
			
					}
				
				}
				  		  
				// PUT  - Not supported
				else if (buf[1] == OPCODE_WRQ)
				{
					myoutput = populateOutput(myoutput, "WRQ", buf);
				
					myoutput.addr = inet_ntoa(clientaddr.sin_addr);
					myoutput.port = clientaddr.sin_port;
					printf("%s %s %s from %s:%d\n", myoutput.opcode, myoutput.mode, myoutput.file, myoutput.addr, myoutput.port); 
					// generate error packet
					error[1] = OPCODE_ERR;
					error[3] = 1;
					strncat((error+4), putError, strlen(putError));		
					if((sendto(sockfd, error, (strlen(error+4)+HEADER_LEN),0,(struct sockaddr *)&clientaddr,sizeof(clientaddr))) < 0){
						fprintf(stderr, "%s\n", strerror(errno));					
					}
				}	
			cleanup:
				//printf("Cleaning buffers...\n");				
				// clear buffers		
				bzero(&error, sizeof(error));
				bzero(&buf, sizeof(buf));
				bzero(&datapacket, sizeof(datapacket));
	
		}
	}
}


