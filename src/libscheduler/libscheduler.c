/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
  int id;
  int arrivalTime;
  int endTime;
  int coreAssigned;
  int duration;

  int scheduleLatency;
  int timeRun;

  int priority;

  int finished;
} job_t;

priqueue_t* queue;
scheme_t activeScheme;
int coreCount;
int* coreArr;
//all comparators should return 1 for A is higher priority
//-1 for B is higher priority
int compareFCFS(job_t* a, job_t* b){
  if(a->arrivalTime<b->arrivalTime){
    return 1;
  }
  return -1;
}

int compareSJF(job_t* a, job_t* b){
  if (a->duration == b->duration){
    if(a->arrivalTime<b->arrivalTime) return 1; //maybe replace this with a call to compareFCFS? not sure if that works with func pointers
    return -1;
  }
  if(a->duration<b->duration){
    return 1;
  }
  return -1;
}

int comparePSJF(job_t* a, job_t* b){
  int aRemaining = a->duration-a->timeRun;
  int bRemaining = a->duration-b->timeRun;
  if(aRemaining == bRemaining){
    if(a->arrivalTime<b->arrivalTime) return 1;
    return -1;
  }
  if(aRemaining<bRemaining) return 1;
  return -1;
}

int comparePRI(job_t* a, job_t* b){
  if(a->priority == b->priority){
    if(a->arrivalTime<b->arrivalTime) return 1;
    return -1;
  }
  if(a->priority>b->priority) return 1;
  return -1;
}

int comparePPRI(job_t* a, job_t* b){
  if(a->priority == b->priority){
    if(a->arrivalTime<b->arrivalTime) return 1;
    return -1;
  }
  if(a->priority>b->priority) return 1;
  return -1;
}

int compareRR(job_t* a, job_t* b){
  return 1; //always push to back of queue
}




void updateTimes(int time){
  for(int i =0;i<priqueue_size(queue);i++){
    job_t* job = priqueue_at(queue, i);
    if(job->coreAssigned != -1 || job->endTime == time){
      job->timeRun = time-job->arrivalTime; 
    }
    if(job->scheduleLatency==-1 && job->coreAssigned!=-1){ //if job is scheduled but its schedule latency hasnt been calculated yet, calculate it
      job->scheduleLatency = time-job->arrivalTime;
    }
  }
}

//pull from the priority queue until the cores are full
void fillIdleCores(){
  job_t* curr;
  int found = 0;
  if(priqueue_size(queue)>0){
    int nextCore = -1;
    for(int i=0;i<coreCount;i++){
      if(coreArr[i] == -1){
        nextCore = i;
        break;
      }
    }
    while(nextCore!=-1){
      found = 0;
      for(int i=0;i<priqueue_size(queue);i++){
        curr = (job_t*)priqueue_at(queue, i);
        if(!curr->finished&&curr->coreAssigned==-1){
          coreArr[nextCore] = curr->id;
          curr->coreAssigned = nextCore;
          found = 1;
          break;
        }
      }
      if(!found) break; //if the priqueue doesn't have any jobs to insert we don't need to assign to further cores
      for(int i=0;i<coreCount;i++){ //if we're iterating again find a new core to fill
        if(coreArr[i] == -1){
          nextCore = i;
          break;
        }
      }

    }
  }
}
//fill idle cores and preempt lower-priority jobs already running
void preemptCores(int time){
  if(priqueue_size(queue)>0){
    for(int i =0;i<priqueue_size(queue);i++){
      job_t* curr = priqueue_at(queue, i);
      if(curr->coreAssigned!=-1 && !curr->finished){
        int nextCore = -1;
        for(int i=0;i<coreCount;i++){
          if(coreArr[i] ==-1){
            nextCore = i;
            break;
          }
        }
        if(nextCore == -1){ //preempt
          job_t* jobToPreempt = NULL;
          for(int i =0;i<priqueue_size(queue);i++){
            jobToPreempt = priqueue_at(queue, i);
            if(jobToPreempt->coreAssigned!=-1 && queue->compare(jobToPreempt, curr) == 1){ //check this condition
              break; //found job to preempt
            }
          }
          if(jobToPreempt!=NULL){
            curr->coreAssigned = jobToPreempt->coreAssigned;
            jobToPreempt->coreAssigned = -1;
            coreArr[curr->coreAssigned] = curr->id;
            jobToPreempt->timeRun = time-jobToPreempt->arrivalTime;
            if(jobToPreempt->timeRun == 0){ 
              jobToPreempt->scheduleLatency = -1;
            }
          }
        }
        else{ //idle core found 
          curr->coreAssigned = nextCore;
          coreArr[nextCore] = curr->id;
        }
      }
    }
  }
}

