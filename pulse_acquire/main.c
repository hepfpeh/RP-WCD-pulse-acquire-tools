/*
 * main.c
 * 
 * Pulse acquire utility for Red Pitaya.
 * 
 * 
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "oscilloscope.h"
#include "ini.h"
#include "pa.h"

volatile pa_flags_t pa_flags = { false, false };


// Called when we recieve SIGINT (CTRL+C)
// which then stops the infinite loop in main().

void inthand(int signum) 
{
   pa_flags.Running = false;
}


int main(int argc, char **argv)
{
    if(argc < 2){
        printf("\n  Usage: %s Config_file\n\n", argv[0]);
        return 1;
    }
    
    pa_config_t     *pa_config      = (pa_config_t *)       malloc( sizeof(pa_config_t)     );
    pa_run_info_t   *pa_run_info    = (pa_run_info_t *)     malloc( sizeof(pa_run_info_t)   );
    pa_timer_data_t *pa_timer_data  = (pa_timer_data_t *)   malloc( sizeof(pa_timer_data_t) );
    pa_log_file_t   *pa_log_file    = (pa_log_file_t *)     malloc( sizeof(pa_log_file_t)   );
    pa_data_file_t  *pa_data_file   = (pa_data_file_t *)    malloc( sizeof(pa_data_file_t)  );
    pa_logger_t     *pa_logger      = (pa_logger_t *)       malloc( sizeof(pa_logger_t)     );
    
    
    pa_InitVars( pa_config, pa_run_info, pa_timer_data, pa_log_file, pa_data_file, pa_logger );
    
    
    if (ini_parse(argv[1], pa_config_handler, pa_config) != 0) {
        printf("\nCan't load '%s' or has syntax errors\n\n",argv[1]);
        return 1;
    }
    
    if( pa_config->Parse_Errors ){
        printf("\nBad configuration file\n\n");
        return 1;
    }
    
    if( pa_CheckConfig( pa_config ) != 0 )
        return 1;
    
    strcpy( pa_config->Config_File_Name, argv[1] );
    
    pa_InitLogFile(  pa_log_file );
    
    char log_entry[200];
    sprintf(log_entry, "%s Configuration file loaded", argv[1]);
    pa_LogFileEntry( pa_log_file, log_entry );
    
    printf("\n|--------------------------- Pulse Acquire Tool ------------------------------|");
    printf("\n| Configuration values");
    printf("\n| ====================");
    printf("\n|   %-25s%i", "Pre_Trigger_Points:",      pa_config->Pre_Trigger_Points);
    printf("\n|   %-25s%i", "Pos_Trigger_Points:",      pa_config->Pos_Trigger_Points);
    printf("\n|   %-25s%i", "Trigger_Level:",           pa_config->Trigger_Level);
    printf("\n|   %-25s%i", "Trigger_Timeout_Secs:",    pa_config->Trigger_Timeout_Secs);
    printf("\n|   %-25s%i", "Capture_Time_Secs:",       pa_config->Capture_Time_Secs);
    printf("\n|   %-25s%s", "File_Name_Prefix:",        pa_config->File_Name_Prefix);
    printf("\n|   %-25s%s", "File_Header_Comment:",     pa_config->File_Header_Comment);
    printf("\n|   %-25s%i", "File_Time_Secs:",          pa_config->File_Time_Secs);
    printf("\n|");
    printf("\n| Press CTRL-C to stop");
    printf("\n|");
    printf("\n|-----------------------------------------------------------------------------|\n");
    
    
    
//    return 0;
    
    signal(SIGINT, inthand);
    
    const uint16_t BuffSize = pa_config->Pre_Trigger_Points + pa_config->Pos_Trigger_Points;
    
    uint16_t *PulseData = (uint16_t *)malloc( sizeof(uint16_t) * BuffSize );
    
    pa_data_file->p_size = BuffSize;
    
    
    pa_InitRP();
    pa_SettingsRP( pa_config );

    
    pa_LogFileEntry( pa_log_file, "Red Pitaya acquisition configured" );
    
    pa_flags.Running = true;
    
    pthread_t pa_Timer_thr_id;
    pthread_t pa_DisplayInfo_thr_id;
    pthread_t pa_Logger_thr_id;
    pthread_create(&pa_Timer_thr_id,        NULL,   pa_Timer_thr,       (void*)pa_timer_data);
    pthread_create(&pa_DisplayInfo_thr_id,  NULL,   pa_DisplayInfo_thr, (void*)pa_run_info);
    pthread_create(&pa_Logger_thr_id,       NULL,   pa_Logger_thr,      (void*)pa_logger);
    
    int c_error_count=0;
    float avg_rate = 0;
    
 //   uint16_t Trigger_timeout = 60;
    struct timespec LTClock, EClock;
    clock_gettime(CLOCK_REALTIME, &LTClock);
    
    pa_InitDataFile( pa_data_file );
    
    
    pa_LogFileEntry( pa_log_file, "Acquisition started" );
    
    while( pa_flags.Running )
    {
        
        /* Enabling writting data into memory */
        osc_WriteDataIntoMemory( true );

        
        /* Is necesary to wait until the buffer fills with
         * new samples.
         */
        
        //usleep( MAX( (2*PRE_TRIGGER_POINTS*0.008), 1 ) );
        usleep( MAX( (2*pa_config->Pre_Trigger_Points*0.008), 1 ) );

        /* Set trigger source
         * By some unknow reason, trigger source MUST
         * be set AFTER enabling write into memory 
         * (osc_WriteDataIntoMemory( true ). Otherwise
         * trigger condition is never raised.
         */ 
        osc_SetTriggerSource(3); // Channel A, negative slope.
        
        bool Triggerded = false;

        /* countout will detect if the trigger is never
         * raised.
         */ 
        volatile int countout = 100000;
        while(--countout){
            /* Checking if trigger condition is raised */
            osc_GetTriggerState( &Triggerded );
            if( Triggerded ){
                break;
            }
        }

        if( countout )
        {
             
            c_error_count = 0;
            
            /* When trigger is raised, some time is
             * needed to fill the buffer with the
             * number of samples specifided in osc_SetTriggerDelay
             */
            
            usleep( MAX( (2*pa_config->Pos_Trigger_Points*0.008), 1 ) );
            
            /* After capturing all the samples specifided
             * in osc_SetTriggerDelay, memory write stops
             * automaticaly. Then the data can be
             * retrived from buffer.
             * osc_GetWritePointerAtTrig gives the point
             * where the trigger condition was found. 
             */ 
            
            uint32_t TriggerPoint, StartPoint;
            osc_GetWritePointerAtTrig( &TriggerPoint );
 

            if( TriggerPoint >  pa_config->Pre_Trigger_Points )
                StartPoint = TriggerPoint - pa_config->Pre_Trigger_Points ;
            else
                StartPoint = ADC_BUFFER_SIZE - pa_config->Pre_Trigger_Points + TriggerPoint;

            const volatile uint32_t *buff = osc_GetDataBufferChA();
            
            for(uint32_t i = 0; i < BuffSize; ++i){
                PulseData[i] = ( buff[(StartPoint + i) % ADC_BUFFER_SIZE] ) & ADC_BITS_MAK;
            }
        

            pa_GetFileName( pa_data_file, pa_log_file );
            
            fwrite(PulseData, sizeof(uint16_t), BuffSize, pa_data_file->Output_File );
 
            fflush( pa_data_file->Output_File );
            
            pa_data_file->cf_pulses++;
            
            pa_run_info->n_pulses++;
            
            clock_gettime(CLOCK_REALTIME, &LTClock);
        }
        else
        {
            /* If countout reaches 0, means that trigger
             * was not raised.
            */ 
        
            pa_run_info->t_errors++;
            c_error_count++;
            if( (c_error_count % 35) == 0 ){
                clock_gettime(CLOCK_REALTIME, &EClock);
                uint16_t cttime = EClock.tv_sec - LTClock.tv_sec;
                if( cttime > pa_config->Trigger_Timeout_Secs)
                {
                    printf("\nTrigger timeout. Aborting...\n");
                    pa_LogFileEntry( pa_log_file, "Trigger timeout: Stopping" );
                    pa_flags.Running = false;
                    break;
                }
            }
        }
    }
    
    
    pa_LogFileEntry( pa_log_file, "Acquisition stopped" );
    
    pa_CloseDataFile( pa_data_file, pa_log_file );
    
    
    /* Final inform */
    
    avg_rate = (float) pa_run_info->n_pulses / (float)(*pa_run_info->Elapsed_Time_ptr);
    printf("\n|---------------------------------- TOTALS -----------------------------------|");
    printf("\n| Elapsed time:\t%11i s\n| Pulse count:\t%11" PRIu64 " \n| Average rate:\t%11.2f Hz\n| Files writed:\t%11i", *pa_run_info->Elapsed_Time_ptr, pa_run_info->n_pulses, avg_rate, *pa_run_info->File_Number_ptr);
    printf("\n|-----------------------------------------------------------------------------|\n");
    
    
    sprintf(log_entry, "Elapsed time: %7i s; Pulse count: %11" PRIu64 "; Average rate: %5.2f Hz; Files writed: %7i", *pa_run_info->Elapsed_Time_ptr, pa_run_info->n_pulses, avg_rate, *pa_run_info->File_Number_ptr);
    pa_LogFileEntry( pa_log_file, log_entry );
    
    /* Joining threads */
    
    pthread_join(   pa_Logger_thr_id,      NULL);
    pthread_join(   pa_DisplayInfo_thr_id, NULL);
    pthread_join(   pa_Timer_thr_id,       NULL);

    
    /* Releasing RP */
    
    pa_StopRP();
    
    /* Closing log file */
    
    pa_CloseLogFile( pa_log_file );
    
    /* Releasing resources */
    
    free(PulseData);
    free(pa_config);
    free(pa_run_info);
    free(pa_timer_data);
    free(pa_log_file);
    free(pa_data_file);
    free(pa_logger);
    
    return 0;
}

