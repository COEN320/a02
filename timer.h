/*
 * timer.h
 *
 *  Created on: 29.11.2013
 */

#ifndef TIMER_H_
#define TIMER_H_

// pulses
#define CODE_TIMER          1       // pulse from timer

#include <cstdlib>
#include <iostream>

#include <stdio.h>
#include <pthread.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <errno.h>


// message structure
typedef struct
{
    // contains both message to and from client
    int messageType;
    // optional data, depending upon message
    int messageData;
} ClientMessageT;


typedef union
{
    // a message can be either from a client, or a pulse
    ClientMessageT  msg;
    struct _pulse   pulse;
} MessageT;

class Ctimer
{
	public:
		Ctimer();
		~Ctimer();
		void setupPulseAndTimer(long seconds, long nanoseconds);
		void wait();

		Ctimer(int seconds, int nanoseconds);

	private:
		int m_chid;
};

#endif /* TIMER_H_ */
