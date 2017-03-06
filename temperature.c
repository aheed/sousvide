/*
  * Temperature controller for sous vide cooking with a Raspberry Pi
  * Anders Heed 2015
  *
  * Implementation of temperature reading functions for DS18B20
  * temp sensors using 1-wire protocol. The serial numbers of
  * sensors are currently hard coded.
  * 
*/

#include "temperature.h"
#include <stdio.h>

#define TEMPSENSORFILE1 "/sys/bus/w1/devices/28-0000065edab6/w1_slave"
#define TEMPSENSORFILE2 "/sys/bus/w1/devices/28-0000065fcd57/w1_slave"
#define TEMPSENSORFILELEN 74 //expected file size

static int get_temperature(char *filename)
{

  FILE *fp;
  int n;
  int readtemp;
  char buffer[TEMPSENSORFILELEN + 1];
  char *p = &buffer[TEMPSENSORFILELEN - 5];
  char *dummy;
  
  fp = fopen(filename, "r");

  if(fp != NULL)
  {
    n = fread(buffer, TEMPSENSORFILELEN, 1, fp);
    buffer[TEMPSENSORFILELEN] = '\0';
    fclose(fp);
  }
  else
  {
    n = 0;
  }

  if(n == 1)
  {
    //success
    readtemp = strtoul(p, &dummy, 10);
  }
  else
  {
    readtemp = -100;
  }

  return readtemp;
}


int get_temperature1(void)
{
  return get_temperature(TEMPSENSORFILE1);
}

int get_temperature2(void)
{
  return get_temperature(TEMPSENSORFILE2);
}

