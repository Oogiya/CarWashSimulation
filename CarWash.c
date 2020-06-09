//Miyu Levin 207226556

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
// Values are considered as seconds
/*
#define WASH_STANDS 2
#define AVG_TIME_BETWEEN_ARRIVAL 1.5
#define AVG_TIME_WASH 3.5
#define RUN_TIME 20
*/

int WASH_STANDS = 5;
float AVG_TIME_BETWEEN_ARRIVAL = 1.5;
float AVG_TIME_WASH = 3;
float RUN_TIME = 20;

float timeCounter = 0;

typedef struct node {
  int val;
  struct node *next;
} node_t;

int pop(node_t **head) {
  int retval = -1;
  node_t *next_node = NULL;

  if (*head == NULL) {
    return 1;
  }

  next_node = (*head)->next;
  retval = (*head)->val;
  free(*head);
  *head = next_node;

  return retval;
}

void print_list(node_t * head) {
  node_t * current = head;

  while (current != NULL) {
    printf("%d\n", current->val);
    current = current->next;
  }
}


node_t *init(int value) {
  node_t * head = NULL;
  head = (node_t *) malloc(sizeof(node_t));

  head->val = value;
  head->next = NULL;

  return head;
}

int getLast(node_t *head) {
  return head->val;
}

void push(node_t *head, int val) {
  node_t *current = head;
  while (current->next != NULL) {
    current = current->next;
  }
  current->next = (node_t *) malloc(sizeof(node_t));
  current->next->val = val;
  current->next->next = NULL;
}


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

node_t *carList = NULL;

int EXIT_FLAG = 0;

float nextTime(float rateParameter) {
  float result = -logf(1.0 - (float)rand() / (RAND_MAX + 1.0)) / rateParameter;
  return result;
}

void *runTimeThread(void *ptr) {
  while (timeCounter < RUN_TIME && EXIT_FLAG == 0) {
    sleep(1);
    timeCounter++;
  }

  pthread_mutex_lock(&mutex);
  EXIT_FLAG = 1;
  pthread_mutex_unlock(&mutex);

  pthread_exit(NULL);
}



