#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>


/* autres includes (eventuellement) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#define MSG_SIZE 1024
#define NAME_MAX 128


#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   int port_distant;
   int new_sockfd;
   int size_hostname;
   char *hostname;
};
typedef struct dsm_proc_conn dsm_proc_conn_t;

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int type, int *port_num);

int creer_socket_connect(char* addr, char* port);

int do_socket(int domain, int type, int protocol);

struct sockaddr_in init_serv_addr(const char* port, struct sockaddr_in serv_addr);

void do_bind(int sockfd, struct sockaddr_in serv_addr);

void do_listen(int sockfd);

int do_accept(int sockfd, struct sockaddr_in *client_addr);

void do_connect(int sockfd, struct sockaddr_in sock_host, int size_host);

ssize_t send_line(int fd, void *buf, size_t len);

ssize_t display_line(char * buf, int len);

ssize_t read_line(int fd, char * buf, size_t len);

int get_words_lign(char *txt, int number_lign, char msg[][NAME_MAX]);

int get_number_lign(char *txt);

char* get_argument(char source[MSG_SIZE], int no_arg);

struct processus_info;

int create_poll(struct pollfd *fds, int num_procs_create, int nfds, int timeout, dsm_proc_t *proc_array,int** pipefd_std, char* std_mode);
