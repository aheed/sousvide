/*
  * Temperature controller for sous vide cooking with a Raspberry Pi
  * Anders Heed 2015
  *
  * compiling on Raspberry Pi:
  * $ gcc main.c temperature.c -o control
  *
  * usage:
  * control [target temp in degrees Celsius x1000]
  *
  * Running, example (target temperature 56.5 degrees):
  * $ sudo ./control 56500
  *
  * 
  * Based on GPIO example downloaded 20151031 from
  * http://www.susa.net/wordpress/2012/06/raspberry-pi-relay-using-gpio/
  * 
  * The example was intended as an example of using Raspberry Pi hardware registers to drive a relay using GPIO.
  * There are more conventional ways of doing this using kernel drivers.
  */

#include "temperature.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

#define TEMPTHRESHOLD 30000 //degrees Celsius x1000
#define TEMPDIFFMAX 4000 //max diff deg Celsius x1000
#define TEMPSANITYMIN 5000 //abs min temp deg Celsius x1000
#define TEMPSANITYMAX 90000 //abs max temp deg Celsius x1000

#define IOBASE   0x20000000

#define GPIO_BASE (IOBASE + 0x200000)

#define GPFSEL0    *(gpio.addr + 0)
#define GPFSEL1    *(gpio.addr + 1)
#define GPFSEL2    *(gpio.addr + 2)
#define GPFSEL3    *(gpio.addr + 3)
#define GPFSEL4    *(gpio.addr + 4)
#define GPFSEL5    *(gpio.addr + 5)
// Reserved @ word offset 6
#define GPSET0    *(gpio.addr + 7)
#define GPSET1    *(gpio.addr + 8)
// Reserved @ word offset 9
#define GPCLR0    *(gpio.addr + 10)
#define GPCLR1    *(gpio.addr + 11)
// Reserved @ word offset 12
#define GPLEV0    *(gpio.addr + 13)
#define GPLEV1    *(gpio.addr + 14)

#define BIT_17 (1 << 17)
#define BIT_22 (1 << 22)

#define PAGESIZE 4096
#define BLOCK_SIZE 4096

#define MEAS_INTERVAL_SECONDS 5

// degrees Celsius x1000 * seconds
#define INTEGRATOR_MAX 0
#define INTEGRATOR_MIN (-500000L)

#define INTEGRATOR_FACTOR 3L // 1000 / seconds

#define DERIV_FACTOR (-4)

#define TEMP_UNDEFINED (-200000)

struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

struct bcm2835_peripheral gpio = {GPIO_BASE};

// Some forward declarations...
int map_peripheral(struct bcm2835_peripheral *p);
void unmap_peripheral(struct bcm2835_peripheral *p);

int gpio_state = -1;


static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

