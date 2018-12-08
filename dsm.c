
/* Deux fonctions a implementer: dsminit et dsmfileline
-- /!\ quand le processus distant execute dsminit après le dsmwrap.
Donc quand il est en train d'echanger il y a un bout d'information qui se fait
dans le dsmwrap et l'autre dans le dsminit. Ce n'est pas le meme contexte d'echange
mais c'est le meme processus donc ca marche.

-- mmap de la memoire partagee

-- tout le systeme repose sur les segmentations fault quand on essaye d'acceder a une page
dont on est pas proprietaire. Donc on met en place un traitant de signal. Pour recuperer
l'adresse qui a fait l'erreur on utilise un traitant etendu (segvhandler)

-- on fait un thread et on utilise dsm_comm_daemon pour y repondre

Differentes fonctions:

-- obtenir base_addr, l'adresse de la zone qui est partagee par tout le monde

-- finalisation: on ferme tout bien, (pthread_cancel a enlever)

-- a partir du numero de page, on obtient l'adresse de la page

-- change info permet de modifier les proprietés de la page (proprietaire, ...)

-- allocation d'une nouvelle page, on fait un mmap de type private (alloc page)

-- une fonction qui permet de lire/ecrire dedans

-- fonction qui libere la page d'adressage

-- a partir d'une adresse, recuperer le numero de la page */

#include "dsm.h"
#include "common_impl.h"
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */

/* indique l'adresse de debut de la page de numero numpage */

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
char* get_argument(char source[MSG_SIZE], int no_arg){
  char* buffer = malloc(sizeof(char)*MSG_SIZE);
  memset(buffer,'\0', MSG_SIZE);
  int check=0,compt, m=0;

  for (compt = 0; compt < (strlen(source))*sizeof(char); compt++) {

    if (source[compt] == ' ' && check != no_arg)
      check++;

    if (source[compt] == ' ' && check == no_arg){
      buffer[m]='\0';
      break;
    }

    if (source[compt] != ' ' && check == no_arg-1){
      buffer[m] = source[compt];
      m++;
    }


  }

  return buffer;
}
static char *num2address( int numpage )
{
   char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));

   if( pointer >= (char *)TOP_ADDR ){
      fprintf(stderr,"[%i] Invalid address !\n", DSM_NODE_ID);
      return NULL;
   }
   else return pointer;
}


int address2num ( char* addr){
  return (((long int)(addr-BASE_ADDR)))/(PAGE_SIZE);
}

/* fonctions pouvant etre utiles */

/* Modifier le status et proprietaire de la page */
int creer_socket_connect(char *addr,char *port){
  struct addrinfo hints,*res,*p;
  int status,sock;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; 
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  if ((status = getaddrinfo(addr, port, &hints, &res)) != 0)
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));

  for(p = res; p != NULL; p = p->ai_next) {

    //get the socket
    sock = do_socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    //connect to remote socket
    if (connect(sock, p->ai_addr, p->ai_addrlen)!=0)
      perror("connect dsmwrap");

    }

  freeaddrinfo(res);
  return sock;
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
static void dsm_change_info( int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
   if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {
	if (state != NO_CHANGE )
	table_page[numpage].status = state;
      if (owner >= 0 )
	table_page[numpage].owner = owner;
      return;
   }
   else {
	fprintf(stderr,"[%i] Invalid page number !\n", DSM_NODE_ID);
      return;
   }
}

/* Recuperer le proprietaire de la page */
static dsm_page_owner_t get_owner( int numpage)
{
   return table_page[numpage].owner;
}

