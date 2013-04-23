/*
 * $RCSfile: AIOUSB_ADC_ExternalCal.c,v $
 * $Revision: 1.5 $
 * $Date: 2009/11/11 19:33:26 $
 * jEdit:tabSize=4:indentSize=4:collapseFolds=1:
 *
 * ACCES I/O USB API for Linux
 */



#include "AIOUSB_Core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#ifdef __cplusplus
namespace AIOUSB {
#endif




static int CompareVoltage( const void *p1, const void *p2 ) {
	assert( p1 != 0
		&& p2 != 0 );
	const double voltage1 = *( double * ) p1
		, voltage2 = *( double * ) p2;
	if( voltage1 < voltage2 )
		return -1;
	else if( voltage1 > voltage2 )
		return 1;
	else
		return 0;
}	// CompareVoltage()


unsigned long AIOUSB_ADC_ExternalCal(
	unsigned long DeviceIndex
	, const double points[]
	, int numPoints
	, unsigned short returnCalTable[]
	, const char *saveFileName
) {
	if(
		points == 0
		|| numPoints < 2
		|| numPoints > CAL_TABLE_WORDS
	)
		return AIOUSB_ERROR_INVALID_PARAMETER;

	const int INPUT_COLUMNS = 2
		, COLUMN_VOLTS = 0
		, COLUMN_COUNTS = 1;
	int index;
	for( index = 0; index < numPoints; index++ ) {
		if(
			points[ index * INPUT_COLUMNS + COLUMN_COUNTS ] < 0
			|| points[ index * INPUT_COLUMNS + COLUMN_COUNTS ] > AI_16_MAX_COUNTS
		) {
#if defined( DEBUG_EXT_CAL )
			printf( "Error: invalid count value at point (%0.3f,%0.3f)\n"
				, points[ index * INPUT_COLUMNS + COLUMN_VOLTS ]
				, points[ index * INPUT_COLUMNS + COLUMN_COUNTS ] );
#endif
			return AIOUSB_ERROR_INVALID_PARAMETER;
		}	// if( points[ ...
	}	// for( index ...

	if( ! AIOUSB_Lock() )
		return AIOUSB_ERROR_INVALID_MUTEX;

	unsigned long result = AIOUSB_Validate( &DeviceIndex );
	if( result != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( result ...

	DeviceDescriptor *const deviceDesc = &deviceTable[ DeviceIndex ];
	if( deviceDesc->bADCStream == AIOUSB_FALSE ) {
		AIOUSB_UnLock();
		return AIOUSB_ERROR_NOT_SUPPORTED;
	}	// if( deviceDesc->bADCStream ...

	if( ( result = ADC_QueryCal( DeviceIndex ) ) != AIOUSB_SUCCESS ) {
		AIOUSB_UnLock();
		return result;
	}	// if( ( result ...

	AIOUSB_UnLock();

	/*
	 * sort table into ascending order by input voltage; then verify that both the
	 * input voltages and the measured counts are unique and uniformly increasing;
	 * since the user's points[] array is declared to be 'const' we need to allocate
	 * a working table that we can sort; in addition, we want to allocate space for
	 * a slope and offset between each pair of points; so while points[] is like a
	 * table with numPoints rows and two columns (input voltage, measured counts),
	 * the working table effectively has the same number of rows, but four columns
	 * (input voltage, measured counts, slope, offset)
	 *
	 *       points[] format:
	 *       +-----------------+       +-----------------+
	 *   [0] |  input voltage  |   [1] | measured counts |
	 *       |=================|       |=================|
	 *   [2] |  input voltage  |   [3] | measured counts |
	 *       |=================|       |=================|
	 *                            ...
	 *       |=================|       |=================|
	 * [n-2] |  input voltage  | [n-1] | measured counts |
	 *       +-----------------+       +-----------------+
	 * 'n' is not numPoints, but numPoints*2
	 */
	const int WORKING_COLUMNS = 4
		, COLUMN_SLOPE = 2
		, COLUMN_OFFSET = 3;
	double *const workingPoints = ( double * ) malloc( numPoints * WORKING_COLUMNS * sizeof( double ) );
	assert( workingPoints != 0 );
	if( workingPoints != 0 ) {
		/*
		 * copy user's table to our working table and set slope and offset to valid values
		 */
		for( index = 0; index < numPoints; index++ ) {
			workingPoints[ index * WORKING_COLUMNS + COLUMN_VOLTS ] = points[ index * INPUT_COLUMNS + COLUMN_VOLTS ];
			workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ] = points[ index * INPUT_COLUMNS + COLUMN_COUNTS ];
			workingPoints[ index * WORKING_COLUMNS + COLUMN_SLOPE ] = 1.0;
			workingPoints[ index * WORKING_COLUMNS + COLUMN_OFFSET ] = 0.0;
		}	// for( index ...

		/*
		 * sort working table in ascending order of input voltage
		 */
		qsort( workingPoints, numPoints, WORKING_COLUMNS * sizeof( double ), CompareVoltage );

		/*
		 * verify that input voltages and measured counts are unique and ascending
		 */
		for( index = 1 /* yes, 1 */; index < numPoints; index++ ) {
			if(
				workingPoints[ index * WORKING_COLUMNS + COLUMN_VOLTS ]
					<= workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_VOLTS ]
				|| workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ]
					<= workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_COUNTS ]
			) {
#if defined( DEBUG_EXT_CAL )
				printf( "Error: points (%0.3f,%0.3f) and (%0.3f,%0.3f) are not unique or not increasing\n"
					, workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_VOLTS ]
					, workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_COUNTS ]
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_VOLTS ]
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ] );
#endif
				result = AIOUSB_ERROR_INVALID_PARAMETER;
				break;
			}	// if( workingPoints[ ...
		}	// for( index ...

		/*
		 * if table of calibration points looks good, then proceed to calculate slopes and
		 * offsets of line segments between points; we verified that no two points in the
		 * table are equal, so we should not get any division by zero errors
		 */
		if( result == AIOUSB_SUCCESS ) {
			/*
			 * the calibration table really only applies to one range if precision is our
			 * objective; therefore, we assume that all the channels are configured for the
			 * same range during calibration mode, and that the user is still using the same
			 * range now as when they collected the calibration data points; if all these
			 * assumptions are correct, then we can use the range setting for channel 0
			 *
			 * the calculations are based on the following model:
			 *   mcounts = icounts x slope + offset
			 * where,
			 *   mcounts is the measured counts (reported by an uncalibrated A/D)
			 *   icounts is the input counts from an external voltage source
			 *   slope is the gain error inherent in the A/D and associated circuitry
			 *   offset is the offset error inherent in the A/D and associated circuitry
			 * to reverse the effect of these slope and offset errors, we use this equation:
			 *   ccounts = ( mcounts – offset ) / slope
			 * where,
			 *   ccounts is the corrected counts
			 * we calculate the slope and offset using these equations:
			 *   slope = ( mcounts[s] – mcounts[z] ) / ( icounts[m] – icounts[z] )
			 *   offset = mcounts[z] – icounts[z] x slope
			 * where,
			 *   [s] is the reading at "span" (the upper reference point)
			 *   [z] is the reading at "zero" (the lower reference point)
			 * in the simplest case, we would use merely two points to correct the entire voltage
			 * range of the A/D; in such a simple case, the "zero" point would be a point near 0V,
			 * and the "span" point would be a point near the top of the voltage range, such as 9.9V;
			 * however, since this function is actually calculating a whole bunch of slope/offset
			 * correction factors, one between each pair of points, "zero" refers to the lower of
			 * two points, and "span" refers to the higher of the two points
			 */
			for( index = 1 /* yes, 1 */; index < numPoints; index++ ) {
				const double counts0 = AIOUSB_VoltsToCounts( DeviceIndex, 0 /* channel */
						, workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_VOLTS ] )
					, counts1 = AIOUSB_VoltsToCounts( DeviceIndex, 0 /* channel */
						, workingPoints[ index * WORKING_COLUMNS + COLUMN_VOLTS ] );
				const double slope
					= (
						workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ]
						- workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_COUNTS ]
					)
					/ ( counts1 - counts0 );
				const double offset
					= workingPoints[ ( index - 1 ) * WORKING_COLUMNS + COLUMN_COUNTS ]
					- ( counts0 * slope );
				if(
					slope >= 0.1
					&& slope <= 10.0
					&& offset >= -1000.0
					&& offset <= 1000.0
				) {
					workingPoints[ index * WORKING_COLUMNS + COLUMN_SLOPE ] = slope;
					workingPoints[ index * WORKING_COLUMNS + COLUMN_OFFSET ] = offset;
				} else {
#if defined( DEBUG_EXT_CAL )
					printf( "Error: slope of %0.3f or offset of %0.3f is outside the allowed limits\n", slope, offset );
#endif
					result = AIOUSB_ERROR_INVALID_DATA;		// slopes and offsets are way off, abort
					break;						// from for()
				}	// if( slope ...
			}	// for( index ...
		}	// if( result ...

		if( result == AIOUSB_SUCCESS ) {
#if defined( DEBUG_EXT_CAL )
			printf(
				"External Calibration Points\n"
				"     Input    Measured  Calculated  Calculated\n"
				"     Volts      Counts       Slope      Offset\n"
			);
			for( index = 0; index < numPoints; index++ ) {
				printf( "%10.3f  %10.3f  %10.3f  %10.3f\n"
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_VOLTS ]
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ]
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_SLOPE ]
					, workingPoints[ index * WORKING_COLUMNS + COLUMN_OFFSET ]
				);
			}	// for( index ...
