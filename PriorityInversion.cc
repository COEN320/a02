//=======================PRIORITY INVERSION==================================
//===========================================================================
//===========================================================================
//===========================================================================
#include <stdio.h>
#include <iostream.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sync.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/syspage.h>
#include "timer.h"

//=============================================================================

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;


#define PCnt 10 /* Maximum number of threads*/
#define eps .005
#define RELEASE_TIME_P1 4
#define RELEASE_TIME_P2 2
#define RELEASE_TIME_P3 0
#define PRIORITY_P1	0.7
#define PRIORITY_P2	0.6
#define PRIORITY_P3	0.5

float priority[PCnt]={0}; // priority of threads

bool isRunable[PCnt]={0};;    // threads are runable?

int active_p=0;			  // detemine the active thread that should be run

// indicate the algorithm here

//#define PRIORITY_CEILING
#define PRIORITY_INHERITANCE

//=============================================================================
// IMPLEMENT THE TIMER HERE

//timer is implemented in timer.h & timer.cc - for code clarity

void ThreadManager();

#ifdef PRIORITY_CEILING
//=======================    PRIORITY_CEILING  ================================

/* with priority ceilings, the shared mutex process (that runs the operating system code)
 *  has a characteristic (high) priority of its own, which is assigned to the task locking
 *   the mutex. This works well, provided the other high priority task(s) that tries to
 *    access the mutex does not have a priority higher than the ceiling priority.*/

class Sem  // The Priority Ceiling Mutex class
{
public:
	    double m_ceilingPriority;
		Sem ();
		~Sem();
		void lock(int numOfThreadThatIsAttemptingToLock);
		void unlock(int p);
private:
		pthread_mutex_t m_resourceMutex;

		int m_lockedBy;
		double m_originalPriority;
		int m_threadThatIsAttemptingToLock;

		bool m_isPriorityChanged;

		bool isLockedBy;

};

Sem::Sem() {
	//initialize the mutex
//	m_resourceMutex=PTHREAD_MUTEX_INITIALIZER;
//	Seb: Correction..To Initialize properly..Constant can only be used on static mutex
	pthread_mutex_init(&m_resourceMutex, NULL);

	m_lockedBy = -1;

	m_isPriorityChanged = false;

	m_originalPriority = 15;

	isLockedBy = false;

	m_ceilingPriority = 0.0;
}

Sem::~Sem(){
}

void Sem::lock(int numOfThreadThatIsAttemptingToLock)
{

	if (m_ceilingPriority <= priority[numOfThreadThatIsAttemptingToLock])
		m_ceilingPriority = priority[numOfThreadThatIsAttemptingToLock];

	if (m_lockedBy!=-1)
	{
		if (priority[m_lockedBy]  < priority[numOfThreadThatIsAttemptingToLock])
		{
			m_originalPriority = priority[m_lockedBy];

			priority[m_lockedBy] = m_ceilingPriority;

			// block the thread that failed to lock the semaphore
			cout<<"\n......P"<<numOfThreadThatIsAttemptingToLock<<" blocked .."<<endl;
			isRunable[numOfThreadThatIsAttemptingToLock] = false;

			m_isPriorityChanged = true;

			// Store number of the thread that failed to lock the semaphore
			m_threadThatIsAttemptingToLock = numOfThreadThatIsAttemptingToLock;
		}
	}

	//Unlock the scheduler mutex
	pthread_mutex_unlock(&mutex);

	//try to lock the resource
	pthread_mutex_lock(&m_resourceMutex);

	m_lockedBy = numOfThreadThatIsAttemptingToLock;

	isLockedBy = true;

	cout << "..... Semaphore locked by P" << m_lockedBy;

}

void Sem::unlock(int numOfUnlockingThread)
{
	if (numOfUnlockingThread == m_lockedBy)
	{

		cout<<"\n......semaphore unlocked by P"<<m_lockedBy<<" .."<<endl;

		// unblock the thread that failed to lock the semaphore
		if(!isRunable[m_threadThatIsAttemptingToLock])
		{
			cout<<"......P"<<m_threadThatIsAttemptingToLock<<" unblocked .."<<endl;
			isRunable[m_threadThatIsAttemptingToLock] = true;
			m_threadThatIsAttemptingToLock = -1;
		}

		//set original priority
		if (m_isPriorityChanged)
		{
			m_isPriorityChanged = false;

			priority[m_lockedBy] = m_originalPriority;

			//Unlock the scheduler mutex
			pthread_mutex_unlock(&mutex);

		}

		m_lockedBy = -1;
		pthread_mutex_unlock(&m_resourceMutex);
	}
}
#endif
//=============================================================================

