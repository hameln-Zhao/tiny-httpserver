#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <mcheck.h>

//#define PORT 8000
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"



int start_server(u_short *);
void process_request(int);
void error_die(const char *);
int get_line(int, char *, int);
void unimplemented(int fd);
void serve_file(int, const char *);
void not_found(int);
void headers(int, const char *);
void cat(int, FILE *);
void execute_fakecgi(int client);
void serve_file_post(int client, const char *filename);


int main() {
    int fd_server;
    int fd_client;
    u_short port = 8000;
    fd_server = start_server(&port);
    printf("running on\n");

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof (client_addr);
    pthread_t newthread;

    mtrace();
    while(1){
        fd_client = accept(fd_server,&client_addr,&client_addr_len);
        if(fd_client == -1){
            error_die("accept error\n");
        }
        if(pthread_create(&newthread , NULL , (void *) process_request , (void *)(intptr_t)fd_client) != 0){
            perror("pthread_create\n");
        }
        else{

            printf("newthread creat\n");

        }


    }
}

int start_server(u_short *port){
    int fd;
    int opt = 1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof (opt));  //端口复用
    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        error_die("socket error\n");
    }

    struct sockaddr_in addr;
    bzero(&addr,sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(* port);

    if(bind(fd,&addr,sizeof (addr)) < 0){
        error_die("bind error\n");
    }

    if(listen(fd,5) < 0){
        error_die("listen error\n");
    }

    return fd;
}

void process_request(int fd){
    printf("process new request\n");
    char buff[1024];
    char method[255];
    char url[255];
    char path[512];
    int numchars;
    int cgi = 0;
    char *query_string = NULL;
    struct stat st;
    char path_unknownmethod[20];
    size_t i,j;
    i = 0;//method url
    j = 0;//buff

    numchars = get_line(fd,buff,sizeof (buff));
    while(!isspace(buff[j]) && (i<sizeof(method)-1)){
        method[i] = buff[j];
        i++;
        j++;
    }
    method[i] = '\0';
    printf("current method is %s\n",method);

    if(strcasecmp(method,"GET") && strcasecmp(method,"POST")){
        unimplemented(fd);
        printf("method error\n");
        return ;
    }

    else if(strcasecmp(method,"POST") == 0){
        cgi = 1;
    }
    else{

    }
    i = 0;
    //skip space
    while(isspace(buff[j]) && j<sizeof (buff)){
        j++;
    }

    while(!isspace(buff[j]) && i<sizeof(url)-1 && j < sizeof (buff)){
        url[i] = buff[j];
        i++;
        j++;
    }
    url[i] = '\0';

    sprintf(path,"/home/hameln/html%s",url);

    printf("%s\n",path);

    if(path[strlen(path)] -1 == '/') {
        strcat(path, "index.html");
    }//if the path is a lujing ,add a file.

    //if cgi = 0 ,means there's only a GET request,no parameter.
    if(!cgi){
        printf("\n to execute server_file \n");
        serve_file(fd,path);
    }
    else if(cgi = 1){
        printf("\n toexecute execute_cgi \n");
        execute_fakecgi(fd);
    }
    else{
        printf("unknown method\n");
        strcpy(path_unknownmethod,"/home/hameln/html/c.html");
        serve_file(fd,path_unknownmethod);
    }

    close(fd);
}


void serve_file(int client, const char *filename) {
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    //确保 buf 里面有东西，能进入下面的 while 循环
    buf[0] = 'A';
    buf[1] = '\0';
    //循环作用是读取并忽略掉这个 http 请求后面的所有内容
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    //打开这个传进来的这个路径所指的文件
    resource = fopen(filename, "r");
    if (resource == NULL){
        not_found(client);
        printf("404\n");
    }
    else {
        //打开成功后，将这个文件的基本信息封装成 response 的头部(header)并返回
        headers(client, filename);
        //接着把这个文件的内容读出来作为 response 的 body 发送到客户端
        cat(client, resource);
    }
    fclose(resource);
}


void serve_file_post(int client, const char *filename) {
    FILE *resource = NULL;
    //打开这个传进来的这个路径所指的文件
    resource = fopen(filename, "r");
    if (resource == NULL){
        not_found(client);
        printf("404\n");
    }
    else {
        //打开成功后，将这个文件的基本信息封装成 response 的头部(header)并返回
        headers(client, filename);
        //接着把这个文件的内容读出来作为 response 的 body 发送到客户端
        cat(client, resource);
    }
    fclose(resource);
}


void execute_fakecgi(int client)
{
    char buf[1024];
    int i=0;
    char user[10] = "\0";
    char passwd[10] = "\0";
    const char path1[255];
    const char path2[255];

    strcpy(path1,"/home/hameln/html/a.html");
    strcpy(path2,"/home/hameln/html/b.html");
    while(buf[0] != '-'){
        get_line(client,buf,sizeof (buf));
    }
    while (i<3)  {
        get_line(client, buf, sizeof(buf));
        i++;
    }

    strcat(user,buf);
    user[strlen(user)-1] = '\0';
    printf("\nusername:%s\n",user);
    while('-' != buf[0]){
        get_line(client,buf,sizeof (buf));
    }
    i = 0;
    while(i<3){
        get_line(client,buf,sizeof (buf));
        i++;
    }
    strcat(passwd,buf);
    passwd[strlen(passwd)-1] = '\0';
    printf("\npassword:%s\n",passwd);
    get_line(client, buf, sizeof(buf));




    if(strcmp(user,"hameln\0") ==0 && strcmp(passwd,"123456\0") == 0){
        serve_file_post(client,path1);
    }
    else{
        serve_file_post(client,path2);
    }

}

void error_die(const char *sc) {
    perror(sc);
    exit(1);
}

int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        //recv()包含于<sys/socket.h>,参读《TLPI》P1259,
        //读一个字节的数据存放在 c 中
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0) {
            if (c == '\r') {
                //
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}

void unimplemented(int fd) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(fd, buf, strlen(buf), 0);
}

void not_found(int client) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

void headers(int client, const char *filename) {
    char buf[1024];
    (void) filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.1 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Connection: Keep-Alive\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Cache-Control: max-age = 10 ,public\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

}

void cat(int client, FILE *resource) {
    char buf[1024];

    //从文件文件描述符中读取指定内容
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}
