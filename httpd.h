#ifndef _HTTPD_H_
#define _HTTPD_H_


#include<stdio.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>

#define BLOCK_SIZE 1024
#define MAIN_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"

#endif
