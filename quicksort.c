#include <LPC17xx.h>
#include <RTL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "quicksort.h"
#include "array_tools.h"
#include "functions.h"


/***********************************************
	Quick Sort Implementation
	-------------------------
	
	Acknowledgements:
		
		Consulted http://rosettacode.org/wiki/Sorting_algorithms/Quicksort
							for quicksort concept clarification
		
************************************************/



// You decide what the threshold will be
#define USE_INSERTION_SORT 6

/**********************************
	Macro Helper Routines
**********************************/

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define swap(array, x,y) { array[x] = array[x] + array[y]; array[y] = array[x] - array[y]; array[x] = array[x] - array[y]; }


/**********************************
	Data Structures
**********************************/
	
	
//structs
typedef struct {
	array_t array;
	size_t a;
	size_t c;
} array_interval_t;

typedef struct{
	array_interval_t interval;
	unsigned char priority;
} qsort_task_parameters_t;

typedef struct{
	array_interval_t interval;
	unsigned char priority;
	OS_SEM   sem;
} qsort_sem_parameters_t;

/**********************************
	Insertion Sort Implementation
**********************************/

void insertion_sort( array_interval_t interval ) {
	
	/**********************************
	Get array size, sort within bounds
	**********************************/
	
	size_t array_size = interval.c - interval.a ;
	
	U32 i, j = i;
	
	if (array_size > 1){
		for (i = interval.a; i <= interval.c ; i++){
			j = i;
			while (j > interval.a  && interval.array.array[j - 1] > interval.array.array[j]){
			
				swap(interval.array.array, j, j - 1);
				j--;
			
			}
		}
	}
}

/**********************************
	Quick Sort Task Implementation
**********************************/

__task void quick_sort_task( void* void_ptr){

	/**********************************
		Parameter Extraction
	**********************************/
	
	U32 a, c, j;
	
	array_type * array;
	array_interval_t interval;
	
	qsort_task_parameters_t task_param_left;
	qsort_task_parameters_t task_param_right;
	
	qsort_task_parameters_t * task_param;
	
	task_param  = (qsort_task_parameters_t *)void_ptr;
	array = task_param -> interval.array.array;
	interval =  task_param -> interval;
	
	a = interval.a;
	c = interval.c;
	
	/**********************************
		Insertion Sort Quick Exit
	**********************************/

	if (c - a < USE_INSERTION_SORT){
		insertion_sort(interval);
		os_tsk_delete_self();
	}
	
	/**********************************
		Iterate QuickSort
	**********************************/
	
	j = iterateQuickSort((char*)array, a, c);
	
	/**********************************
		Create Child Tasks
	**********************************/
	
	task_param_left = *task_param;
	task_param_right = *task_param;
	
	task_param_left.priority++;
	task_param_right.priority++;
	
	task_param_left.interval.c = j;
	task_param_right.interval.a = j + 1;

	os_tsk_create_ex( quick_sort_task, task_param_left.priority, &task_param_left ); 
	os_tsk_create_ex( quick_sort_task, task_param_right.priority, &task_param_right ); 

	os_tsk_delete_self();
}

/*************************************
	Quick Sort Semaphore Implementation
*************************************/

