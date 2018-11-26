#include "common_impl.h"
#define MSG_SIZE 1024

int main(int argc, char **argv)
{

  int sockfd,i, port_num;
  char message[MSG_SIZE];
  char* hostname = argv[1];
  char* port = argv[2];
  char* rang = argv[3];
  char proc_name[258];
  gethostname(proc_name,258);

  fprintf(stderr,"\n*** ___EXECUTION_DMSWRAP___ ***\n");

  int x;
  for(x=0;x<argc;x++){
    fprintf(stderr,"arg %d: %s.\n",x,argv[x]);
  }

  /* processus inter pour "nettoyer" liste arguments qu'on va passer */
  /* a la commande a executer vraiment */

  char **newarg=malloc((argc-3)*sizeof(char*));
  for (i=0;i<argc-3;i++){
    newarg[i]=argv[i+3];
  }
  newarg[argc-3]=NULL;

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */
  sockfd = creer_socket_connect(hostname, port);

  /* Creation de la socket d'ecoute pour les */
  /* connexions avec les autres processus dsm */
  creer_socket(SOCK_STREAM,&port_num);

  /* Envoi de la taille et du nom de machine au lanceur */
  /* Envoi du pid au lanceur */
  /* Envoi du numero de port au lanceur */
  sprintf(message,"%ld %s %d %d %s", strlen(proc_name), proc_name, getpid(), port_num, rang);
  send_line(sockfd, message, strlen(message)+1);

  /* on execute la bonne commande */

  fprintf(stderr,"*** ___FIN_DSMWRAP___ ***\n\n");
  execvp(newarg[0],newarg);

  return 0;
}