#ifdef PRIORITY_INHERITANCE
//=============================================================================
class Sem  // The Priority Inheritance Mutex class
{
	public:
		Sem ();
		~Sem();
		void lock(int numOfThreadThatIsAttemptingToLock);
		void unlock(int p);
private:
		int buffer[100];
		pthread_mutex_t m_resourceMutex;

		int buffer2[100];

		int m_lockedBy;
		double m_originalPriority;
		int m_threadThatIsAttemptingToLock;

		bool m_isPriorityChanged;

		bool isLockedBy;

};

Sem::Sem() {
	//initialize the mutex
//	m_resourceMutex=PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&m_resourceMutex, NULL);

	m_lockedBy = -1;

	m_isPriorityChanged = false;

	m_originalPriority = 15;

	isLockedBy = false;
}

Sem::~Sem(){
}

void Sem::lock(int numOfThreadThatIsAttemptingToLock)
{
	//Under the policy of priority inheritance, whenever a high priority task has to wait
	//for some resource shared with an executing low priority task, the low priority task
	//is temporarily assigned the priority of the highest waiting priority task for the duration
	//of its own use of the shared resource.

	if (m_lockedBy!=-1)
	{
		if (priority[m_lockedBy]  < priority[numOfThreadThatIsAttemptingToLock])
		{
			m_originalPriority = priority[m_lockedBy];

			priority[m_lockedBy] = priority[numOfThreadThatIsAttemptingToLock];

			// block the thread that failed to lock the semaphore
			cout<<"\n......P"<<numOfThreadThatIsAttemptingToLock<<" blocked .."<<endl;
			isRunable[numOfThreadThatIsAttemptingToLock] = false;

			m_isPriorityChanged = true;

			m_threadThatIsAttemptingToLock = numOfThreadThatIsAttemptingToLock;
		}
	}

	//Unlock the scheduler mutex
	pthread_mutex_unlock(&mutex);

	//try to lock the resource
	pthread_mutex_lock(&m_resourceMutex);

	m_lockedBy = numOfThreadThatIsAttemptingToLock;

	isLockedBy = true;

	cout << "..... Semaphore locked by P" << m_lockedBy;

}

void Sem::unlock(int numOfUnlockingThread)
{
	if (numOfUnlockingThread == m_lockedBy)
	{

		cout<<"\n......semaphore unlocked by P"<<m_lockedBy<<" .."<<endl;

		// unblock the thread that failed to lock the semaphore
		if(!isRunable[m_threadThatIsAttemptingToLock])
		{
			cout<<"......P"<<m_threadThatIsAttemptingToLock<<" unblocked .."<<endl;
			isRunable[m_threadThatIsAttemptingToLock] = true;
			m_threadThatIsAttemptingToLock = -1;
		}

		//set original priority

		if (m_isPriorityChanged)
		{
			m_isPriorityChanged = false;

			priority[m_lockedBy] = m_originalPriority;

			//Unlock the scheduler mutex
			pthread_mutex_unlock(&mutex);

		}

		m_lockedBy = -1;
		pthread_mutex_unlock(&m_resourceMutex);
	}
}

#endif
//=============================================================================

Sem s[PCnt]; // mutexes are instantiated here

//=============================================================================
void * P1(void* arg)
{
	int cnt=0;


	while(1){
		pthread_mutex_lock(&mutex);
		do{
			pthread_cond_wait(&cond,&mutex); // check for a message from ThreadManager
		}while(active_p!=1);				 // check the active thread

		//+++++++++++++++++++++++++++++++++++++++++++++
			cout<<"P1->["<<cnt<<"]"<<endl;
			if(cnt==1){			// Try to acquire mutex after running for 1 unit
				cout << ".....Attempting to Lock Semaphore ..";
				s[0].lock(1);
			}
			else if(cnt==3){		// Release mutex after running for 3 units
				cout << ".....Unlocking Semaphore ..";
				s[0].unlock(1);
			}
			else if (cnt == 4){		// Finish after 4 units
				//Message msg;
				cout << ".........P1 thread ends.........";
				priority[1]=0; // to remove process 1 from the queue of ThreadManager
				pthread_mutex_unlock(&mutex);
				break;
			}
			cnt++;
		//---------------------------------------------
		pthread_mutex_unlock(&mutex);
	}

}

