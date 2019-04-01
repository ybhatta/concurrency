#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */
#define MAX_CLASS_SEQUENCE 5 /* Maximum number of consecutive students from one class */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3

/* TODO */
/* Add your synchronization variables here */

/* Semaphore will check that at least one student from given class is waiting near the door */
sem_t sem_classa_queue;
sem_t sem_classb_queue;

/* Common mutex will prevent simultaneous  access to counters from different threads */
pthread_mutex_t mutex;

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;
static int classa_consecutive = 0;  /* Total number of consecutive students from class a */
static int classb_consecutive = 0;  /* Total number of consecutive students from class b */


typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */ 
  pthread_mutex_init(&mutex, NULL);
  sem_init(&sem_classa_queue, 0, 1);
  sem_init(&sem_classb_queue, 0, 1);
  classa_consecutive = 0;
  classb_consecutive = 0;
  
  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1) 
  {

    /* TODO */
    /* Add code here to handle the student's request.             */
    /* Currently the body of the loop is empty. There's           */
    /* no communication between professor and students, i.e. all  */
    /* students are admitted without regard of the number         */ 
    /* of available seats, which class a student is in,           */
    /* and whether the professor needs a break. You need to add   */
    /* all of this.                                               */
    
    /* Lock mutex (prevent changing from other threads)  */
    pthread_mutex_lock(&mutex);
    
    /* Check if professor needs a break */
    if (students_since_break == professor_LIMIT &&  /* students processed since break is equal to proper limit */
        students_in_office == 0)                    /* and all students at office asked their questions */
    {
      take_break() ;
    }
    /* UnLock mutex (allow other threads access the counters)  */
    pthread_mutex_unlock(&mutex);

  }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{
  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */ 
  int sem_classb_queue_value;
  
  /* Seize queue for class a students */
  sem_wait(&sem_classa_queue);
  
  /* Will hold is it posible to enter the office */
  int can_enter = 0;
  do {
    /* Lock mutex (prevent changing from other threads)  */
    pthread_mutex_lock(&mutex);
    
    /* Check if there at least one student of class b at the queue 
      sem_classb_queue_value >0, is queue is empty    */
    sem_getvalue(&sem_classb_queue, &sem_classb_queue_value);
    
    /* Check is it possible to enter the office */
    if (students_in_office < MAX_SEATS &&           /* is there free seat at the office */
        students_since_break < professor_LIMIT &&   /* professor doesn't demand for break */
        classb_inoffice == 0 &&                     /* no students from class b at the office */
        (classa_consecutive < MAX_CLASS_SEQUENCE ||  /* no more than limit of consecutive students from class a */
         sem_classb_queue_value > 0))                /* OR there are no students from class b at the queue */
      
    {
      /* access permission to enter the office */
      can_enter = 1;
        
      classa_consecutive += 1;
      classb_consecutive = 0;
      
      students_in_office += 1;
      students_since_break += 1;
      classa_inoffice += 1;
    }
    /* UnLock mutex (allow other threads access the counters)  */
    pthread_mutex_unlock(&mutex);
  } while (can_enter == 0);
  
  /* Release queue for class a students */
  sem_post(&sem_classa_queue);
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */ 
  int sem_classa_queue_value;
  
  /* Seize queue for class b students */
  sem_wait(&sem_classb_queue);
  
  /* Will hold is it posible to enter the office */
  int can_enter = 0;
  do {
    /* Lock mutex (prevent changing from other threads)  */
    pthread_mutex_lock(&mutex);
    
    /* Check if there at least one student of class a at the queue  
       sem_classa_queue_value >0, is queue is empty    */
    sem_getvalue(&sem_classa_queue, &sem_classa_queue_value);
    
    /* Check is it possible to enter the office */
    if (students_in_office < MAX_SEATS &&           /* is there free seat at the office */
        students_since_break < professor_LIMIT &&   /* professor doesn't demand for break */
        classa_inoffice == 0 &&                     /* no students from class a at the office */
        (classb_consecutive < MAX_CLASS_SEQUENCE ||  /* no more than limit of consecutive students from class b */
         sem_classa_queue_value > 0))                /* OR there are no students from class a at the queue */
      
    {
      /* access permission to enter the office */
      can_enter = 1;
        
      classa_consecutive = 0;
      classb_consecutive += 1;
      
      students_in_office += 1;
      students_since_break += 1;
      classb_inoffice += 1;
    }
    /* UnLock mutex (allow other threads access the counters)  */
    pthread_mutex_unlock(&mutex);
  } while (can_enter == 0);
  
  /* Release queue for class b students */
  sem_post(&sem_classb_queue);
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
  /* 
   *  TODO
   *  YOUR CODE HERE. 
   */
  /* Lock mutex (prevent access from other threads)  */
  pthread_mutex_lock(&mutex);
  students_in_office -= 1;
  classa_inoffice -= 1;
  /* UnLock mutex (allow other threads access the counters)  */
  pthread_mutex_unlock(&mutex);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
  /* 
   * TODO
   * YOUR CODE HERE. 
   */
  /* Lock mutex (prevent access from other threads)  */
  pthread_mutex_lock(&mutex);  
  // sem_post(&sem_students_in_office);
  students_in_office -= 1;
  classb_inoffice -= 1;
  /* UnLock mutex (allow other threads access the counters)  */
  pthread_mutex_unlock(&mutex);

}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    // student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);
  
  /* Destroy all synchronization objects */
  pthread_mutex_destroy(&mutex);
  sem_destroy(&sem_classa_queue);
  sem_destroy(&sem_classb_queue);

  printf("Office hour simulation done.\n");

  return 0;
}
