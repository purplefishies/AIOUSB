Guide to the Samples directory





*  sample.c    
  
  A basic sample that shows the Immediate and bulk acquire features


* sample2.c 
  
  Slightly different sample than the file sample.c . 



* cust_sample.cpp
  
  An improved sample based off of a customer's example. It allows you to 
  specify bulk acquires, but using an environmental variable you can 
  alternate the size of the bulk buffer, the oversample and the 
  number of scans



* read_channel_test

  Simple sample for performing many GetImmedidate readings. User can 
  specify -c NUM_SCANS on the command line and then watch the readings.
  This is very handy for determining if the card is reading voltages 
  correctly


* test_bulk