/* Recuperer le status de la page */
static dsm_page_state_t get_status( int numpage)
{
   return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page( int numpage )
{
   char *page_addr = num2address( numpage );
   mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   return ;
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
   char *page_addr = num2address( numpage );
   mprotect(page_addr, PAGE_SIZE, prot);
   return;
}

/* Désallocation d'une page */
static void dsm_free_page( int numpage )
{
   char *page_addr = num2address( numpage );
   munmap(page_addr, PAGE_SIZE);
   return;
}


static void *dsm_comm_daemon( void *arg)
{
   while(1)
     {
	/* a modifier */
	printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
	sleep(2);
     }
   return;
}

/* on envoie la page */
static int dsm_send(int dest,void *buf,size_t size)
{
   /* a completer */
   
}

/* on recoit la page */
static int dsm_recv(int from,void *buf,size_t size)
{
   /* a completer */
}

/* il faut recuperer le numero de page, dire qui est le proprietaire de la page,
envoyer une socket, la recuperer */
static void dsm_handler( void* addr  )
{
   /* A modifier */
   //adam
   int page=address2num(addr);
   int owner=get_owner(page);
   dsm_protect_page(page, PROT_READ | PROT_WRITE);

   //adam
   //printf("[%i] FAULTY  ACCESS !!! \n",DSM_NODE_ID);
   //abort();
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
   /* A completer */
   /* adresse qui a provoque une erreur */
   void  *addr = info->si_addr;
  /* Si ceci ne fonctionne pas, utiliser a la place :*/
  /*
   #ifdef __x86_64__
   void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
   #elif __i386__
   void *addr = (void *)(context->uc_mcontext.cr2);
   #else
   void  addr = info->si_addr;
   #endif
   */
   /*
   pour plus tard (question ++ savoir si c'est en lecture ou ecriture qui a provoquer le seg fault):
   dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
  */
   /* adresse de la page dont fait partie l'adresse qui a provoque la faute */
   void  *page_addr  = (void *)(((unsigned long) addr) & ~(PAGE_SIZE-1));

   // if pour savoir si c'est un vrai ou un faux seg fault
   if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR))
     {
	    dsm_handler(addr);
     }
   else
     {
	/* SIGSEGV normal : ne rien faire*/
     }
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv)
{
   struct sigaction act;
   int index;
   char buffer[MSG_SIZE];
   int socket_dsm;
   int port_num=0;
   int rank;
   struct sockaddr_in client_addr;
   socklen_t len = sizeof(client_addr);
   char hostname[258];
   int numprocs;
   gethostname(hostname,258);
   memset(buffer, '\0', MSG_SIZE);
   socket_dsm = creer_socket_connect(hostname, 0);
   /* reception du nombre de processus dsm envoye */
   /* par le lanceur de programmes (DSM_NODE_NUM)*/
   read_line(socket_dsm, buffer, MSG_SIZE);
   numprocs=atoi(get_argument(buffer,1));
   /* reception de mon numero de processus dsm envoye */
   /* par le lanceur de programmes (DSM_NODE_ID)*/
   rank=atoi(get_argument(buffer,2));
   memset(buffer, '\0', MSG_SIZE);
   infos_procs *infos=NULL;
   infos=malloc(numprocs*sizeof(infos_procs));
   /* reception des informations de connexion des autres */
   /* processus envoyees par le lanceur : */
   /* nom de machine, numero de port, etc. */
   int i;
   int j;
   for(j=0;j<=numprocs;j++){
      memset(buffer, '\0', MSG_SIZE);
      read_line(socket_dsm, buffer, MSG_SIZE);
      infos[j].rang=atoi(get_argument(buffer,1));
      infos[j].port=atoi(get_argument(buffer,2));
      infos[j].hostname = get_argument(buffer,3);
   }
   
   /* initialisation des connexions */
   /* avec les autres processus : connect/accept */
   /* Allocation des pages en tourniquet */
   for(index = 0 ; index < PAGE_NUMBER ; index ++)
   {
     if ((index % DSM_NODE_NUM) == DSM_NODE_ID){
       dsm_alloc_page(index);
     }
     else{
       dsm_protect_page( index , PROT_NONE);
     }
     dsm_change_info( index, WRITE, index % DSM_NODE_NUM);
   }
   


   /* mise en place du traitant de SIGSEGV */
   act.sa_flags = SA_SIGINFO;
   act.sa_sigaction = segv_handler;
   sigaction(SIGSEGV, &act, NULL);

   /* creation du thread de communication */
   /* ce thread va attendre et traiter les requetes */
   /* des autres processus */
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);

   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}

void dsm_finalize( void )
{
   /* fermer proprement les connexions avec les autres processus */

   /* terminer correctement le thread de communication */
   /* pour le moment, on peut faire (mais ce sera a modifier): */
   pthread_cancel(comm_daemon);

  return;
}
