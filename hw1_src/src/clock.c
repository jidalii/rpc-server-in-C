/*******************************************************************************
* CPU Clock Measurement Using RDTSC
*
* Description:
*     This C file provides functions to compute and measure the CPU clock using
*     the `rdtsc` instruction. The `rdtsc` instruction returns the Time Stamp
*     Counter, which can be used to measure CPU clock cycles.
*
* Author:
*     Jida Li
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Notes:
*     Ensure that the platform supports the `rdtsc` instruction before using
*     these functions. Depending on the CPU architecture and power-saving
*     modes, the results might vary. Always refer to the CPU's official
*     documentation for accurate interpretations.
*
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "timelib.h"

int main (int argc, char ** argv)
{
	/* IMPLEMENT ME! */
	if (argc != 4) {
        printf("The program has 4 inputs: program <seconds> <nanoseconds> <method>\n");
        return 1;
    }
    long sec = atol(argv[1]);
    long nsec = atol(argv[2]);
    char method = argv[3][0];
    
    uint64_t elapsed_clock_cycles;
    if (method == 's') {
        elapsed_clock_cycles = get_elapsed_sleep(sec, nsec);
        printf("WaitMethod: SLEEP\n");
    } else {
        elapsed_clock_cycles = get_elapsed_busywait(sec, nsec);
        printf("WaitMethod: BUSYWAIT\n");
    } 
    double elapsed_time_in_sec = (double)sec + ((double)nsec / NANO_IN_SEC);
    double clock_speed = (double)elapsed_clock_cycles / elapsed_time_in_sec / 1000000;

    printf("WaitTime: %ld %ld\n", sec, nsec);
    printf("ClocksElapsed: %lu\n", elapsed_clock_cycles);
    printf("ClockSpeed: %.2lf\n", clock_speed);


	return EXIT_SUCCESS;
}

