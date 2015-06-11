#ifndef _COMMON_H
#define _COMMON_H

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
    unsigned max_count;
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
    struct channel_range **ranges;
};


struct channel_range *get_channel_range(char *optarg );
void process_aio_cmd_line( struct opts *options, int argc, char *argv [] );



#endif
