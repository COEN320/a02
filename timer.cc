#include "timer.h"

Ctimer::Ctimer()
{
	if ((m_chid = ChannelCreate (0)) == -1) {
		        fprintf (stderr, "couldn't create channel!\n");
		        perror (NULL);
		        exit (EXIT_FAILURE);
		    }
}


Ctimer::Ctimer(int seconds, int nanoseconds)
{
	Ctimer();

	setupPulseAndTimer(seconds, nanoseconds);

}

Ctimer::~Ctimer()
{
}



void
Ctimer::setupPulseAndTimer (long seconds, long nanoseconds)
{
    timer_t             timerid;    // timer ID for timer
    struct sigevent     event;      // event to deliver
    struct itimerspec   timer;      // the timer data structure
    int                 coid;       // connection back to ourselves

    // create a connection back to ourselves
    coid = ConnectAttach (0, 0, m_chid, 0, 0);
    if (coid == -1) {
        fprintf (stderr, "couldn't ConnectAttach to self! %d\n", errno);
        perror (NULL);
        exit (EXIT_FAILURE);
    }

    // set up the kind of event that we want to deliver -- a pulse
    SIGEV_PULSE_INIT (&event, coid,
                      SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER, 0);

    // create the timer, binding it to the event
    if (timer_create (CLOCK_REALTIME, &event, &timerid) == -1) {
        fprintf (stderr, "couldn't create a timer, errno %d\n", errno);
        perror (NULL);
        exit (EXIT_FAILURE);
    }

    // setup the timer (1s delay, 1s reload)
    timer.it_value.tv_sec     = seconds;
    timer.it_value.tv_nsec    = nanoseconds;
    timer.it_interval.tv_sec  = seconds;
    timer.it_interval.tv_nsec = nanoseconds;

    // and start it!
    timer_settime (timerid, 0, &timer, NULL);
}
void Ctimer::wait(){
	// Wait for timer tick
	int rcvid;              // process ID of the sender
    MessageT msg;           // the message itself


     rcvid = MsgReceive (m_chid, &msg, sizeof (msg), NULL);

     /*time_t  now;

             time (&now);
             printf ("Got a Pulse at %s", ctime (&now));*/
}
