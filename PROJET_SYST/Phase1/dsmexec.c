#include "common_impl.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


// dsmwrap devra etre connu de tous. On code en dur l'endroit ou il est.
#define PATH_DSMWRAP "/net/t/vmalay/T2/PROJET_SYST/Phase1/bin/dsmwrap"

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

    pid_t pid;
    int port_num=0;
    int i,j,k;
    int num_procs=0;

    int fd=0;
    char buffer[MSG_SIZE];
    char port[10];
    int** pipefd_stderr;
    int** pipefd_stdout;

    /* Recuperation du chemin absolue du fichier à exécuter */
    char exec_path[MSG_SIZE];
    sprintf(exec_path,"/net/t/vmalay/T2/PROJET_SYST/Phase1/bin/%s",argv[2]);


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
    fd = creer_socket(SOCK_STREAM,&port_num);

    /* Initialisation de stderr et stdout */
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


        /* redirection stdout */
        if (dup2(pipefd_stdout[i][1],STDOUT_FILENO) == -1)
        perror("dup2 stdout");

        /* redirection stderr */
        if (dup2(pipefd_stderr[i][1],STDERR_FILENO) == -1)
        perror("dup2 stderr");

        /* Creation du tableau d'arguments pour le ssh */
        char ** newargv = malloc((argc+4)*sizeof(char *));
        newargv[0]="ssh";
        newargv[1]=nom_procs[i];
        newargv[2]=PATH_DSMWRAP;
        sprintf(port,"%d",ntohs(port_num));
        newargv[3]=port;
        newargv[4]=exec_path;
        for (k = 5; k < argc + 3; k++)
        	newargv[k]=argv[k-3];
        newargv[argc+3] = NULL;

        /* jump to new prog : */
        execvp("ssh",newargv);


      } else  if(pid > 0) { /* pere */
        /* fermeture des extremites des tubes non utiles */
        for(j = 0; j < num_procs ; j++){
          //close(pipefd_stdout[j][0]);
          //close(pipefd_stderr[j][0]);
        }
        num_procs_creat++;
      }
    }

    struct processus_info{
      pid_t pid_distant;
      int port_distant;
      int new_sockfd;
      int size_hostname;
      char *hostname;
    };

    struct processus_info proc[num_procs];

    for(k = 0; k < num_procs ; k++){

      /* on accepte les connexions des processus dsm */
      struct sockaddr_in client_addr;
      socklen_t len = sizeof(client_addr);
      proc[k].new_sockfd = accept(fd,(struct sockaddr*) &client_addr, &len ); 

      /*  On recupere le nom de la machine distante */

      /* 1- d'abord la taille de la chaine */
      read_line(proc[k].new_sockfd, buffer, MSG_SIZE);
      proc[k].size_hostname = atoi(buffer);

      /* 2- puis la chaine elle-meme */
      read_line(proc[k].new_sockfd, buffer, proc[k].size_hostname+1);
      proc[k].hostname=(char*)malloc(proc[k].size_hostname*sizeof(char));
      strcpy(proc[k].hostname,buffer);

      /* On recupere le pid du processus distant  */
      read_line(proc[k].new_sockfd, buffer, MSG_SIZE);
      proc[k].pid_distant = atoi(buffer);

      /* On recupere le n°port de la sock ecoute des processus distants */
      read_line(proc[k].new_sockfd, buffer, MSG_SIZE);
      proc[k].port_distant = atoi(buffer);
    }

      /* envoi du nombre de processus aux processus dsm*/
    for(k = 0; k < num_procs ; k++){
      memset(buffer, '\0', MSG_SIZE);
      sprintf(buffer, "%d", num_procs);
      send_line(proc[k].new_sockfd, buffer, strlen(buffer)+1);
    }
      /* envoi des rangs aux processus dsm */
    for(k = 0; k < num_procs ; k++){    
      memset(buffer, '\0', MSG_SIZE);
      sprintf(buffer, "%d", k);
      send_line(proc[k].new_sockfd, buffer, strlen(buffer)+1);
	}
      /* envoi des infos de connexion aux processus */
    for(k = 0; k < num_procs ; k++){
      memset(buffer, '\0', MSG_SIZE);
      sprintf(buffer, "%d %s", proc[k].port_distant, proc[k].hostname);
      send_line(proc[k].new_sockfd, buffer, strlen(buffer)+1);
    }

    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */
    /* while(1)
    {
    je recupere les infos sur les tubes de redirection
    jusqu'à ce qu'ils soient inactifs (ie fermes par les
    processus dsm ecrivains de l'autre cote ...)
  };
  */

  /* on attend les processus fils */
    for (k = 0; k < num_procs; k++)
      wait(NULL);   

  /* on ferme les descripteurs proprement */

  /* on ferme la socket d'ecoute */
  close(fd);
}
exit(EXIT_SUCCESS);
}
