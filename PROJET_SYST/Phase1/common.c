#include "common_impl.h"

int creer_socket(int prop, int *port_num)
{
  int sockfd = 0;
  struct sockaddr_in serv_addr;

  /* fonction de creation et d'attachement d'une nouvelle socket */
  sockfd = do_socket(AF_INET, prop, IPPROTO_TCP);  //Socket initialisation
  serv_addr = init_serv_addr("8080", serv_addr); //Initialisation of server information
  do_bind(sockfd, serv_addr);  //We Bind on the port specified.
  do_listen(sockfd);  //Listen for most of at SOMAXCONN Clients.

  /* renvoie le numero de descripteur et modifie le parametre port_num */
  *port_num = serv_addr.sin_port;

  printf("Creer_socket: succes.\n");

  return sockfd;

}


int do_socket(int domain, int type, int protocol){

  int sockfd;
  int yes = 1;

  //create the socket
  sockfd = socket(domain, type, protocol);

  //check for socket validity
  if (sockfd < 0)
  perror("ERROR opening socket");

  //Reusing already in use adresses
  if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
  perror("Error socket option");

  return sockfd;
}

struct sockaddr_in init_serv_addr(const char* port,struct sockaddr_in serv_addr){

  memset(&serv_addr,0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(port));

  return serv_addr;

}

void do_bind(int sockfd, struct sockaddr_in serv_addr){
  if ( bind( sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
    perror ("bind");
}

void do_listen(int sockfd){
  if (listen(sockfd, SOMAXCONN) == -1 ) {
    perror( "listen: ");
  }
}


int do_accept(int sockfd, struct sockaddr_in *client_addr){
  struct sockaddr_in client_addr2 = *client_addr;
  int client_sock;
  socklen_t len = sizeof(client_addr2);

  //accept connection from client
  client_sock=accept(sockfd,(struct sockaddr *) &client_addr2, &len );
  client_addr->sin_addr=client_addr2.sin_addr;
  client_addr->sin_port=client_addr2.sin_port;
  if (client_sock == -1){
    perror("accept:");
  }

  return client_sock;

}

void do_connect(int sockfd, struct sockaddr_in sock_host, int size_host){
	if (connect(sockfd, (struct sockaddr *) & sock_host, size_host) == -1)
			perror("connect");
}

ssize_t send_line(int fd, void *buf, size_t len){

  /* Variables */
  int nleft;
  int nwritten;
  char * ptr;
  ptr = buf;
  nleft = len;

  /* perform the send */
  while( nleft > 0 ){
    if( (nwritten = write(fd, ptr, nleft)) <= 0) {
      if(errno == EINTR){
        nwritten = 0;
        perror("");
      }
      return -1;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }

  return len;
}

ssize_t display_line(char * buf, int len){
  int n=send_line(STDOUT_FILENO, buf, len);
  printf("\n");

  return n;

}

ssize_t read_line(int fd, char* buf, size_t len){

  memset(buf,'\0', MSG_SIZE);

  /* Variables */
  int i;
  char c;
  int ret=1;
  char * ptr;
  ptr = buf;
  int cnt = 0;

  /* Perform the read */
  for (i = 0 ; i < len; i++){

    ret = read(fd, &c, 1);

    if( ret == 1 && c != '\0'){
      ptr[cnt++] = c;

      if( c == '\n' && fd==STDIN_FILENO){
        //ptr[cnt-1] = '\0';
        return i+1;
      }
      if( c == '\0'){
        return i+1;
      }
    }
    else if( 0 == ret ) {
      //ptr[cnt] = '\0';
      break;
    }
  }
  ptr[len] = '\0';

  /* Empty stdin buffer in the case of too large user_input */
  if( fd == STDIN_FILENO && i == len ){
    char ss[10*MSG_SIZE];
    ret = read(fd, ss, 10*MSG_SIZE);
  }
  return i;
}

int get_number_lign(char *txt) {

  int fd;
  fd = open(txt ,O_RDONLY);
  int i=0,n;
  char c;

  while (1){
    n=read(fd,&c, 1);
    if (n==0)
      break;
    if (c =='\n')
    i++;
  }
  close(fd);
  return i;
}

int get_words_lign(char *txt, int number_lign, char msg[][NAME_MAX]){
  int j=0,k=0,n,fd;
  char c;
  fd = open(txt ,O_RDONLY);

  while (1){
    n=read(fd,&c, 1);
    msg[j][k]=c;
    k++;
    if (n==0)
      break;
    if (c=='\n'){
      msg[j][k-1]='\0';
      j++;
      k=0;
    }
  }
  close(fd);
  return 0;
}
