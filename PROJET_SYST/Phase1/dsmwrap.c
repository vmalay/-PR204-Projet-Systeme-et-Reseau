#include "common_impl.h"
#define MSG_SIZE 1024

int main(int argc, char **argv)
{

  char buffer[MSG_SIZE];
  char hostname[128];
  int sockfd, status;
  int port_num,i;
  struct addrinfo hints, *res, *p;
  char ipstr[INET_ADDRSTRLEN];

  /* processus inter pour "nettoyer" liste arguments qu'on va passer */
  /* a la commande a executer vraiment */

  /* A SUPPRIMER: argument du ssh: 0 = ssh || 1 = hostname || 2 = dsmwrap || 3 = truc || 4.. = argument */
  char **newarg=malloc((argc-3)*sizeof(char*));
  for (i=0;i<argc-3;i++){ 
    newarg[i]=argv[i+3];
  }
  newarg[argc-3]=NULL;

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
  hints.ai_socktype = SOCK_STREAM;

  if (gethostname(hostname, 128)==0)
    printf("hostname: %s\n", hostname);

  if ((status = getaddrinfo(hostname, argv[3], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  printf("IP addresses for %s:\n\n", hostname);

  for(p = res;p != NULL; p = p->ai_next){
    void *addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    addr = &(ipv4->sin_addr);

    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("IP: %s\n", ipstr);
  }

  sockfd =  do_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0){
    perror("error connect dsmwrap");
    return 2;
  }


  /* Envoi de la taille et du nom de machine au lanceur */
  memset(buffer,'\0', MSG_SIZE);
  sprintf(buffer,"%zu",strlen(hostname));
  send_line(sockfd,buffer,strlen(buffer)+1);
  send_line(sockfd, hostname, strlen(hostname)+1);

  /* Envoi du pid au lanceur */
  memset(buffer,'\0', MSG_SIZE);
  sprintf(buffer,"%d",getpid());
  send_line(sockfd,buffer,strlen(buffer)+1);

  /* Creation de la socket d'ecoute pour les */
  /* connexions avec les autres processus dsm */

  int sockfd_dsm=creer_socket(SOCK_STREAM,&port_num);

  /* Envoi du numero de port au lanceur pour qu'il  */
  /* le propage à tous les autres processus dsm */
  memset(buffer,'\0', MSG_SIZE);
  sprintf(buffer,"%d",port_num);
  send_line(sockfd,buffer,strlen(buffer)+1);


  //freeaddrinfo(res); // free the linked list
  /* on execute la bonne commande */
  execvp(newarg[0],newarg);

  return 0;
}
