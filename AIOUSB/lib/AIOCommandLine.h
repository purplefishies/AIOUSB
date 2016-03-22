#ifndef _AIO_COMMAND_LINE_H
#define _AIO_COMMAND_LINE_H

#include "AIOTypes.h"
#include "ADCConfigBlock.h"
#include "AIOContinuousBuffer.h"
#include "AIOConfiguration.h"
#include "AIOUSB_Core.h"
#include "AIODeviceTable.h"
#include "AIOUSB_Properties.h"
#include "AIOUSB_Log.h"
#include <getopt.h>
#include <ctype.h>


#ifdef __aiousb_cplusplus
namespace AIOUSB
{
#endif

#define DUMP   0x1000
#define CNTS   0x1001
#define JCONF  0x1002
#define REPEAT 0x1003

typedef struct AIOChannelRangeTmp {
    int start_channel;
    int end_channel;
    int gaincode;
} AIOChannelRangeTmp;


typedef struct AIOCommandLineOptions {
    int64_t num_scans;
    int64_t default_num_scans;
    int num_channels;
    int default_num_channels;
    int num_oversamples;
    int default_num_oversamples;
    int gain_code;
    int clock_rate;
    int default_clock_rate;
    char *outfile;
    int reset;
    int debug_level;
    int number_ranges;
    int verbose;
    int start_channel;
    int default_start_channel;
    int end_channel;
    int default_end_channel;
    int index;
    int block_size;
    int with_timing;
    int slow_acquire;
    int buffer_size;
    int rate_limit;
    int physical;
    int counts;
    int calibration;            
    int repeat;
    char *aiobuf_json;
    char *default_aiobuf_json;
    char *adcconfig_json;
    AIOChannelRangeTmp **ranges;
 } AIOCommandLineOptions;


typedef enum {
    INDEX_NUM = 0,
    ADCCONFIG_OPT,
    TIMEOUT_OPT,
    DEBUG_OPT,
    SETCAL_OPT,
    COUNT_OPT,
    SAMPLE_OPT,
    FILE_OPT,
    CHANNEL_OPT
} DeviceEnum;

/* BEGIN AIOUSB_API */
PUBLIC_EXTERN AIORET_TYPE AIOProcessCmdline( AIOCommandLineOptions *options, int argc, char **argv);
PUBLIC_EXTERN AIOChannelRangeTmp *AIOGetChannelRange(char *optarg );
PUBLIC_EXTERN void AIOPrintUsage(int argc, char **argv,  struct option  *options);
/* END AIOUSB_API */


extern AIOCommandLineOptions AIO_DEFAULT_CMDLINE_OPTIONS;

#ifdef __aiousb_cplusplus
}
#endif



#endif



