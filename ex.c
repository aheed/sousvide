
//#define TEMPSENSORFILE1 "/home/pi/src/sousvide/tempsensfile.txt"
#define TEMPSENSORFILE1 "/sys/bus/w1/devices/28-0000065edab6/w1_slave"
#define TEMPSENSORFILE2 "/sys/bus/w1/devices/28-0000065fcd57/w1_slave"
#define TEMPSENSORFILELEN 74 //expected file size

#include <stdio.h>

int get_temperature(char *filename)
{

  FILE *fp;
  int n;
  int readtemp;
  char buffer[TEMPSENSORFILELEN + 1];
  char *p = &buffer[TEMPSENSORFILELEN - 5];
  char *dummy;
  
  fp = fopen(filename, "r");

  n = fread(buffer, TEMPSENSORFILELEN, 1, fp);
  buffer[TEMPSENSORFILELEN] = '\0';

  fclose(fp);

  readtemp = strtoul(p, &dummy, 10);
  return readtemp;
}

int main(int argc, char *argv[]) {

  /*FILE *fp;
  int n;*/
  int readtemp;
/*  char buffer[TEMPSENSORFILELEN + 1];
  char *p = &buffer[TEMPSENSORFILELEN - 5];
  char *dummy;
  
  fp = fopen(TEMPSENSORFILE1,"r");

  n = fread(buffer, TEMPSENSORFILELEN, 1, fp);
  buffer[TEMPSENSORFILELEN] = '\0';

  fclose(fp);

  printf("%s %d\n", buffer, n);
  printf("%sx\n", p, n);
  printf("apa\n");

  readtemp = strtoul(p, &dummy, 10);*/

  readtemp = get_temperature(TEMPSENSORFILE1);
  printf("%d\n", readtemp);
  readtemp = get_temperature(TEMPSENSORFILE2);
  printf("%d\n", readtemp);

}

