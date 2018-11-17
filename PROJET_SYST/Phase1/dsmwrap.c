#include "common_impl.h"
#define MSG_SIZE 1024

int main(int argc, char **argv)
{

  char buffer[MSG_SIZE];
  char* hostname = argv[1];
  int sockfd;
  int port_num,i;
  struct addrinfo hints, *info;
  char host_ip[INET_ADDRSTRLEN];

  /* processus inter pour "nettoyer" liste arguments qu'on va passer */
  /* a la commande a executer vraiment */

  /* A SUPPRIMER: argument du ssh: 0 = ssh || 1 = hostname || 2 = dsmwrap || 3 = truc || 4.. = argument */
  char **newarg=malloc(sizeof(char*)*(argc-3));
  for (i=0;i<argc-3;i++){ 
    newarg[i]=argv[i+3];
  }
  newarg[argc-3]=NULL;

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;

  getaddrinfo(argv[1], argv[2], &hints, &info);
  inet_ntop(info->ai_family, info->ai_addr, host_ip, sizeof host_ip);

  sockfd =  do_socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  connect(sockfd, info->ai_addr, info->ai_addrlen);


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

  /* on execute la bonne commande */
  execvp(newarg[0],newarg);

  return 0;
}