#endif

			/*
			 * generate calibration table using the equation
			 *   ccounts = ( mcounts – offset ) / slope
			 * described above; each slope/offset pair in workingPoints[] describes the line
			 * segment running between the _previous_ point and the current one; in addition,
			 * the first row in workingPoints[] doesn't contain a valid slope/offset pair
			 * because there is no previous point before the first one (!), so we stretch the
			 * first line segment (between points 0 and 1) backward to the beginning of the A/D
			 * count range; similarly, since the highest calibration point is probably not right
			 * at the top of the A/D count range, we stretch the highest line segment (between
			 * points n-2 and n-1) up to the top of the A/D count range
			 */
			unsigned short *const calTable = ( unsigned short * ) malloc( CAL_TABLE_WORDS * sizeof( unsigned short ) );
			if( calTable != 0 ) {
				int measCounts = 0;				// stretch first line segment to bottom of A/D count range
				for( index = 1 /* yes, 1 */; index < numPoints; index++ ) {
					const double slope = workingPoints[ index * WORKING_COLUMNS + COLUMN_SLOPE ]
						, offset = workingPoints[ index * WORKING_COLUMNS + COLUMN_OFFSET ];
					const int maxSegmentCounts
						= ( index == ( numPoints - 1 ) )
							? ( CAL_TABLE_WORDS - 1 )	// stretch last line segment to top of A/D count range
							: ( int ) workingPoints[ index * WORKING_COLUMNS + COLUMN_COUNTS ];
					for( ; measCounts <= maxSegmentCounts; measCounts++ ) {
						int corrCounts = round( ( measCounts - offset ) / slope );
						if( corrCounts < 0 )
							corrCounts = 0;
						else if( corrCounts > AI_16_MAX_COUNTS )
							corrCounts = AI_16_MAX_COUNTS;
						calTable[ measCounts ] = corrCounts;
					}	// for( ; measCounts ...
				}	// for( index ...

				/*
				 * optionally return calibration table to caller
				 */
				if( returnCalTable != 0 )
					memcpy( returnCalTable, calTable, CAL_TABLE_WORDS * sizeof( unsigned short ) );

				/*
				 * optionally save calibration table to a file
				 */
				if( saveFileName != 0 ) {
					FILE *const calFile = fopen( saveFileName, "w" );
					if( calFile != NULL ) {
						const size_t wordsWritten = fwrite( calTable, sizeof( unsigned short ), CAL_TABLE_WORDS, calFile );
						fclose( calFile );
						if( wordsWritten != ( size_t ) CAL_TABLE_WORDS ) {
							remove( saveFileName );		// file is likely corrupt or incomplete
							result = AIOUSB_ERROR_FILE_NOT_FOUND;
						}	// if( wordsWritten ...
					} else
						result = AIOUSB_ERROR_FILE_NOT_FOUND;
				}	// if( saveFileName ...

				/*
				 * finally, send calibration table to device
				 */
				result = AIOUSB_ADC_SetCalTable( DeviceIndex, calTable );

				free( calTable );
			} else
				result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;
		}	// if( result ...

		free( workingPoints );
	} else
		result = AIOUSB_ERROR_NOT_ENOUGH_MEMORY;

	return result;
}	// AIOUSB_ADC_ExternalCal()




#ifdef __cplusplus
}	// namespace AIOUSB
#endif



/* end of file */
