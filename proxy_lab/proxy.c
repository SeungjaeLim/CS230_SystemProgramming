#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

#define MAXLINE 8192

static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";

void doit(int fd); //프록시 기능
void write_requesthdrs(rio_t *rp, char *proxy_request); //request 작성
void parse_uri(char *uri, int *port, char *hostname, char *path); //hostname path로 파싱
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg); //에러 메세지
void *thread(void *vargp); //쓰레드

int main(int argc, char **argv)
{
    int listenfd, connfd; //듣기 식별자, 연결 식별자
    int *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    if(argc != 2) //hostname, port로 들어오지 않은 경우
    {
        fprintf(stderr, "usage : %s <port>\n", argv[0]); 
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);

    listenfd = Open_listenfd(argv[1]);//호스트 char * hostname, 포트 char* port로 서버와 연결함. 열린 소켓 식별자 리턴 
    for(;;)
    {
        clientlen = sizeof(struct sockaddr_storage); //clientaddr : 소켓 주소 구조체
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); //듣기 식별자 받음
        //Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); //소켓 주소 구조체 -> 호스트 서비스이름 스트링으로 변환
        //printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp);
        //Close(connfd); //종료
    }
    return 0;
}

void doit(int connfd)
{
    int proxy_connfd; 
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    char proxy_request[MAXLINE];
    rio_t rio_client, rio_proxy;
    int port_int;
    char port_string[5];
    int line_length;

    port_int = 80; //기본 web

    Rio_readinitb(&rio_client, connfd); //fd를 읽음
    Rio_readlineb(&rio_client, buf, MAXLINE); //buf : request headers 예시 : GET http://www.cmu.edu/hub/index.html HTTP/1.1
    sscanf(buf, "%s %s %s",method,uri,version); //헤더의 메쏘드, uri, 버전 저장
    if (strcasecmp(method, "GET")) 
    { //GET만 구현                
        clienterror(connfd, method, "501", "Not Implemented", "Tiny Server does not implement this method");
        return; 
    }
    parse_uri(uri, &port_int, hostname, path); //http://www.cmu.edu/hub/index.html에서 www.cmu.edu 와 /hub/index.html로 바꿈

    sprintf(port_string, "%d", port_int);  //기본 80, parse를 통해 프리포트로 바뀔 port를 문자열로 저장
    proxy_connfd = Open_clientfd(hostname, port_string); //프록시가 클라이언트가 되어 host에 접속
    if(proxy_connfd == -1) //실패
    {
        return;
    }
    Rio_readinitb(&rio_proxy, proxy_connfd); //웹으로부터 읽어올 RIO초기화

    sprintf(proxy_request, "GET %s HTTP/1.1\r\n", path); //proxy가 웹으로 보낼 request 작성
    write_requesthdrs(&rio_client, proxy_request); //request의 헤더 작성

    sprintf(proxy_request, "%sHost: %s\r\n", proxy_request, hostname); //request의 헤더 작성
    sprintf(proxy_request, "%s%s%s%s", proxy_request, user_agent_hdr, conn_hdr, prox_hdr); //request의 헤더 작성
    sprintf(proxy_request, "%s\r\n", proxy_request); //마지막 \r\n 작성

    Rio_writen(proxy_connfd, proxy_request, strlen(proxy_request)); //웹 서버로 request 작성
    for(; (line_length = Rio_readlineb(&rio_proxy, buf, MAXLINE)) != 0;) //web에서 받은 내용 buf에 저장하여 한줄씩 받음
    {
        Rio_writen(connfd, buf, line_length); //클라이언트에게 작성
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) //에러 메세지 출력 함수
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void parse_uri(char *uri, int *port, char *hostname,char *path) //uri를 파싱
{
    char* tmp_hostname; //hostname으로 덮어씌워질 문자열
    char* tmp_path; //path으로 덮어씌워질 문자열

    tmp_hostname = strstr(uri,"//"); // //www.cmu.edu/hub/index.html

    tmp_hostname = tmp_hostname + 2; // www.cmu.edu/hub/index.html

    tmp_path = strstr(tmp_hostname, ":"); // http://localhost:4501/home.html 에서 :를 기준으로 잘라 프리포트 받음
    if(tmp_path == NULL) //프리포트 아닌경우
    {
        tmp_path = strstr(tmp_hostname, "/"); // /r기준으로 자름
        if(tmp_path != NULL) // www.cmu.edu/hub/index.html같은 경우
        {
            *tmp_path = '\0'; // hostname 파싱을 위해 / -> NULL (strncpy는 \0기준으로 자름)
            strncpy(hostname, tmp_hostname, MAXLINE); //hostname 파싱
            *tmp_path = '/'; // NULL -> /
            strncpy(path, tmp_path, MAXLINE); //path 파싱
        }
        else // www.cmu.edu 같은 경우
        {
            strncpy(hostname, tmp_hostname, MAXLINE); //hostname 파싱
            strcpy(path,""); //path는 NULL이 아닌 빈 문자열
        }
    }
    else //프리포트인경우
    {
        *tmp_path = '\0'; //:를 hostname 파싱을 위해 NULL로
        strncpy(hostname, tmp_hostname, MAXLINE); //hostname 파싱
        sscanf(tmp_path + 1, "%d%s", port, path); //:이후로 port||/~~~ 로 port 프리포트, path 설정
    }
    return; 
}

void write_requesthdrs(rio_t *rp, char *proxy_request)
{
    char buf[MAXLINE]; //Rio 받아올 버퍼
    
    int Content_length = 0; 

    for(;Rio_readlineb(rp, buf, MAXLINE);) //rp 한줄씩 읽기
    {
        if(strcmp(buf, "\r\n") == 0) //마지막이 나온 경우 루프 탈출
        {
            break;
        }
        if (strstr(buf,"Host:") != NULL) //있으면 패스
        {
            continue;
        }
        if (strstr(buf,"User-Agent:") != NULL)  //있으면 패스
        {
            continue;
        }
        if (strstr(buf,"Connection:") != NULL)  //있으면 패스
        {
            continue;
        }
        if (strstr(buf,"Proxy-Connection:") != NULL)  //있으면 패스
        {
            continue;
        }
        if (strncasecmp(buf, "Content-length:", 15) == 0) 
        {
            sscanf(buf + 15, "%u", &Content_length); //14+'\0' = 15 content-length 덮어씌움
        }
        sprintf(proxy_request,"%s%s", proxy_request,buf); //host, user-agent 등등 없는 것 있을 경우 buf에 저장되어 있어서 덮어씌움
    }
    return;
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);                                             
    Close(connfd);  
    return NULL;
}