////////////////
//  main()
////////////////
int main(int argc, char *argv[]) {

    int t1, t2, t_avg, t_thresh, loopcnt, errstate, i, t_thresh_dynamic;
    long int integrator_acc; // degrees Celsius x1000 * seconds
    long int integrator_adj; //degrees Celsius x1000
    int t_avg_old, delta_t, deriv_adj;
    char *dummy;

    signal(SIGINT, intHandler);

    if(argc == 2) {
 
       t_thresh = strtoul(argv[1], &dummy, 10);
       if(t_thresh == 0)
       {
         printf("usage:\n%s [target temp in degrees Celsius x1000]\n\n", argv[0]);
         return(0);
       }
       else
       {
         printf("Reading threshold from cmd line\n");
       }
    }
    else
    {
      //default temperature threshold
      printf("Using default threshold\n");
      t_thresh = TEMPTHRESHOLD;
    }

    printf("Threshold=%d\n", t_thresh);

    if(map_peripheral(&gpio) == -1) {
        printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
        return -1;
    }

    /* Set GPIO 22 as an output pin */
    GPFSEL2 &= ~(7 << 6); // Mask out bits 8-6 of GPFSEL2 (i.e. force to zero)
    GPFSEL2 |= (1 << 6);  // Set bits 8-6 of GPFSEL2 to binary '001'

    /* Set GPIO 17 as an output pin */
    GPFSEL1 &= ~(7 << 21); // Mask out bits 23-21 of GPFSEL1 (i.e. force to zero)
    GPFSEL1 |= (1 << 21);  // Set bits 23-21 of GPFSEL1 to binary '001'

    integrator_acc = 0;
    t_avg_old = TEMP_UNDEFINED;

    system("date");
    printf("\nt1 t2 t_avg t_thresh t_thresh_dynamic integrator_acc integrator_adj delta_t deriv_adj heat_on errstate\n");

    while(keepRunning)
    {

      t1 = get_temperature1();
      t2 = get_temperature2();
      t_avg = (t1 + t2) / 2;

      integrator_acc += (t_thresh - t_avg) * MEAS_INTERVAL_SECONDS;
      if(integrator_acc > INTEGRATOR_MAX)
      {
        integrator_acc = INTEGRATOR_MAX;
      }
      if(integrator_acc < INTEGRATOR_MIN)
      {
        integrator_acc = INTEGRATOR_MIN;
      }

      integrator_adj = (integrator_acc * INTEGRATOR_FACTOR) / 1000;

      if(t_avg_old == TEMP_UNDEFINED)
      {
        t_avg_old = t_avg;
      }
      delta_t = t_avg - t_avg_old;
      deriv_adj = delta_t * DERIV_FACTOR;
      t_avg_old = t_avg;

      t_thresh_dynamic =  t_thresh + integrator_adj + deriv_adj;

      printf("%d %d %d %d %d %7d %5d %5d %5d", t1, t2, t_avg, t_thresh, t_thresh_dynamic,
                                  integrator_acc, integrator_adj, delta_t, deriv_adj);

      gpio_state = t_avg < t_thresh_dynamic;

      errstate = 0;
      if((t1 < TEMPSANITYMIN) || (t1 > TEMPSANITYMAX))
      {
         gpio_state = 0;
         errstate = 1;
         printf("\ntemperature sanity check failed (min=%d max=%d)", TEMPSANITYMIN, TEMPSANITYMAX);
      }

      if(((t1 - t2) > 4000) || ((t2 - t1) > 4000))
      {
        gpio_state = 0;
        errstate = 2;
        printf("temp diff check failed: diff too high (max=%d)", TEMPDIFFMAX);
      }

      if(gpio_state == 0)
          GPCLR0 = BIT_17;
      else if(gpio_state == 1)
          GPSET0 = BIT_17;

      usleep(1);    // Delay to allow any change in state to be reflected in the LEVn, register bit.

//      printf(" GPIO 17 is %s\n", (GPLEV0 & BIT_17) ? "high" : "low");
      printf(" %s %d\n", (GPLEV0 & BIT_17) ? "1" : "0", errstate);

      for (loopcnt = 0; (loopcnt < MEAS_INTERVAL_SECONDS) && keepRunning; loopcnt++)
      {
        for (i = 0; (i < errstate + 1); i++)
        {
          GPSET0 = BIT_22;
          usleep(100000);
          GPCLR0 = BIT_22;
          usleep(100000);
        }

        usleep(1000000); // wait 1 sec
      }

    }

    GPCLR0 = BIT_17;
    usleep(1);    // Delay to allow any change in state to be reflected in the LEVn, register bit.
    printf("\nGPIO 17 is %s\n", (GPLEV0 & BIT_17) ? "high" : "low");

    unmap_peripheral(&gpio);

    system("date");

    // Done!
}

// Exposes the physical address defined in the passed structure using mmap on /dev/mem
int map_peripheral(struct bcm2835_peripheral *p)
{
   // Open /dev/mem
   if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("Failed to open /dev/mem, try checking permissions.\n");
      return -1;
   }

   p->map = mmap(
      NULL,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,  // File descriptor to physical memory virtual file '/dev/mem'
      p->addr_p      // Address in physical map that we want this memory block to expose
   );

   if (p->map == MAP_FAILED) {
        perror("mmap");
        return -1;
   }

   p->addr = (volatile unsigned int *)p->map;

   return 0;
}

void unmap_peripheral(struct bcm2835_peripheral *p) {

    munmap(p->map, BLOCK_SIZE);
    close(p->mem_fd);
}


