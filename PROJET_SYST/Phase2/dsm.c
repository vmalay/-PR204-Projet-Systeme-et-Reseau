
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

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */

/* indique l'adresse de debut de la page de numero numpage */
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
static void dsm_handler( void )
{
   /* A modifier */
   printf("[%i] FAULTY  ACCESS !!! \n",DSM_NODE_ID);
   abort();
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
	dsm_handler();
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
   memset(buffer, '\0', MSG_SIZE);
   read_line(argv[1], buffer, MSG_SIZE);

   /* reception du nombre de processus dsm envoye */
   /* par le lanceur de programmes (DSM_NODE_NUM)*/
   read_line(argv[1], buffer, MSG_SIZE);
   DSM_NODE_NUM = atoi(get_argument(buffer,1));

   /* reception de mon numero de processus dsm envoye */
   /* par le lanceur de programmes (DSM_NODE_ID)*/
   DSM_NODE_ID = atoi(get_argument(buffer,2));

   /* reception des informations de connexion des autres */
   /* processus envoyees par le lanceur : */
   /* nom de machine, numero de port, etc. */
   int port_num = atoi(get_argument(buffer,3));
   char* hostname = get_argument(buffer,4)

   /* initialisation des connexions */
   /* avec les autres processus : connect/accept */

   /* Allocation des pages en tourniquet */
   for(index = 0; index < PAGE_NUMBER; index ++){
     if ((index % DSM_NODE_NUM) == DSM_NODE_ID)
       dsm_alloc_page(index);
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