//=============================================================================
void * P2(void* arg)
{

	int cnt=0;

	while(1){

		pthread_mutex_lock(&mutex);
		do{
			pthread_cond_wait(&cond,&mutex); // check for a message from ThreadManager
		}while(active_p!=2);				 // check the active thread

		//+++++++++++++++++++++++++++++++++++++++++++++
			cout<<"P2->["<<cnt<<"]"<<endl;
			if (cnt == 6){
				cout << ".........P2 thread ends.........";
				priority[2]=0; // to remove process 2 from the queue of ThreadManager
				pthread_mutex_unlock(&mutex);
				break;
			}
			cnt++;
		//---------------------------------------------
		pthread_mutex_unlock(&mutex);
	}
}


//=============================================================================
void * P3(void* arg)
{

	int cnt=0;

	while(1){
		pthread_mutex_lock(&mutex);
		do{
			pthread_cond_wait(&cond,&mutex); // check for a message from ThreadManager
		}while(active_p!=3);				 // check the active thread

		//+++++++++++++++++++++++++++++++++++++++++++++
			cout<<"P3->["<<cnt<<"]"<<endl;
			if(cnt==1){
				cout << ".....Attempting to Lock Semaphore by ..";
				s[0].lock(3);
			}
			else if(cnt==3){
				cout << ".....Unlocking Semaphore by ..";
					s[0].unlock(3);
			}
			else if (cnt == 5){
				cout << ".........P3 thread ends.........";
				priority[3]=0; // to remove process 3 from the queue of ThreadManager
				pthread_mutex_unlock(&mutex);
				break;
			}
			cnt++;
		//---------------------------------------------
		pthread_mutex_unlock(&mutex);
	}
}

void ThreadManager(){ // determines that which thread should be run
	float p;
	int i;

	pthread_mutex_lock(&mutex);

	cout<<" scheduler runs ";

	p=-1;
	for(i=1;i<PCnt;i++){ // find the thread with the most priority and set it as active thread
		if (isRunable[i])
		{
		   if(priority[i]>p)
		   {
			   active_p=i;

				p=priority[i];
		   }
		}
	}

	if (priority[active_p]>0)
		cout<<", active_p="<<active_p<<endl;

	pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&cond); // send the signal to the threads
}

//=============================================================================
//                                 M     A     I     N
//=============================================================================
int main(void)
{
	pthread_t P1_ID,P2_ID, P3_ID;       //p1, p2, p3 threads

	isRunable[1] = true;
	isRunable[2] = true;
	isRunable[3] = true;

	int cnt=0;

	//creating up a periodic  timer to generate pulses every 1 sec.
	Ctimer t(0,10000000);
	// *** TODO: Change back to Ctimer t(1,0);

	while(1)
	{
		//--------------------------------------------
		pthread_mutex_lock(&mutex);

		 // release P1 t= 4
		if(cnt==RELEASE_TIME_P1){
			priority[1]=PRIORITY_P1;
			pthread_create(&P1_ID , NULL, P1, NULL);
		}
		 // release P2 at t=2
		if(cnt==RELEASE_TIME_P2){
			priority[2]=PRIORITY_P2;
			pthread_create(&P2_ID , NULL, P2, NULL);
		}
		 // release P3 at t=0
		if(cnt==RELEASE_TIME_P3){
			priority[3]=PRIORITY_P3;
			pthread_create(&P3_ID , NULL, P3, NULL);
		}
		 // terminate the program at t=30
		if (cnt == 30){
			break;
		}
		pthread_mutex_unlock(&mutex);
		//---------------------------------------------
		// wait for the timer pulse
		t.wait();
		//+++++++++++++++++++++++++++++++++++++++++++++
		// message
		cout<<endl<<"tick="<<cnt<<"   ";//", active_p="<<active_p<<"->";//<<endl;

		ThreadManager(); // to find out and run the active thread
		cnt++;

    	//*********************************************
	}
	return 0;
}
//=============================================================================
