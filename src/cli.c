/* tcp-client.c */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512


void* Sendfile(char* Filename, void* Socked)
{
	int *SockedCopy = Socked; 
	char *filename = Filename;
	char buffer[1025];
	FILE *fp;
	fp = fopen(filename, "r");
	if(NULL == fp)
	{
		printf("File:%s Not Founded\n", filename);
	}
	else
	{
		buffer[0]='\0';
		int length =0;
		while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
		{
			write(*SockedCopy, &length, sizeof(int));
			if(write(*SockedCopy, buffer, length) < 0)
			{
				printf("Upload file:%s Failed.\n", filename);
				break;
			}
			bzero(buffer, BUFFER_SIZE);
		}
	}
	fclose(fp);
	printf("File:%s Upload Successfully!\n", filename);
	
}
//send the message to the server any time terminal get input
void*  Send(void* Socked)
{
	char sender[80];
	char Filename[FILE_NAME_MAX_SIZE];
	//save the socked into a int pointer
	int *SockedCopy = Socked;
	while(fgets(sender, sizeof(sender), stdin)){
		printf("\r");
		//whenever enter a string, send it
		int messageSize = strlen(sender) + 1;
		write(*SockedCopy, &messageSize, sizeof(int));
 		int i = write(*SockedCopy, sender, messageSize);      //both work when sending message
		//check whether this is a quit message
		if(strcmp(sender, ":q!\n") == 0)
			exit(1);
		else if(sender[1] == 'f' && sender[2] == '!')
		{	
			printf("please enter the file name again( including address):\n");
			scanf("%s", Filename);
			FILE *fp=fopen(Filename, "r");
			fseek(fp, 0L, SEEK_END);
			int Filesize=ftell(fp); 
			int intSize = sizeof(int);
			write(*SockedCopy, &intSize, sizeof(int));
			write(*SockedCopy, &Filesize, sizeof(int));
	                Sendfile( Filename, SockedCopy );
			}			
	}
}

//receive message from server
void* Receive(void* Socked)
{
	int *SockedCopy = Socked;
	char Receiver[80];

	while(1){
		//read message continuosly
		int reveiverEnd = 0;
		reveiverEnd  = read (*SockedCopy, Receiver, 1000);
		Receiver[reveiverEnd] = '\0';	
		fputs(Receiver, stdout);
		Receiver[0] = '\0';
	}
}

//receive file from server
void* ReceiveFile(char* Filename, int Filesize)
{
}

int main ()
{
	int sockfd, n;
	pthread_t threadSend;
	pthread_t threadReceive;
	struct sockaddr_in serv, cli;
	char rec[1000];
	char send[80];
	char serAddress[80];
	
	//input server address
	printf("Input server address(type <Enter> to use default): ");
	fgets(serAddress, sizeof(serAddress), stdin);
	if(serAddress[0] == '\n')
	{
		strcpy(serAddress, "127.0.0.1\n");
	}
	serAddress[strlen(serAddress) - 1] = '\0';

	//input UserName
	Start: printf("Input Username: " );
	fgets(send, sizeof(send), stdin);
	send[strlen(send) - 1] = '\0';
 	int MessageSize = strlen(send);

	//create socked
 	sockfd = socket (PF_INET, SOCK_STREAM, 0);

	//create server information
 	bzero (&serv, sizeof (serv));
	serv.sin_family = PF_INET;
	serv.sin_port = htons (8888);
 	serv.sin_addr.s_addr = inet_addr (serAddress /*server address*/);

	//connect to the server
	if(connect (sockfd, (struct sockaddr *) &serv, sizeof (struct sockaddr)) == -1)
	{
		printf("connect failed\n");
		exit(1);
	}

	//send the user name to the server
	write(sockfd, &MessageSize, sizeof(int));
 	write (sockfd, send, sizeof(send));
	send[0] = '\0';

	//get successfully connecting message
	n = read (sockfd, rec, 1000);//n marks real length
 	rec[n] = '\0';	

	//check whether been rejected
	if(rec[0] == 'R')
	{
		rec[0] = '\0';
		printf("Username existed, choose another one.\n");
	 	goto Start; 
	}
	else
	{	
		fputs(rec, stdout);

		//open send thread 	
		pthread_create(&threadSend, 0, Send, &sockfd);

		//open receiving message thread
		pthread_create(&threadReceive, 0, Receive, &sockfd);
	}

	//close socked and close two threads
	for(int i = 0; i < 100; ++i)
		sleep(100000);
	pthread_exit(&threadSend);
	pthread_exit(&threadReceive);
	close(sockfd);
	
	return 0;
}
