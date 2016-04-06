#include"httpd.h"
//acquire listen socket
int listensocket(unsigned short port)
{
	int listen_sock=socket(AF_INET/*ipv4*/,SOCK_STREAM,0);
	if(listen_sock<0)
	{
		perror("socket");
		exit(1);
	}
	int flag = 0;
	setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_port=htons(port);
	local.sin_addr.s_addr=htonl(INADDR_ANY);
	int ret=bind(listen_sock,(struct sockaddr*)&local,sizeof(local));
	if(ret<0)
	{
		perror("bind");
		exit(1);
	}
	//bind success
	
	ret=listen(listen_sock,5);
	if(ret<0)
	{
		perror("listen");
		exit(1);
	}
	return listen_sock;
}

int get_line(int data_client,char buf[],int len)
{
	if(buf==NULL||len<0)
	{
		exit(1);
	}
	int i=0;
	int n=0;
	char ch='\0';
	while(i<len-1&&ch!='\n')
	{
		n=recv(data_client,&ch,1,0);
		if(ch=='\r')
		{
			n=recv(data_client,&ch,1,MSG_PEEK);
			if(ch=='\n')
			{
				n=recv(data_client,&ch,1,0);
			}
			else
			{
				ch='\n';
			}
		}
		buf[i++]=ch;
	}
	buf[i]='\0';
	return i;
}
static void bad_request(int data_sock)
{
	printf("enter our fault...\n");
	char buf[1024];
	sprintf(buf,"HTTP/1.0 400 BAD REQUEST\r\n");
	send(data_sock,buf,strlen(buf),0);
	sprintf(buf,"Content-type:text/html\r\n");
	send(data_sock,buf,strlen(buf),0);
	sprintf(buf,"\r\n");
	send(data_sock,buf,strlen(buf),0);
	sprintf(buf,"<html></br><p>your enter message is a bad request</p></br></html>\r\n");
	send(data_sock,buf,strlen(buf),0);
}
void return_error_client(int data_sock,int error_code)
{
	switch(error_code)
	{
		case 400://request error
//			bad_request(data_sock);
			break;
		case 404://not found
//			not_found(data_sock);
			break;
		case 500://server error
//			server_error(data_sock);
			break;
		case 503://server unavailable
//			server_unavailable(data_scok);
			break;
		default:
//			default_error(data_sock);
			break;
	}
}

void clear_header(int client)
{
	char buf[BLOCK_SIZE];
	int ret=-1;
	do
	{
		ret=get_line(client,buf,sizeof(buf)-1);
	}while(ret>0 && strcmp(buf,"\n")!=0);
}
void echo_html(int data_sock,char*path,size_t size)
{
	clear_header(data_sock);
	int fd = open(path,O_RDONLY,0);
	if(fd<0)
	{
		return_error_client(data_sock,404);
		return ;
	}
	else
	{
		sendfile(data_sock,fd,NULL,size);
	}
	close(fd);
}

