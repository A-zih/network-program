#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOcTYPE html>\r\n"
"<html><head><title>Demo_select</title>\r\n"
"<style>body { background-color: white}</style></head>\r\n"
"<body><center><h1>Hello world select version!</h1><br>\r\n"
"<img src=\"irving.jpg\"></center></body></html>\r\n";

int main(int argc, char *argv[]){
	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len=sizeof(client_addr);
	int fd_server, fd_client,fd_max;
	char buf[2048];
	int fdimg;
	int on=1,i;
	fd_set active_fd_set;

	fd_server=socket(AF_INET,SOCK_STREAM,0);
	if(fd_server<0){
		perror("socket");
		exit(1);
	}
	setsockopt(fd_server,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8080);

	if(bind(fd_server,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1){
		perror("bind");
		close(fd_server);
		exit(1);
	}
	if(listen(fd_server,10)==-1){
		perror("listen");
		close(fd_server);
		exit(1);
	}
	FD_ZERO(&active_fd_set);
	FD_SET(fd_server,&active_fd_set);
	fd_max=fd_server;
	while(1){
		int ret;
		struct timeval tv;
		fd_set read_fds;
		
		/*set timeout*/
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		/*copy fd set*/
		read_fds = active_fd_set;
		ret = select(fd_max+1,&read_fds,NULL,NULL,&tv);
		if(ret==-1){
			perror("select");
			return -1;
		}
		else if(ret==0){
			printf("select timeout\n");
			continue;
		}
		else{
			/* select all sockets*/
			for(i=0;i<FD_SETSIZE;i++){
				if(FD_ISSET(i,&read_fds)){
					if(i==fd_server){
						/*accept*/
						fd_client = accept(fd_server,(struct sockaddr *)&client_addr,&sin_len);
						if(fd_client==-1){
							perror("accept");
							return -1;
						}
						else{
							printf("connected!!\n");
							read(fd_client,buf,2047);
							if(strncmp(buf,"GET /irving.jpg",15)==0){
								fdimg = open("irving.jpg",O_RDONLY);
								sendfile(fd_client,fdimg,NULL,51000);
								close(fdimg);
							}
							else
								write(fd_client,webpage,sizeof(webpage)-1);

							/*add to fd set*/
							FD_SET(fd_client,&active_fd_set);
							if(fd_client>fd_max)
								fd_max=fd_client;
						}
					}
					else{
						int recv_len;

						/*receive*/
						memset(buf,0,sizeof(buf));
						recv_len = recv(i,buf,sizeof(buf),0);
						if(recv_len==-1){
							perror("recv");
							return -1;
						}
						else if(recv_len==0)
							printf("Client disconnect\n");
						else{
							printf("Receive: len=[%d] msg=[%s]\n",recv_len,buf);

							/*send (in fact we should determine when it can be written)*/
							send(i,buf,recv_len,0);
						}
						/*clean up*/
						close(i);
						FD_CLR(i,&active_fd_set);
					}
				}
			}
		}
	}
	return 0;
}
