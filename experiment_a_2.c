#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <zconf.h>
#include <math.h>
#include "tsc.c"

double MHz = 0;


u_int64_t inactive_periods(int num, uint64_t threshold, uint64_t *samples) {

    start_counter();
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

double cycles_to_time(u_int64_t cycles) {
    return cycles / (MHz * 1000);
}

u_int64_t measure_frequency() {

    start_counter();
    sleep(2);
    return get_counter() / (2 * 1000000);

}


int calc_threshold() {
    /*
        Run the function 'inactive_periods' with various thresholds, and then choose
        the threshld that leads to minimum standard deviation of the duration of active periods.
        The idea : timer interrupt occurs periodically so ideally the active period durations
        should be of nearly same length everytime.
    */
    int num = 200; // samples to collect per run
    int start_threshold = 100; // starting threshold
    int increment_by = 50;  //increment threshold by this amount in each run
    int max_threshold = start_threshold*10;  // max threshold to test till
    uint64_t *samples = malloc(2*num*sizeof(uint64_t));  // array to store samples
    double *duration = malloc((num-1)*sizeof(double));  // array to store active durations
    int size = ((max_threshold-start_threshold)/increment_by)+1;
    int i;

    long double min_stddev = 999999;
    int min_threshold = 0;
    int threshold=start_threshold;
    while (threshold<max_threshold) {
        // warm_up_cache_n_times(1000000);
        printf("Checking threshold : %d\n", threshold);
        uint64_t first_inactive = inactive_periods(num, threshold, samples);
        // calculate durations in ms of active periods
        int indx=0;
        for(i=0; i<2*num-2; i+=2, indx++) {
            duration[indx] = (samples[i+2]-samples[i+1])/(MHz*1000);
        }

        long double mean=0;
        for (i=0; i<num-1; i++) {
            mean += duration[i];
        }
        mean/=(num-1);
        printf("Mean=%LG\n", mean);

        long double stddev=0;
        for(i=0; i<num-1; i++) {
            stddev += (duration[i]-mean)*(duration[i]-mean);
        }
        stddev/=(num-1);
        stddev=sqrt(stddev);

        if(stddev<min_stddev) {
            min_stddev = stddev;
            min_threshold = threshold;
        }
        printf("Stddev=%LG\n", stddev);
        threshold += increment_by;
    }

    free(samples);
    free(duration);

    return min_threshold;
}


int main(int argc, char *argv[]) {
    int num_inactive_periods = atoi(argv[1]);

    // calculate frequency of the CPU
    MHz = measure_frequency();
    printf("=====The estimated frequency on this machine is\t%fMHz=====\n", MHz);

    //TODO: test with different threshold value to acqurie more stable result
    int threshold = 500;
    uint64_t *samples = malloc(2 * num_inactive_periods * sizeof(uint64_t));


    //TODO: must do stuff to detect context switch


    u_int64_t first_active_start = inactive_periods(num_inactive_periods, threshold, samples);

    uint64_t *active_cycles = malloc(num_inactive_periods * sizeof(double));
    uint64_t *inactive_cycles = malloc(num_inactive_periods * sizeof(double));

    uint64_t first_active_cycles = samples[0] - first_active_start;

    active_cycles[0] = first_active_cycles;
    for (int i = 0; i < num_inactive_periods + 2; i++) {
        inactive_cycles[i] = (samples[i + 1] - samples[i]);
        active_cycles[i + 1] = (samples[i + 2] - samples[i + 1]);
    }

    //count total cycles for gnu plot
    double total_cycles = 0;
    FILE *file;
    int index = 1;
    double x_prev, x_next;
    file = fopen("gnu_plot2.gplot", "w+");

    // print result of inactive period measurement
    printf("Active 0: start at %lu, duration %lu cycles (%fms)\n",
           first_active_start,
           active_cycles[0],
           cycles_to_time(first_active_cycles));

    x_prev = 0;
    x_next = cycles_to_time(first_active_cycles);
    total_cycles += first_active_cycles;

    fprintf(file, "set title \"Experiment A-2: Active and inactive periods\"\n");
    fprintf(file, "set xlabel \"Time(ms)\"\n");
    fprintf(file, "set nokey\n");
    fprintf(file, "set noytics\n");

    fprintf(file, "set term postscript eps 10\n");
    fprintf(file, "set output \"experiment_a_2.eps\"\n");
    fprintf(file, "set object %d rect from %f, 1 to %f, 2 fs empty\n", index, x_prev, x_next);
    index++;

    for (int i = 0; i < num_inactive_periods * 2 - 2; i += 2) {

        printf("Inactive %d: start at %lu, duration %lu cycles (%fms)\n",
               i / 2,
               samples[i],
               samples[i + 1] - samples[i],
               cycles_to_time(inactive_cycles[i]));

        printf("Active %d: start at %lu, duration %lu cycles (%fms)\n",
               i / 2 + 1,
               samples[i + 1],
               samples[i + 2] - samples[i + 1],
               cycles_to_time(active_cycles[i + 1]));


        x_prev = x_next;
        x_next = x_prev + cycles_to_time(samples[i + 1] - samples[i]);
        fprintf(file, "set object %d rect from %f, 1 to %f, 2 fc rgb \"black\" fs solid\n", index, x_prev, x_next);


        x_prev = x_next;
        x_next = x_prev + cycles_to_time(samples[i + 2] - samples[i + 1]);
        fprintf(file, "set object %d rect from %f, 1 to %f, 2 fs empty\n", index, x_prev, x_next);
        index++;

        //update total number of cycles
        total_cycles += inactive_cycles[i];
        total_cycles += active_cycles[i + 1];
    }

    fprintf(file, "plot [0:%f] [0:3] 0", cycles_to_time(total_cycles));



    // calculate time slice

    //


    //deallocate memory address
    free(samples);
    free(active_cycles);
    free(inactive_cycles);

    return 0;


}