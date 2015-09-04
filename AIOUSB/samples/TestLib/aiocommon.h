#ifndef _COMMON_H
#define _COMMON_H

#include "aiousb.h"
#include <getopt.h>

struct channel_range {
  int start_channel;
  int end_channel;
  int gaincode;
};

struct opts {
    unsigned long num_scans;
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
    int buffer_size;
    int rate_limit;
    int physical;
    int counts;
    int calibration;
    char *aiobuf_json;
    char *adcconfig_json;
    struct channel_range **ranges;
};


struct channel_range *get_channel_range(char *optarg );
void process_aio_cmd_line( struct opts *options, int argc, char *argv [] );
void print_aio_usage(int argc, char **argv,  struct option *options);
AIORET_TYPE aio_list_devices(struct opts *options, int *indices, int num_devices );
AIORET_TYPE aio_override_aiobuf_settings( AIOContinuousBuf *buf, struct opts *options );
AIORET_TYPE aio_override_adcconfig_settings( ADCConfigBlock *config, struct opts *options );


extern struct opts AIO_OPTIONS;


#endif