__task void quick_sort_task_sem( void* void_ptr){
  
	/**********************************
			Local Variables
	**********************************/
	
	U32 a, c, j;
	
	array_type * array;
	array_interval_t interval;
	
	qsort_sem_parameters_t sem_param_left;
	qsort_sem_parameters_t sem_param_right;
	
	qsort_sem_parameters_t * sem_param;
	
	sem_param  = (qsort_sem_parameters_t *)void_ptr;
	
	array = sem_param -> interval.array.array;
	interval = sem_param -> interval;
	
	/**********************************
		Setting up bounds
	**********************************/	
	
	a = interval.a;
	c = interval.c;

	/**********************************
			Insertion Sort Quick Exit
	**********************************/	
		
	if (c - a < USE_INSERTION_SORT){
		insertion_sort(interval);
		
		/**********************************
			Post Semaphore and delete task
		**********************************/
		
		os_sem_send(&sem_param -> sem);
		os_tsk_delete_self();
	}
	
	/************************************************
		Iterate Quick Sort and retrieve pivot index
	************************************************/
	
	j = iterateQuickSort((char*)array, a, c);
	
	/**********************************
		Set up new task parameters
	**********************************/
	
	sem_param_left = *sem_param;
	sem_param_right = *sem_param;
	
	sem_param_left.priority++;
	sem_param_right.priority++;
	
	sem_param_left.interval.c = j;
	sem_param_right.interval.a = j + 1;
	
	os_sem_init(&sem_param_left.sem, 0);
	os_sem_init(&sem_param_right.sem, 0);
	
	/******************************************
		Create new tasks and manage task flow
	******************************************/
	
	os_tsk_create_ex( quick_sort_task_sem, sem_param_left.priority, &sem_param_left );
	
	os_sem_wait(&sem_param_left.sem, 0xFFFF);
	
	os_tsk_create_ex( quick_sort_task_sem, sem_param_right.priority, &sem_param_right ); 
	
	os_sem_wait(&sem_param_right.sem, 0xFFFF);
	
	/********************************************
		Send semaphore to allow parent to proceed
	********************************************/
	
	os_sem_send(&sem_param -> sem);
	os_tsk_delete_self();
}

/**********************************
	Quicksort parent task
**********************************/

void quicksort_sem( array_t array ) {
	
	/**********************************
		Setup locals
	**********************************/
	
	array_interval_t interval;
	qsort_sem_parameters_t sem_param;

	interval.array =  array;
	interval.a     =  0;
	interval.c     =  array.length - 1;
	
	sem_param.interval = interval;
	sem_param.priority = 10;
	
	/**************************************
		Create First iteration of Quick sort
	**************************************/
	
 	os_sem_init(&sem_param.sem, 0);
	
	os_tsk_create_ex( quick_sort_task_sem, sem_param.priority, &sem_param ); 
	
	os_sem_wait(&sem_param.sem, 0xFFFF);
	
}

/**********************************
		Quick sort task parent task
**********************************/

void quicksort( array_t array ) {
	array_interval_t interval;
	qsort_task_parameters_t task_param;

	interval.array =  array;
	interval.a     =  0;
	interval.c     =  array.length - 1;
	
	task_param.interval = interval;

	// If you are using priorities, you can change this
	task_param.priority = 10;
	
	os_tsk_create_ex( quick_sort_task, task_param.priority, &task_param ); 
}

/*************************************************
		Swap Values about Pivot, returns Pivot Index
*************************************************/

int iterateQuickSort(char * array, U32 left, U32 right) {
	
	/*************************************************
		Local Variable Declaration and Pivot Selection
	*************************************************/
	
	U32 i, j;
	char pivot;
	
	/**********************************
		Select First Index for Pivot
	**********************************/
	
	pivot = array[left];
	
	/******************************************
		Since do-while implementation was used
		must start one offset from initial walls
	******************************************/
	
	i = left - 1;
	j = right + 1;

	while(1){
		
		/**********************************
			Iterate Left Wall Looking
			for Val Greater than Pivot
		**********************************/
		do {
			i++;
		}while( array[i] < pivot);
		
		/**********************************
			Iterate Right Wall Looking
			for Val Less than Pivot
		**********************************/
		
		do {
			j--;
		}while( array[j] > pivot );
		
		/*************************************
			Exit condition when i and j overlap
		*************************************/
		
		if( i >= j )
			return j;
		
		/************************************
			When Exit condition is not called
			Swap values and continue
		************************************/
		
		swap(array, i, j);
		
	}
}

/**********************************
	Function used for debugging
**********************************/

void printArray(char * array, int a, int c){
	int i;
	printf("Start Printing Array \n");
	for (i = a; i <= c; i++)
		printf("%i\n", array[i]);
	printf("End \n");
	return;
}


