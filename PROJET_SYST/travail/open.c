#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int get_number_lign(char *txt) {

	int fd;
	fd = open(txt ,O_RDONLY);
	int i=0;
  char c;

	while (1){
		read(fd,&c, 1);
		if (c =='\n')
			i++;
		if (c =='0')
			break;
	}

  close(fd);

  return i;

}

int get_words_lign(char *txt, int number_lign, char msg[][128]){
  int j=0,k=0;
  char c;

  int fd;
	fd = open(txt ,O_RDONLY);

	while (1){

		read(fd,&c, 1);
    msg[j][k]=c;

    k++;

    if (c =='0')
			break;

		if (c=='\n'){
      msg[j][k-1]='\0';
			j++;
      k=0;
		}
	}

	close(fd);
	printf("1er: %s \n2em: %s \n3em: %s\n", msg[0], msg[1], msg[2]);

	return 0;
}


int main(int argc, char *argv[])
{
  int num_procs=get_number_lign("machine_file");
  char nom_procs[num_procs][128];
  get_words_lign("machine_file", num_procs, nom_procs);

  return 0;
}
