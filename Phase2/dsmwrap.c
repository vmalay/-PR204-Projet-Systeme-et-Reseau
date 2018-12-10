#include "common_impl.h"
#define MSG_SIZE 1024

int main(int argc, char **argv)
{

  int sockfd,i, port_num;
  char buffer[MSG_SIZE];
  char* hostname = argv[1];
  char* port = argv[2];
  char* rank = argv[3];
  char proc_name[258];
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(client_addr);
  gethostname(proc_name,258);

  fprintf(stderr,"\n*** ___EXECUTION_DMSWRAP___ ***\n");

  int x;
  for(x=0;x<argc;x++){
    fprintf(stderr,"arg %d: %s.\n",x,argv[x]);
  }

  /* processus inter pour "nettoyer" liste arguments qu'on va passer */
  /* a la commande a executer vraiment */

  char **newarg=malloc((argc-4)*sizeof(char*));
  for (i=0;i<argc-4;i++){
    newarg[i]=argv[i+4];
    fprintf(stderr,"newarg i: %s\n", newarg[i]);
  }
  newarg[argc-4]=NULL;


  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */
  sockfd = creer_socket_connect(hostname, port);

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
  int sock = creer_socket(SOCK_STREAM,&port_num);
  do_listen(sock);
  int sockdsm = accept(sock,(struct sockaddr*) &client_addr, &len );

  /* Envoi du numero de port au lanceur pour qu'il  */
  /* le propage à tous les autres processus dsm */
  memset(buffer,'\0', MSG_SIZE);
  sprintf(buffer,"%d",port_num);
  send_line(sockfd,buffer,strlen(buffer)+1);

  /* Envoi du rank */
  memset(buffer,'\0', MSG_SIZE);
  sprintf(buffer,"%s",rank);
  send_line(sockfd,buffer,strlen(buffer)+1);


  /* on execute la bonne commande */
  fprintf(stderr,"*** ___FIN_DSMWRAP___ ***\n\n");
  fflush(stdout);
  fflush(stderr);
  execvp(newarg[0],newarg);

  return 0;
}
