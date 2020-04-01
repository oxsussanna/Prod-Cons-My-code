/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem usinFg
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	: Christos Adam Morsy AEM : 8363
*/




#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <math.h>



#define QUEUESIZE 64
#define LOOP 3000
#define   PRODUCERS  1     // Pick the number of producers and consumers
#define   CONSUMERS  13

void* sinF(void* x);
void* cosF(void* x);      //Declare my function
void* tanF(void* x);


void *producer (void *args);
void *consumer (void *args);

typedef struct{

  void* (*work)(void*);                   //Workfunction struct 
  void* arg;
   struct timeval start_t,end_t;  //struct in the workFunction-struct to calculate time
   double time;

}workFunction;

typedef struct {

  workFunction buf[QUEUESIZE];   // Queue has workFunction type elements
  long head, tail;
  int full, empty;
  pthread_mutex_t *end_pro;  // new variable that indicates the end of the producer
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;

  int num_pros;              //variable that holds how many producer are done

} queue;

queue *queueInit (void);
void queueDelete (queue *q);
//the queueAdd and queueDel take workfunction variables instead of integers//
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);


FILE *f;                            //create a file for the measurements


static void* (*random_funcs[3])(void *) = {&sinF,&cosF,&tanF}; //Declare random func

static int random_args[9] = {0,20,30,45,60,75,90,120,150};   //Declare random arguments for random functions

int main ()
{
  queue *fifo;

  pthread_t pros[PRODUCERS], cons[CONSUMERS];

  fifo = queueInit ();

  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
 
  f = fopen("apotelesmata13.csv","a");   // Save the measurements in a file

  for(int i=0; i<PRODUCERS; i++){
    pthread_create (&pros[i], NULL, producer, fifo);
  }
                                                                       //Create producers' and consumers' threads
  for(int i=0; i<CONSUMERS; i++){
    pthread_create (&cons[i], NULL, consumer, fifo);
  }

  for(int i=0; i<PRODUCERS; i++){
    pthread_join (pros[i], NULL);
  }
                                                                       //Wait producers' and consumers' threads to finish
  for(int i=0; i<CONSUMERS; i++){
    pthread_join (cons[i], NULL);
  }
  fclose(f);
  queueDelete (fifo);
  return 0;
}

void *producer (void *q)
{
  queue *fifo;
      
  int i;
  void * t; 

  srand(time(NULL));                          //Random pick of a function

  fifo = (queue *)q;
  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    
    workFunction  func;                   //create a workFunction type element to be stored to the queue

    t = &random_args[rand()%9];

    func.arg = t;

    func.work = random_funcs[rand()%3];

    gettimeofday(&func.start_t,NULL);
    queueAdd (fifo, func);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);

  }
  pthread_mutex_lock (fifo->end_pro);
  fifo->num_pros++;                                //Check and note when a producer is done
  pthread_mutex_unlock (fifo->end_pro);
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction d;
  fifo = (queue *)q;

  while(1){
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      if(fifo->num_pros == PRODUCERS){
        fclose(f);
       }
       pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &d);
    gettimeofday(&d.end_t,NULL);
    d.time = (double)((d.end_t.tv_usec-d.start_t.tv_usec)/1.0e6+d.end_t.tv_sec-d.start_t.tv_sec);
    fprintf(f,"%f \n",d.time);                  // Save the requested time in the file
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    (*d.work)(d.arg);
  }
  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->num_pros = 0;
  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->end_pro = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->end_pro, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_mutex_destroy (q->end_pro);
  free (q->end_pro);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}


void queueAdd (queue *q, workFunction in)     //insert workFunction type in the queue
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)     //Get workFunction type from the queue
{
  *out = q->buf[q->head];
  q->head++;                              
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
  
  
  #define PI 3.14159265

void* sinF(void* arg){        //Calculate sine of an angle
  int x = *(int *)arg;
  double a =sin((PI*(x))/180); 

  printf("The sine of the %d degrees is %.3f\n",x,a);
  return (NULL);
}

void* cosF(void* arg){      //Calculate cosine of an angle
  int x = *(int *)arg;
  double a =cos((PI*(x))/180); 

  printf("The cosine of the %d degrees is %.3f\n",x,a);
  return (NULL);
}

void* tanF(void* arg){      //Calculate tangent of an angle
  int x = *(int *)arg;
  double a =tan((PI*(x))/180); 
    if (x==90){
      printf("90 degrees tangent cannot be defined\n");
     return (0);
    }
  printf("The tangent of the %d degrees is %.3f\n",x,a);
  return (NULL);
}