void reschedule(int time){
  switch(activeScheme){
    case FCFS:{
      fillIdleCores();
      break;
    }
    case SJF:{
      fillIdleCores();
      break;
    }
    case PSJF:{
      preemptCores(time);
      break;
    }
    case PRI:{
      fillIdleCores();
      break;
    }
    case PPRI:{
      preemptCores(time);
      break;
    }
    case RR:{
      fillIdleCores();
      break;
    }
  }
}



/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
  queue = malloc(sizeof(priqueue_t));
  
  coreCount = cores;
  coreArr = malloc(coreCount*sizeof(int));
  for(int i=0;i<coreCount;i++){
    coreArr[i] = -1;
  }

  activeScheme = scheme;
  switch(scheme){
    case FCFS:{
      priqueue_init(queue, *compareFCFS);
      break;
    }
    case SJF:{
      priqueue_init(queue, *compareSJF);
      break;
    }
    case PSJF:{
      priqueue_init(queue, *comparePSJF);
      break;
    }
    case PRI:{
      priqueue_init(queue,*comparePRI);
      break;
    }
    case PPRI:{
      priqueue_init(queue, *comparePPRI);
      break;
    }
    case RR:{
      priqueue_init(queue, *  compareRR);
      break;
    }

  }
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  job_t* newJob = malloc(sizeof(job_t));

  newJob->arrivalTime=time;
  newJob->id = job_number;
  newJob->duration = running_time;
  newJob->priority = priority;

  newJob->endTime = -1;
  newJob->timeRun = 0;
  newJob->finished = 0;
  newJob->scheduleLatency = -1; 
  newJob->coreAssigned = -1;

  priqueue_offer(queue, newJob);

  reschedule(time);
  updateTimes(time);

	return newJob->coreAssigned;
}



/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  job_t* finishedJob = NULL;
  for(int i=0;i<priqueue_size(queue);i++){
    int currID = ((job_t*)priqueue_at(queue, i))->id;
    if(currID == job_number){
      finishedJob = (job_t*)priqueue_at(queue,i);
      break;
    }
  }
  if(finishedJob==NULL) return coreArr[core_id];
  finishedJob->endTime = time;
  finishedJob->coreAssigned = -1;
  coreArr[core_id] = -1;
  finishedJob->finished = 1;
  updateTimes(time);
  reschedule(time);
	return coreArr[core_id];
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
  float sum = 0;
  for(int i =0;i<priqueue_size(queue);i++){
    job_t* job = priqueue_at(queue, i);
    int length = job->endTime - job->arrivalTime;
    sum += length-(job->duration);
  }
  return sum/priqueue_size(queue);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
  float sum = 0;
  for(int i =0;i<priqueue_size(queue);i++){
    job_t* job = priqueue_at(queue, i);
    int length = job->endTime - job->arrivalTime;
    sum += length;
  }
  return sum/priqueue_size(queue);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
  float sum = 0;
  for(int i =0;i<priqueue_size(queue);i++){
    job_t* job = priqueue_at(queue, i);
    sum += job->scheduleLatency;
  }
  return sum/priqueue_size(queue);

}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{

}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
