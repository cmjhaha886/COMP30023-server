#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

//constant variables
#define MAXCLIENT 10
#define BUFFERSIZE 1024
#define REQUESTLEN 4096
#define PATHLEN 1000
#define ROOTLEN 100
#define CONTENTLEN 100

//a struct defined to store the information needed for a thread
struct pthreadargs{
	int nsockfd;
	char *root_path;
};

void *respond(void* arg);
void send_content(int newsockfd, char *file_path);

int main(int argc, char **argv){
        
    int sockfd, newsockfd, portno;
	char rootpath[ROOTLEN];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	pthread_t tid;

    //need port number and root path
	if (argc < 3) {
		fprintf(stderr,"ERROR, no port or path provided\n");
		exit(0);
	}

	 // Create TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	//initialize server address
	bzero((char *) &serv_addr, sizeof(serv_addr));
    //store port number and root path
	portno = atoi(argv[1]);
	strcpy(rootpath, argv[2]);
	
	// create address going to listen on
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	// converted to network byte order
	serv_addr.sin_port = htons(portno);

	 // Bind address to the socket 	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}
	
	// listen on socket
	if (listen(sockfd, MAXCLIENT) != 0){
		perror("ERROR on listening");
		exit(1);
	}
	
	//waiting for connection request
	while (1){
		clilen = sizeof(cli_addr);
		//accept connection request
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    if (newsockfd < 0) {
	    	perror("ERROR on accept");
	    	exit(1);
	    }
	    else{
	    	//create a thread to respond the request
	    	struct pthreadargs *pthread_arg = malloc(sizeof(struct pthreadargs));
	    	pthread_arg->nsockfd = newsockfd;
	    	pthread_arg->root_path = rootpath;
	    	pthread_create(&tid, NULL, respond, pthread_arg);
	    }
	}
	return 0;
}

//the function that a thread uses to repond a request
void *respond(void* arg){
	char buffer[BUFFERSIZE], message[REQUESTLEN], *reqline[3], path[PATHLEN], root_path[ROOTLEN];
	int bytes, newsockfd;
	FILE *fp;
	struct pthreadargs *pthread_arg;

	//cast the argument and store the information inside
	pthread_arg = (struct pthreadargs *)arg;
	strcpy(root_path, pthread_arg->root_path);
	newsockfd = pthread_arg->nsockfd;

    //initialize the buffer
	bzero(buffer, BUFFERSIZE);

	//receive the message and store it
	recv(newsockfd, message, REQUESTLEN, 0);
	reqline[0] = strtok (message, " \r\n");

	//only deal with "get" request
	if (strcmp(reqline[0], "GET") == 0){
		reqline[1] = strtok(NULL, " ");
		reqline[2] = strtok(NULL, " ");
		strcpy(path, root_path);
		strcat(path, reqline[1]);

		//return 404 message if the file is not found
		if ( (fp = fopen(path, "r")) == NULL){
			send(newsockfd, "HTTP/1.0 404 Not Fount\r\n\r\n", 24, 0);
		}
		else{

			//send 200 success message if file exists
			send(newsockfd, "HTTP/1.0 200 OK\r\n", 17, 0);
			send_content(newsockfd, reqline[1]);

			//send the file by buffers separately
			while ((bytes = fread(buffer, sizeof(char), BUFFERSIZE, fp)) > 0){
				send(newsockfd, buffer, bytes, 0);
			}
			fclose(fp);
		}
	}
	//everything is done with thie request
	close(newsockfd);
	free(pthread_arg);
	return NULL;
}

//the function to send the content type line of the header
void send_content(int newsockfd, char *file_path){
	int path_len;
	path_len = strlen(file_path);
	int i = path_len;
	int j = 0;
	char content_line[CONTENTLEN];
	//look for the "." from the end
	while (file_path[i-1] != '.'){
		i--;
	}
	char extension[8];
	//store the extension into the char array
	while (i < (int)strlen(file_path)){
		extension[j] = file_path[i];
		j++;
		i++;
	}
	extension[j] = '\0';
	//modify the content type line
	strcpy(content_line, "Content-Type: ");
	if (strcmp(extension, "html") == 0){
		strcat(content_line, "text/html\r\n\r\n");
	}
	else if (strcmp(extension, "jpg") == 0){
		strcat(content_line, "image/jpeg\r\n\r\n");
	}
	else if (strcmp(extension, "css") == 0){
		strcat(content_line, "text/css\r\n\r\n");
	}
	else if (strcmp(extension, "js") == 0){
		strcat(content_line, "application/javascript\r\n\r\n");
	}
	int content_len = strlen(content_line);
	send(newsockfd, content_line, content_len, 0);
}