void exe_cgi(int data_sock,const char*path,const char*method,const char*query_string)
{
	int numchars=-1;
	char buf[BLOCK_SIZE];
	int content_length=-1;

	int cgi_input[2]={0,0};
	int cgi_output[2]={0,0};
	pid_t id;

	if(strcasecmp(method,"GET")==0)//GET
	{
		clear_header(data_sock);
	}
	else//POST
	{

		do
		{
			memset(buf,'\0',sizeof(buf));
			numchars=get_line(data_sock,buf,sizeof(buf)-1);
			if(strncasecmp(buf,"Content-Length:",15)==0)
			{
				content_length=atoi(&buf[16]);
			}
		}while(numchars>0&&strcmp("\n",buf));
		if(content_length==-1)
		{
			return_error_client(data_sock,404);
			return ;
		}
	}
		memset(buf,'\0',sizeof(buf));
		strcpy(buf,HTTP_VERSION);
		strcat(buf,"200 OK\r\n\r\n");
		send(data_sock,buf,strlen(buf),0);
		if(pipe(cgi_input)<0)
		{
			perror("pipe");
			return ;
		}
		if(pipe(cgi_output)<0)
		{
			close(cgi_input[0]);
			close(cgi_output[1]);
			perror("pipe1");
			return ;
		}
		id=fork();
		if(id<0)
		{
			close(cgi_input[0]);
			close(cgi_input[1]);
			close(cgi_output[0]);
			close(cgi_output[1]);
			perror("fork");
			return ;
		}
		else if(id==0)
		{//child
			char query_env[BLOCK_SIZE];
			char method_env[BLOCK_SIZE];
			char content_len_env[BLOCK_SIZE];

			memset(query_env,'\0',sizeof(query_env));
			memset(method_env,'\0',sizeof(query_env));
			memset(content_len_env,'\0',sizeof(query_env));
			close(cgi_input[1]);
			close(cgi_output[0]);

			dup2(cgi_input[0],0);
			dup2(cgi_output[1],1);

			sprintf(method_env,"REQUEST_METHOD=%s",method);
	
			putenv(method_env);
			printf("method=%s\n",getenv("REQUEST_METHOD"));
			if(strcasecmp("GET",method)==0)
			{
				sprintf(query_env,"QUERY_STRING=%s",query_string);
				putenv(query_env);
			}
			else
			{//POST
				printf("hah a");
				printf("%d\n",content_length);
				sprintf(content_len_env,"CONTENT_LENGTH=%d",content_length);
				putenv(content_len_env);
				printf("conten-len:%s\n",getenv("content_len_env"));
			}

			execl(path,path,NULL);
			exit(1);
		}
		else
		{//father
			close(cgi_input[0]);
			close(cgi_output[1]);

			int i=0;
			char ch='\0';
			if(strcasecmp("POST",method)==0)
			{
				for(;i<content_length;i++)
				{
					recv(data_sock,&ch,1,0);
					write(cgi_input[1],&ch,1);
				}
			}
			while(read(cgi_output[0],&ch,1)>0)
			{
				send(data_sock,&ch,1,0);
			}
			close(cgi_input[1]);
			close(cgi_output[0]);

			waitpid(id,NULL,0);
		}
	
}
void* accept_request(void*arg)
{
		printf("i am pthread\n");
		int data_sock=(int)arg;
		
		int cgi=0;
		char *query_string=NULL;
		char path[BLOCK_SIZE];

		char buf[BLOCK_SIZE];
		char method[BLOCK_SIZE];
		char url[BLOCK_SIZE];

		memset(path,'\0',sizeof(path));
		memset(buf,'\0',sizeof(buf));
		memset(method,'\0',sizeof(method));
		memset(url,'\0',sizeof(url));


#ifdef _DEBUG_
		while(get_line(data_sock,buf,sizeof(buf))>0)
		{
			printf("%s",buf);
			memset(buf,'\0',sizeof(buf));
		}

#endif  //_DEBUG_

		if(get_line(data_sock,buf,sizeof(buf))<1)
		{
			return_error_client(data_sock,404);
			close(data_sock);
			return (void*)1;
		}//get_line failed

		
		int i=0;//method index
		int j=0;//buf index
		while(!isspace(buf[j])&&i<sizeof(method)-1&&j<sizeof(buf))
		{
        	method[i]=buf[j];
			i++;
			j++;
		}//acquire request method

		if(strcasecmp(method,"GET")&&strcasecmp(method,"POST"))
		{
			return_error_client(data_sock,404);
			close(data_sock);
			return (void*)1;
		}
		
		//POST method execult cgi
		if(strcasecmp(method,"POST")==0)
		{
			cgi=1;
		}

		while(isspace(buf[j])&&j<sizeof(buf))
		{
			j++;
		}//skip isspace


		i=0;//url index
		while(!isspace(buf[j])&&i<sizeof(url)-1&&j<sizeof(buf))
		{
			url[i]=buf[j];
			i++;
			j++;
		}//acquire url

		if(strcasecmp(method,"GET")==0)
		{
			query_string=url;
			while(*query_string!='?'&&*query_string!='\0')
			{
				query_string++;
			}
			if(*query_string=='?')
			{
		
				cgi=1;
				*query_string='\0';
				query_string++;
			}
		}
		
		bzero(buf,sizeof(buf));
		sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
		send(data_sock,buf,strlen(buf),0);

		sprintf(path,"htdocs%s",url);//output htdocs%s to path
		if(path[strlen(path)-1]=='/')//directory
		{
			strcat(path,MAIN_PAGE);//return index.html
		}

		struct stat st;
		if(stat(path,&st)<0)
		{//not found path,is this execult file not exist
			return_error_client(data_sock,404);
			close(data_sock);
			return (void*)1;
		}
		else
		{
			if(S_ISDIR(st.st_mode))
			{
				strcat(path,"/");
		 		strcat(path,"MAIN_PAGE");
			}
			else if((st.st_mode & S_IXUSR)||
		 			(st.st_mode & S_IXGRP)||
					(st.st_mode & S_IXOTH))
			{
				cgi=1;
			}
		}
		if(cgi)
		{
			exe_cgi(data_sock,path,method,query_string);
		}
		else
		{
			echo_html(data_sock,path,st.st_size);
		}

		close(data_sock);
		return NULL;
}
	
int main(int argc,char*argv[])
{	
	if(argc!=2)
	{
		printf("format: ./httpd [port]\n");
		exit(1);
	}
	unsigned short port=atoi(argv[1]);
	int listen_sock=listensocket(port);
	
	struct sockaddr_in client_info;
	int client_info_len=0;

	while(1)
	{
		int data_sock=accept(listen_sock,(struct sockaddr*)&client_info,&client_info_len);
		if(data_sock<0)
		{
			perror("accept");
			exit(1);
		}
		printf("get a new link\n");
		pthread_t tid;
		pthread_create(&tid,NULL,accept_request,(void*)data_sock);
		pthread_detach(tid);
	}
	close(listen_sock);
	return 0;
}


