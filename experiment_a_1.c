#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <zconf.h>
#include <math.h>

#include "tsc.c"

double MHz = 0;


u_int64_t inactive_periods(int num, uint64_t threshold, uint64_t *samples) {

    start = 0;

    int inactive_periods_count = 0;

    u_int64_t first_active_start = get_counter();

    u_int64_t curr_start = 0;
    u_int64_t curr_end = 0;

    while (inactive_periods_count < num) {
        curr_start = get_counter();
        curr_end = get_counter();

        if (curr_end - curr_start > threshold) {

            samples[inactive_periods_count * 2] = curr_start;
            samples[inactive_periods_count * 2 + 1] = curr_end;

            //now increase the count since an inactive period has been collected
            inactive_periods_count++;
        }
    }
    return first_active_start;


}


u_int64_t measure_frequency() {

    start_counter();
    sleep(2);

    return get_counter() / (2 * 1000000);
}

double cycles_to_time(u_int64_t cycles) {
    return cycles / (MHz * 1000);
}

int main(int argc, char *argv[]) {

    int num_inactive_periods = atoi(argv[1]);

    // calculate frequency of the CPU
    MHz = measure_frequency();
    printf("=====The estimated frequency on this machine is\t%fMHz=====\n", MHz);

    //TODO: figure out how to warm up the cache

    //TODO: test with different threshold value to acqurie more stable result
    int threshold = 100;

    // allocate memory addresses to store the inactive periods.
    uint64_t *samples = malloc(2 * num_inactive_periods * sizeof(uint64_t));
    uint64_t *active_cycles = malloc(num_inactive_periods * sizeof(double));
    uint64_t *inactive_cycles = malloc(num_inactive_periods * sizeof(double));

    u_int64_t first_active_start = inactive_periods(num_inactive_periods, threshold, samples);
    uint64_t first_active_cycles = samples[0] - first_active_start;

    active_cycles[0] = first_active_cycles;
    for (int i = 0; i < num_inactive_periods + 2; i++) {
        inactive_cycles[i] = (samples[i + 1] - samples[i]);
        active_cycles[i + 1] = (samples[i + 2] - samples[i+1]);
    }

    // implement gnu plot graph
    double total_cycles = 0;
    FILE *file;
    int index = 1;
    double x_prev, x_next;
    file = fopen("gnu_plot.gplot", "w+");

    // print result of inactive period measurement
    printf("Active 0: start at %lu, duration %lu cycles (%fms)\n",
           first_active_start,
           active_cycles[0],
           cycles_to_time(first_active_cycles));

    x_prev = 0;
    x_next = cycles_to_time(first_active_cycles);
    total_cycles += first_active_cycles;


    fprintf(file, "set title \"Experiment A-1: Active and inactive periods\"\n");
    fprintf(file, "set xlabel \"Time(ms)\"\n");
    fprintf(file, "set nokey\n");
    fprintf(file, "set noytics\n");

    fprintf(file, "set term postscript eps 10\n");
    fprintf(file, "set output \"experiment_a_1.eps\"\n");
    fprintf(file, "set object %d rect from %f, 1 to %f, 2 fs empty\n", index, x_prev, x_next);
    index++;

    for (int i = 0; i < num_inactive_periods * 2 - 2; i += 2) {

        printf("Inactive %d: start at %lu, duration %lu cycles (%fms)\n",
               i / 2,
               samples[i],
               samples[i+1] - samples[i],
               cycles_to_time(inactive_cycles[i]));

        printf("Active %d: start at %lu, duration %lu cycles (%fms)\n",
               i / 2 + 1,
               samples[i + 1],
               samples[i+2] - samples[i+1],
               cycles_to_time(active_cycles[i + 1]));

        x_prev = x_next;
        x_next = x_prev + cycles_to_time(samples[i+1] - samples[i]);
        fprintf(file, "set object %d rect from %f, 1 to %f, 2 fc rgb \"black\" fs solid\n", index, x_prev, x_next);



        x_prev = x_next;
        x_next = x_prev + cycles_to_time(samples[i+2] - samples[i+1]);
        fprintf(file, "set object %d rect from %f, 1 to %f, 2 fs empty\n", index, x_prev, x_next);
        index++;

        //update total number of cycles
        total_cycles += inactive_cycles[i];
        total_cycles += active_cycles[i+1];
    }

    fprintf(file, "plot [0:%f] [0:3] 0", cycles_to_time(total_cycles));



    // calculate time took to handle timer interrupt

    // calculate frequency of timer interrupt

    // calculate time lost to servicing interrupt


    //deallocate memory address
    free(samples);

    return 0;


}