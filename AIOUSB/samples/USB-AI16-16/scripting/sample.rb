#!/usr/bin/python

require 'AIOUSB'
#sys.path.append('build/lib.linux-x86_64-2.7')
#import AIOUSB

deviceIndex = 0
AIOUSB.AIOUSB_Init()
AIOUSB.AIOUSB_Reset( deviceIndex )
AIOUSB.AIOUSB_SetCommTimeout( deviceIndex, 1000 )
AIOUSB.AIOUSB_SetDiscardFirstSample( deviceIndex, AIOUSB::AIOUSB_TRUE )

cb = AIOUSB.AIOUSB_GetConfigBlock( deviceIndex  )

AIOUSB.AIOUSB_InitConfigBlock(cb, deviceIndex, AIOUSB::AIOUSB_FALSE )
AIOUSB.AIOUSB_SetAllGainCodeAndDiffMode(cb, AIOUSB::AD_GAIN_CODE_10V, AIOUSB::AIOUSB_FALSE )
AIOUSB.AIOUSB_SetCalMode(cb, AIOUSB::AD_CAL_MODE_NORMAL )
AIOUSB.AIOUSB_SetTriggerMode(cb, 0 )
AIOUSB.AIOUSB_SetScanRange(cb, 2, 13 )
AIOUSB.AIOUSB_SetOversample(cb, 0 )
AIOUSB.ADC_CopyConfig( deviceIndex,cb )


result = AIOUSB.ADC_SetCal( deviceIndex, ":AUTO:" )


CAL_CHANNEL = 5

AIOUSB.ADC_SetOversample( deviceIndex, 0 )
AIOUSB.ADC_SetScanLimits( deviceIndex, CAL_CHANNEL, CAL_CHANNEL )
AIOUSB.ADC_ADMode( deviceIndex, 0 , AIOUSB::AD_CAL_MODE_GROUND )

if result == AIOUSB::AIOUSB_SUCCESS
    print "Ground counts = %u (should be approx. 0)\n" 
else
    puts sprintf("Error '%s' attempting to read ground counts\n", AIOUSB::AIOUSB_GetResultCodeAsString( result.abs ))
end

AIOUSB.ADC_ADMode( deviceIndex, 0 , AIOUSB.AD_CAL_MODE_REFERENCE )

AIOUSB.AIOUSB_Reset( deviceIndex )
AIOUSB.ADC_SetOversample( deviceIndex, 10 )
AIOUSB.ADC_SetScanLimits( deviceIndex, 0, 15 )
AIOUSB.AIOUSB_SetStreamingBlockSize( deviceIndex, 100352 )

bb = AIOUSB.CreateSmartBuffer( deviceIndex )

clockHz = AIOUSB.new_dp()
AIOUSB.dp_assign( clockHz , 1 )
AIOUSB.CTR_StartOutputFreq( deviceIndex, 0, clockHz )

AIOUSB.ADC_ADMode( deviceIndex, AIOUSB::AD_TRIGGER_SCAN | AIOUSB::AD_TRIGGER_TIMER, AIOUSB::AD_CAL_MODE_NORMAL )
AIOUSB.AIOUSB_SetMiscClock( deviceIndex, 100000 )

result = AIOUSB.BulkAcquire( deviceIndex,  bb , bb.bufsize  )

if result != AIOUSB::AIOUSB_SUCCESS
    puts sprintf("Error '%s' attempting to start bulk acquire of %d bytes\n", AIOUSB.AIOUSB_GetResultCodeAsString( result.abs ), bb.bufsize )
    exit(1)
end

puts sprintf("Remaning is %d\n", bb.bytes_remaining )
for seconds in (1..100)
    time.sleep( 1 )

    result = AIOUSB.BulkPoll( deviceIndex, bb )

    if result == AIOUSB::AIOUSB_SUCCESS
        puts sprintf("  %lu bytes remaining\n", bb.bytes_remaining )
        if bb.bytes_remaining == 0:
            break
        end
    else
      puts sprintf("Error '%s' polling bulk acquire progress" ,AIOUSB.AIOUSB_GetResultCodeAsString( result.abs ))
      break
    end
end
