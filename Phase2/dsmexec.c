#include "common_impl.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

// envoie de la page (numero de page ou page en entier)

// erreur de segmentation, protect sur la page ou changer ?

// envoyer un tableau d'entier

// accept et connect


// dsmwrap devra etre connu de tous. On code en dur l'endroit ou il est.
#define PATH_DSMWRAP "~/T2/PROJET_SYSTEM/PR204_MOI/Phase2/bin/dsmwrap"
#define PATH_TRUC "~/T2/PROJET_SYSTEM/PR204_MOI/Phase2/bin/truc"

/* variables globales */
#define MSG_SIZE 1024
/* un tableau gerant les infos d'identification des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
  pid_t pid;

  while ((pid = waitpid(-1, NULL, WNOHANG)) != -1)
  {
    if (pid > 0)
    {
      num_procs_creat--;
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {

    // Variables
    int i,j,k;
    char buffer[MSG_SIZE];

    // DSM Information Variables
    pid_t pid;
    int port_num=0;
    int num_procs=0;
    int sockfd=0;
    char port[10];
    char hostname[258];
    char rank[10];

    // PIPE redirrection Variables
    int** pipefd_stderr;
    int** pipefd_stdout;

    //Poll Structure Variables
    int timeout=-1;
    struct pollfd fds_stdout[200];
    struct pollfd fds_stderr[200];
    memset(fds_stdout, 0, sizeof(fds_stdout)*num_procs_creat);
    memset(fds_stderr, 0, sizeof(fds_stderr)*num_procs_creat);

    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &sigchld_handler;
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);

    /* lecture du fichier de machines */

    /* 1- on recupere le nombre de processus a lancer */
    num_procs=get_number_lign(argv[1]);

    /* 2- on recupere les noms des machines :  */
    char nom_procs[num_procs][NAME_MAX];
    get_words_lign(argv[1], num_procs, nom_procs);

    /* la machine est un des elements d'identification */

    /* creation de la socket d'ecoute + ecoute effective */
    sockfd = creer_socket(SOCK_STREAM,&port_num);
    do_listen(sockfd);

    /* Initialisation de pipe stderr et stdout */
    pipefd_stderr=malloc(sizeof(int)*num_procs);
    pipefd_stdout=malloc(sizeof(int)*num_procs);

    for(k = 0; k < num_procs ; k++) {
      pipefd_stderr[k]=malloc(2*sizeof(int));
      pipefd_stdout[k]=malloc(2*sizeof(int));
    }

    /* creation des fils */
    for(i = 0; i < num_procs ; i++) {

      pid = fork();

      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */

        // /* redirection stdout */
      if (dup2(pipefd_stdout[i][1],STDOUT_FILENO) == -1)
        perror("dup2 stdout");

        // /* redirection stderr */
      if (dup2(pipefd_stderr[i][1],STDERR_FILENO) == -1)
        perror("dup2 stderr");

        /* Creation du tableau d'arguments pour le ssh */
      char ** newargv = malloc((argc+4)*sizeof(char *));
      gethostname(hostname,258);
      sprintf(port,"%i",port_num);
      sprintf(rank,"%d", i);
      newargv[0]="ssh";
      newargv[1]=nom_procs[i];
      newargv[2]=PATH_DSMWRAP;
      newargv[3]=hostname;
      newargv[4]=port;
      newargv[5]=rank;
      newargv[6]=PATH_TRUC;
      for (k = 7; k < argc + 3; k++)
        newargv[k]=argv[k-3];
      newargv[argc+3] = NULL;

        /* jump to new prog : */
      fprintf(stdout,"\n*** ___EXECUTION_SSH___ ***\n");
      execvp("ssh",newargv);

      } else  if(pid > 0) { /* pere */
        /* fermeture des extremites des tubes non utiles */
      for(j = 0; j < num_procs ; j++){
          close(pipefd_stdout[j][1]);
          close(pipefd_stderr[j][1]);
      }
      num_procs_creat++;
    }
  }


  proc_array=malloc(num_procs*sizeof(dsm_proc_t));

  sleep(2);


  for(k = 0; k < num_procs ; k++){

    int pid,rank,size_hostname,port_distant;
    /* on accepte les connexions des processus dsm */
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    proc_array[k].connect_info.new_sockfd = accept(sockfd,(struct sockaddr*) &client_addr, &len );
    fprintf(stdout,"\n*** ___ACCEPT_PROCESSUS___ ***\n");
    /*  On recupere le nom de la machine distante */

    /* 1- d'abord la taille de la chaine */
    read(proc_array[k].connect_info.new_sockfd,&size_hostname,sizeof(int));

    /* 2- puis la chaine elle-meme */
    memset(buffer, '\0', MSG_SIZE);
    read(proc_array[k].connect_info.new_sockfd, buffer, size_hostname+1);
    proc_array[k].connect_info.hostname=(char*)malloc(size_hostname*sizeof(char));
    strcpy(proc_array[k].connect_info.hostname,buffer);

    /* On recupere le pid du processus distant  */
    read(proc_array[k].connect_info.new_sockfd, &pid, sizeof(int));
    proc_array[k].pid = pid;

    /* On recupere le n°port de la sock ecoute des processus distants */
    read(proc_array[k].connect_info.new_sockfd, &port_distant, sizeof(int));
    proc_array[k].connect_info.port_distant = port_distant;

    /* On recupere le rank des processus distant */
    read(proc_array[k].connect_info.new_sockfd, &rank, sizeof(int));
    proc_array[k].connect_info.rank= rank;

    printf("proc_array[%d] a reçu: %d, %s, %d, %d, %d\n",k,size_hostname,
      proc_array[k].connect_info.hostname, proc_array[k].pid, proc_array[k].connect_info.port_distant,
      proc_array[k].connect_info.rank);
  } // end of FOR

  printf("~~~~~~~~~~~~SORTIE BOUCLE1~~~~~~~~~~~~\n");
 fflush(stdout);
 fflush(stderr);
  for(k = 0; k < num_procs ; k++){
    /* envoi du nombre de processus aux processus dsm*/
    //send_line(proc_array[k].connect_info.new_sockfd, &num_procs_creat, sizeof(int));
    /* envoi des ranks aux processus dsm */
    //send_line(proc_array[k].connect_info.new_sockfd, &proc_array.connect_info.rank,sizeof(int)*num_procs_creat);
    /* envoi des infos de connexion aux processus */
    //send_line(proc_array[k].connect_info.new_sockfd, &proc_array.connect_info,sizeof(proc_array.connect_info)*num_procs_creat);
  }
  printf("~~~~~~~~~~~~SORTIE BOUCLE2~~~~~~~~~~~~ \n");
  fflush(stdout);
  fflush(stderr);

  // nfds = num_procs_creat;
  for (i=0; i< num_procs_creat; i++){
    fds_stdout[i].fd = pipefd_stdout[i][0];
    fds_stdout[i].events = POLLIN;
    fds_stderr[i].fd = pipefd_stderr[i][0];
    fds_stderr[i].events = POLLIN;
  }

  /* gestion des E/S : on recupere les caracteres */
  /* sur les tubes de redirection de stdout/stderr */
  while(1){
      /*je recupere les infos sur les tubes de redirection
      jusqu'à ce qu'ils soient inactifs (ie fermes par les
      processus dsm ecrivains de l'autre cote ...) */
    int nfds_stdout = create_poll(fds_stdout,num_procs_creat,num_procs_creat,timeout,proc_array,pipefd_stdout,"stdout");
    int nfds_stderr = create_poll(fds_stderr,num_procs_creat,num_procs_creat,timeout,proc_array,pipefd_stderr,"stderr");
    if (nfds_stdout == 0 && nfds_stderr ==0)
      break;

  }; // end of WHILE

  printf("~~~~~~~~~~~~SORTIE BOUCLE3~~~~~~~~~~~~\n");

  /* on attend les processus fils */
  for (k = 0; k < num_procs; k++)
    wait(NULL);

  /* on ferme les descripteurs proprement */
  for (k = 0; k < num_procs; k++)
    close(proc_array[i].connect_info.new_sockfd);

  printf("~~~~SORTIE BOUCLE4~~~~\n");
  fflush(stdout);
  fflush(stderr);
  /* on ferme la socket d'ecoute */
  close(sockfd);
}
exit(EXIT_SUCCESS);
}