char* getTotalCarsWashed() {
  int shmid;
  key_t key1;
  char *totalCarsWashed;
  key1 = 0101;

  //int slots = -1;

  if ((shmid = shmget(key1, 27, 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  if ((totalCarsWashed = shmat(shmid, NULL, 0)) == (char *)-1) {
    perror("shmat");
    exit(1);
  }

  return totalCarsWashed;
}

char* getAvailableWashingSlots() {

  int shmid;
  key_t key1;
  char *sharedData;
  key1 = 1212;

  //int slots = -1;

  if ((shmid = shmget(key1, 27, 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  if ((sharedData = shmat(shmid, NULL, 0)) == (char *)-1) {
    perror("shmat");
    exit(1);
  }

  return sharedData;
}

void *cleanCarsThread(void *ptr) {
  sleep(nextTime(1/AVG_TIME_WASH));
  pthread_exit(NULL);

}

void runCarProcess() {
  // car checks if any available slots
  pthread_mutex_lock(&mutex);
  char *slotsData = getAvailableWashingSlots();
  pthread_mutex_unlock(&mutex);
  int slots = strtol(slotsData, NULL, 10);
  if (slots > 0) {
    slots--;

    pthread_mutex_lock(&mutex);
    sprintf(slotsData, "%d", slots);
    pthread_mutex_unlock(&mutex);

    printf("%d entered the car wash\n", getpid());

    pthread_t washCar;
    pthread_create(&washCar, NULL, cleanCarsThread, NULL);

    pthread_join(washCar, NULL);
    printf("%d left the car wash\n", getpid());
    slots++;

    pthread_mutex_lock(&mutex);
    sprintf(slotsData, "%d", slots);
    pthread_mutex_unlock(&mutex);

  }
}

void *runCarsThread(void *ptr) {
  while(EXIT_FLAG == 0) {

    if (fork() == 0) {

      int shmid;
      key_t key1 = 2323;
      char *sharedData;

      // car registers in the list
      if ((shmid = shmget(key1, 27, 0666)) < 0) {
        perror("shmget");
        exit(1);
      }

      if ((sharedData = shmat(shmid, NULL, 0)) == (char *)-1) {
        perror("shmat");
        exit(1);
      }
      pthread_mutex_lock(&mutex);
      sprintf(sharedData, "%d", getpid());
      pthread_mutex_unlock(&mutex);

      runCarProcess();

      return 0;
    }

    sleep(nextTime(1/AVG_TIME_BETWEEN_ARRIVAL));
  }
  pthread_exit(NULL);
}



int main(int argc, char* argv[])
{
  printf("Please enter desired car wash runtime:\n");
  scanf("%f",&RUN_TIME);
  printf("Please enter desired average time between car arrival: \n");
  scanf("%f",&AVG_TIME_BETWEEN_ARRIVAL);
  printf("Please enter desired average car wash time: \n");
  scanf("%f", &AVG_TIME_WASH);
  printf("Please enter desired washing stands: \n");
  scanf("%d", &WASH_STANDS);

  int c;
  while ((c = getchar()) != '\n' && c != EOF) { }

  printf("Running for: %.2f seconds [%.2f] - [%.2f]\n", RUN_TIME, AVG_TIME_BETWEEN_ARRIVAL, AVG_TIME_WASH);

  int parentPid = getppid();

  char *sharedData;
  char *availableWashingSlots;
  char *totalCarsWashed;
  int shmid, shmid2, shmid3;
  key_t key, key2, key3;
  key = 2323;
  key2 = 1212;
  key3 = 0101;

  int totalCars = 0;

  if ((shmid = shmget(key, 27, IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    return 1;
  }

  if ((sharedData = shmat(shmid, NULL, 0)) == (char *) - 1) {
    perror("shmat");
    return 2;
  }

  if ((shmid2 = shmget(key2, 27, IPC_CREAT | 0666)) < 0) {
    perror("shmget2");
    return 3;
  }
  if ((availableWashingSlots = shmat(shmid2, NULL, 0)) == (char *) - 1) {
    perror("shmat2");
    return 4;
  }

  if ((shmid3 = shmget(key3, 27, IPC_CREAT | 0666)) < 0) {
    perror("shmget3");
    return 5;
  }
  if ((totalCarsWashed = shmat(shmid3, NULL, 0)) == (char *) - 1) {
    perror("shmat3");
    return 6;
  }

  sprintf(availableWashingSlots, "%d", WASH_STANDS);

  printf("%s\n", availableWashingSlots);

  strcpy(sharedData, "waiting");

  pthread_t runTime;
  pthread_t runCars;
  pthread_create(&runTime, NULL, runTimeThread, NULL);
  pthread_create(&runCars, NULL, runCarsThread, NULL);

  int processNum = 0;
  int slots = 0;

  char exit;

  while (EXIT_FLAG == 0) {

    if (strcmp(sharedData, "waiting") != 0) {
      totalCars++;
      processNum = strtol(sharedData, NULL, 10);
      slots = strtol(availableWashingSlots, NULL, 10);

      if (slots <= 0) {
        printf("[TIMER: %.2f/%.2f] No washing slots available adding %d to the queue\n", timeCounter, RUN_TIME, processNum);
        if (carList == NULL) carList = init(processNum);
        else {
          push(carList, processNum);
        }
      }

      strcpy(sharedData, "waiting");
    }
    //exit = getchar();
    //if (exit == '\n') EXIT_FLAG = 1;
    sleep(1);
  }

  //Stop running carwash
  pthread_join(runTime, NULL);


  if (carList != NULL) {

    while(carList != NULL) {
      processNum = pop(&carList);
      if (getpid() == processNum) {
        runCarProcess();
      }
    }
  }

  while(wait(NULL) > 0);

  printf("Total cars washed: %d\n", totalCars);
  shmdt(sharedData);
  shmctl(shmid, IPC_RMID, 0);
  return 0;

}
