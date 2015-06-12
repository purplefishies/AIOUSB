#ifndef _COMMON_H
#define _COMMON_H

#include <getopt.h>

struct channel_range {
  int start_channel;
  int end_channel;
  int gaincode;
};

struct opts {
    unsigned num_scans;
    unsigned num_channels;
    unsigned num_oversamples;
    int gain_code;
    int clock_rate;
    char *outfile;
    int reset;
    int debug_level;
    int number_ranges;
    int verbose;
    int start_channel;
    int end_channel;
    int index;
    int block_size;
    int with_timing;
    int slow_acquire;
    struct channel_range **ranges;
};


struct channel_range *get_channel_range(char *optarg );
void process_aio_cmd_line( struct opts *options, int argc, char *argv [] );
void print_aio_usage(int argc, char **argv,  struct option *options);
void aio_list_devices(struct opts *options, int *indices, int num_devices );


extern struct opts AIO_OPTIONS;


#endif
