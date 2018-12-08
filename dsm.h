#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>



/* fin des includes */

#define TOP_ADDR    (0x40000000) // NE PAS CHANGER
#define PAGE_NUMBER (100) // on peut changer pour faire notre tambouille
#define PAGE_SIZE   (sysconf(_SC_PAGE_SIZE))
#define BASE_ADDR   (TOP_ADDR - (PAGE_NUMBER * PAGE_SIZE))

typedef enum
{
   NO_ACCESS,
   READ_ACCESS,
   WRITE_ACCESS,
   UNKNOWN_ACCESS
} dsm_page_access_t;

typedef enum
{
   INVALID,
   READ_ONLY,
   WRITE,
   NO_CHANGE
} dsm_page_state_t;

typedef int dsm_page_owner_t;

typedef struct
{
   int rang;
   char *hostname;
   int socket;
   int port;
}infos_procs;

typedef struct
{
   dsm_page_state_t status;
   dsm_page_owner_t owner;
} dsm_page_info_t;

dsm_page_info_t table_page[PAGE_NUMBER]; //information pour chaque page

pthread_t comm_daemon;
extern int DSM_NODE_ID; // identifier le rang d'un processus
extern int DSM_NODE_NUM; // identifier le nombre max de processus

char *dsm_init( int argc, char **argv);
void  dsm_finalize( void );
