#include "common_impl.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MSG_SIZE 1024

// dsmwrap devra etre connu de tous. On code en dur l'endroit ou il est.

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
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
  /* on traite les fils qui se terminent */
  /* pour eviter les zombies */
}

int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {

    pid_t pid;
    int i,j,k,ret;
    int num_procs=0;
    int port_num=0;
    int fd=0;
    char exec_path[1024];
    char buffer[MSG_SIZE];
    int** pipefd_stderr;
    int** pipefd_stdout;

    /* Recuperation du chemin absolue du fichier à exécuter */
    getcwd(exec_path,1024);
    strcat(exec_path,"/");
    strcat(exec_path,argv[2]);

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

    /* creation de la socket d'ecoute */
    /* + ecoute effective */
    fd = creer_socket(SOCK_STREAM,&port_num);

    /* Initialisation de stderr et stdout */
    pipefd_stderr=malloc(sizeof(int)*num_procs);
    pipefd_stdout=malloc(sizeof(int)*num_procs);
    for(k = 0; k < num_procs ; k++) {
      pipefd_stderr[i]=malloc(2*sizeof(int));
      pipefd_stdout[i]=malloc(2*sizeof(int));
    }
    
    /* creation des fils */
    for(i = 0; i < num_procs ; i++) {

      pid = fork();

      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */

        /* redirection stdout */
        ret=dup2(pipefd_stdout[i][1],STDOUT_FILENO);
        printf("ret=%d\n",ret);

        /* redirection stderr */
        dup2(pipefd_stderr[i][1],STDERR_FILENO);
        printf("ret=%d\n",ret);

        /* Creation du tableau d'arguments pour le ssh */
        char *newargv[]={"ssh", nom_procs[i], exec_path, "no", "ui", "ok"};

        /* jump to new prog : */
        execvp("ssh",newargv);




      } else  if(pid > 0) { /* pere */
        /* fermeture des extremites des tubes non utiles */
        for(j = 0; j < num_procs ; j++){
          //close(pipefd_stdout[i][0]);
          //close(pipefd_stderr[i][0]);
        }
        num_procs_creat++;
      }
    }


    for(i = 0; i < num_procs ; i++){

      /* on accepte les connexions des processus dsm */
      struct sockaddr_in *client_addr = malloc(10*sizeof(client_addr));
      int new_sockfd = do_accept(fd, client_addr); //accept connection from client

      /*  On recupere le nom de la machine distante */
      /* 1- d'abord la taille de la chaine */
      /* 2- puis la chaine elle-meme */
      char hostname[256];
      int test = gethostname(hostname,256);
      printf("test: %d",test);

      /* On recupere le pid du processus distant  */

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
    }

    /* envoi du nombre de processus aux processus dsm*/

    /* envoi des rangs aux processus dsm */

    /* envoi des infos de connexion aux processus */

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

  /* on ferme les descripteurs proprement */

  /* on ferme la socket d'ecoute */
}
exit(EXIT_SUCCESS);
}
