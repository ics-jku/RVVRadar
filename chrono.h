#ifndef CHRONO_H
#define CHRONO_H

#include <time.h>

/*
 * Usage example
 *
 * chrono_t chrono;
 * chrono_reset(&chrono);
 * loop {
 * 	chrono_start(&chrono);
 * 	<function to measure>
 * 	if fail
 * 		continue
 * 	chrono_stop(&chrono);
 * }
 * <print statistics from chrono_t>
 *
 * Note:
 * A measurement can be aborted (e.g. fail of function to measure) simply
 * by skipping chrono_stop (see fail handling above)
 */



typedef struct chrono {
	/* last start and end times */
	struct timespec tstart;
	struct timespec tend;

	int count;

	/* statistics */
	unsigned long long tdlast;
	unsigned long long tdmin;
	unsigned long long tdmax;
	unsigned long long tdsum;
	unsigned long long tdavg;
	/* TODO: var/stdev */
} chrono_t;


/*
 * reset chronometer and statistics
 * return: 0 .. ok; <0 .. error
 */
int chrono_reset(chrono_t *chrono);


/*
 * start chronometer
 * return: 0 .. ok; <0 .. error
 */
int chrono_start(chrono_t *chrono);


/*
 * stop chronometer and update statistic
 * return: 0 .. ok; <0 .. error
 */
int chrono_stop(chrono_t *chrono);

#endif /* CHRONO_H */
