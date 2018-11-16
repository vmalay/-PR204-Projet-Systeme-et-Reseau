#include "common_impl.h"
#define MSG_SIZE 1024

int main(int argc, char **argv)
{

  char buffer[MSG_SIZE];
  int port = (atoi(argv[2]));
  char* hostname = argv[1];
  int sockfd;
  int port_num,i;
  struct sockaddr_in sock_host;

  /* processus inter pour "nettoyer" liste arguments qu'on va passer */
  /* a la commande a executer vraiment */

  char **newarg=malloc(sizeof(char*)*(argc-4));
  for (i=0;i<argc;i++){
    newarg[i]=argv[i+4];
  }
  newarg[argc-4]=NULL;

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */

  // hostname = gethostbyname // getaddrinfo et getservername
  memset(& sock_host, '\0', sizeof(sock_host));
  sock_host.sin_family = AF_INET;
  sock_host.sin_port = htons(port);
  inet_aton(hostname, & sock_host.sin_addr);

  sockfd =  do_socket(AF_INET, SOCK_STREAM, 0);
  do_connect(sockfd, sock_host, sizeof(sock_host));


  /* Envoi du nom de machine au lanceur */
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

  /* on execute la bonne commande */
  execvp(newarg[0],newarg);
  return 0;
}
