/* Title	:	A minimal SSH client for the gutpSSH Server			*/
/* Source	:	ssh_client.c 							*/	
/* Date		:	12/03/2012	   						*/
/* Author	:	Alok Upadhyay	   						*/
/* Input	:	The incoming connections from user
			- Port 2222 for control messages and data transfer		*/
/* Output	:	The server prompts, the error/warning messages.			*/
/* Method	:	A well planned execution of Linux function calls		*/
/* Possible Further Improvements :	1. User account creation/authentication -not done, since it is tftp type only
					2. A good/innovative prompt.		-done
					3. An impressive banner! :P		-done
					4. Serve users across multiple sessions -done
					5. Serve multiple users at the same time-not done 	
					6. How to recieve/compile using sendfile()-done	*/


/* Included header files */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>		// For SHA1() function.


/* Pre-processor directives */
#define DELIM " "


/* Functions and subroutines declaration */
void sendUsername(char *);
void hash_pass(unsigned char *, unsigned char *);
void waitForServer();
void writeToFile(char *, char *);
void transferFile(char *);
void sendPassword();
void get_hidden_pass(char *);


/* Global Variables */
int sock, bytes_recieved, file_sock, file_bytes_recieved;  
char send_data[1024],recv_data[4096], recv_file_data[4096], send_file_data[4096];
struct hostent *host;
struct sockaddr_in server_addr, file_server_addr;  
unsigned char pass_plain[20], pass_hashed[20];

int main(int argc, char **argv)

{
	if ( argc != 2 ) /* argc should be 2 for correct execution */
    	{
      	  	 /* We print argv[0] assuming it is the program name */
       		 printf( "usage: %s <user-name>@<server-ip-address>\n", argv[0] );
    	}
   	
	else 
 	{
			/** Splitting the argv[1] to extract the username */
			char * user_name ,  * server_ip ;
			user_name = strtok(argv[1], "@");
			server_ip = strtok(NULL, "@");

			
		        host = (struct hostent *)gethostbyname(server_ip); 

			/** Control socket declaration */	
		        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // TCP connection
		            perror("Socket");
		            exit(1);
		        }

		        server_addr.sin_family = AF_INET;     
		        server_addr.sin_port = htons(2222);    // Port to connect to server
		        server_addr.sin_addr = *((struct in_addr *) host->h_addr);
		        bzero(&(server_addr.sin_zero),8); 

		        if (connect(sock, (struct sockaddr *)&server_addr,
		                    sizeof(struct sockaddr)) == -1) 
		        {
		            perror("Control Connect");
		            exit(1);
		        }
			/** Control socket declaration ends*/	

			
			/** Sending the username entered on the commandline to the server to know if there is an account */
			sendUsername(user_name);
			bytes_recieved=recv(sock,recv_data,1024,0);
         		recv_data[bytes_recieved] = '\0';
			printf("%s", recv_data);
			if(!strcmp(recv_data, "NO-EXIST\n"))
				exit(1);
			else
			{
				sendPassword();	
			}	
	}	

	return (0);
}

void sendPassword()
{
	//Recieving password prompt
	bytes_recieved=recv(sock,recv_data,1024,0);
	recv_data[bytes_recieved] = '\0';
	
	//Print prompt
	puts(recv_data);

	gets(pass_plain);
	hash_pass(pass_plain, pass_hashed);
	send(sock,pass_hashed,strlen(pass_hashed), 0);

		
			
	//Recieve auth information.
	bytes_recieved=recv(sock,recv_data,1024,0);
	recv_data[bytes_recieved] = '\0';

	fflush(stdout);

	printf("%s", recv_data);

	if(!strcmp(recv_data, "authorized"))
	{
		waitForServer();
	}

	else
	{
		printf("Authorization failed.\n");
		sendPassword();
	}
}


void sendUsername(char *username)
{
	send(sock,username,strlen(username), 0);
}


void hash_pass(unsigned char *plain, unsigned char *hashed)
{
	SHA1(plain, strlen(plain), hashed);	
}

	
void waitForServer()
{
	//Recieving initial banner information through the control socket.
	 bytes_recieved=recv(sock,recv_data,1024,0);
         recv_data[bytes_recieved] = '\0';
         printf("%s" , recv_data);


        while(1)
        {
        
	//Recieving the customary prompt and response of the previous command.	
		int i=0;
		while(i<4096)
		{	
			recv_data[i] = '\0';	
			i++; 
		}
	
	        bytes_recieved=recv(sock,recv_data,4096,0);
	        recv_data[bytes_recieved] = '\0';
	        printf("%s \b " , recv_data);
       		
	  	gets(send_data);
          
           
                	
		if(!strcmp(send_data, "bye"))
		{
			send(sock,send_data,strlen(send_data), 0); 
			close(sock);
			exit(0);			
			
		}


		else if (!strncmp(send_data, "get", 3))
		{

			send(sock,send_data,strlen(send_data), 0);
			bytes_recieved=recv(sock,recv_data,4096,0);
		        recv_data[bytes_recieved] = '\0';
          		
			printf("File data recieved.\n"  , recv_data);
			
			//send_data will contain the filename; recv_data will contain the actual file data;
			writeToFile(send_data, recv_data);	
			
			int i=0;
			while(i<4096)
			{	
				recv_data[i] = '\0';	
				if(i<1024)
					send_data[i] = '\0';
				i++; 
			}
			

		}

		
		else if (!strncmp(send_data, "put", 3))
		{
			send(sock, send_data, strlen(send_data), 0);	//sending the command to prepare server for data reception.			
			transferFile(send_data);	// will incur one send operation of the data			
		}

		else 
			send(sock,send_data,strlen(send_data), 0); 
        
        }   
	
	
}



void writeToFile(char *command, char *data)
{
	FILE *fp;
	
	char * first_arg ,  * second_arg ;
	first_arg = strtok(command, DELIM);
	second_arg = strtok(NULL, DELIM);

	fp = fopen(second_arg, "w");
	if (fp!=NULL)
 	 {
  		 fputs ( data,fp);
  	 	 fclose (fp);
  	 }

}

void transferFile(char *command)
{
	int fd;
	char * first_arg ,  * second_arg ;
	first_arg = strtok(command, DELIM);
	second_arg = strtok(NULL, DELIM);

	
	struct stat stat_buf;	/* argument to fstat */
	off_t offset = 0;          /* file offset */
	int rc;


	/* open the file to be sent */
	    fd = open(second_arg, O_RDONLY);
	    if (fd == -1) {
	      fprintf(stderr, "unable to open '%s': %s\n", second_arg, strerror(errno));
	      //exit(1);
	    }

	/* get the size of the file to be sent */
	    fstat(fd, &stat_buf);


    	/* copy file using sendfile */
	    offset = 0;
	    rc = sendfile (sock, fd, &offset, stat_buf.st_size);
	    if (rc == -1) {
	      fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
	      exit(1);
	    }
	    if (rc != stat_buf.st_size) {
	      fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
	              rc,
	              (int)stat_buf.st_size);
	      exit(1);
	    }
	
    	/* close descriptor for file that was sent */
	    close(fd);

	
		
}
