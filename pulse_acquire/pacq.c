/* Red Pitaya C API example Acquiring a signal from a buffer  
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "common.h"
#include "oscilloscope.h"
#include "ini.h"

/* configuration file */

typedef struct pa_config_s
{
    uint32_t    Pre_Trigger_Points;
    uint32_t    Pos_Trigger_Points;
    uint32_t    Capture_time_secs;
    const char* File_Prefix;
    uint32_t    Pulses_Per_File;
} pa_config_t;

static int pa_config_handler(void* user, const char* section, const char* name,
                   const char* value)
{
    pa_config_t* pconfig = (pa_config_t*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("Capture", "Pre_Trigger_Points")) {
        pconfig->Pre_Trigger_Points = atoi(value);
    } else if (MATCH("Capture", "Pos_Trigger_Points")) {
        pconfig->Pos_Trigger_Points = atoi(value);
    } else if (MATCH("Capture", "Capture_time_secs")) {
        pconfig->Capture_time_secs = atoi(value);
    } else if (MATCH("File", "File_Prefix")) {
        pconfig->File_Prefix = strdup(value);
    } else if (MATCH("File", "Pulses_Per_File")) {
        pconfig->Pulses_Per_File = atoi(value);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

/* ADC acquisition bits mask. */
static const int ADC_BITS_MAK = 0x3FFF;

#define PRE_TRIGGER_POINTS  5
#define POST_TRIGGER_POINTS 27
#define CAPTURING_TIME_SEC  10
//#define DATA_POINTS 64
//#define DATA_POINTS_2 (DATA_POINTS/2)

// This must be of type `volatile` to prevent
// the compiler from optimizing away the
// while loop condition.
volatile sig_atomic_t stop;

// Called when we recieve SIGINT (CTRL+C)
// which then stops the infinite loop in main().
void inthand(int signum) 
{
   stop=1;
}



typedef struct pa_info_s 
{
    uint32_t n_pulses;
    uint32_t t_errors;
    uint32_t e_time;
} pa_info_t;

// timer
volatile bool ShowInfo = false;

void *Timer( void *targs )
{
    pa_info_t *pa_info = (pa_info_t*)targs;
    printf("\nTimer starting\n");
    struct timespec Start_clock, Current_clock;
    clock_gettime(CLOCK_REALTIME, &Start_clock);
    double elapsed_seconds;
    while(!stop)
    {
        sleep(1);
        ShowInfo = true;
        clock_gettime(CLOCK_REALTIME, &Current_clock);
        pa_info->e_time = Current_clock.tv_sec - Start_clock.tv_sec;
        if( pa_info->e_time > ( CAPTURING_TIME_SEC - 1 ) ) stop = 1;
    }
    printf("\nTimer stopping\n");
}

int pa_init()
{
    /* Init procedures */
    cmn_Init();
    osc_Init();
    osc_SetAveraging(true);
    osc_ResetWriteStateMachine();
    return 0;
}

int pa_stop()
{
    osc_Release();
    cmn_Release();
    return 0;
}

int pa_settings()
{
    /* Setting parameters */

    
    /* Threshold level for trigger in channel A in ADC counts.
     * 
     * Notice that a value of 0 is in the "middle" of ADC range,
     * i.e. O V in input. For LV gain +1 V is 8191. Negative values
     * are expressed as 2-complement to 2^14, thus -1 V is 8192
    */
    osc_SetThresholdChA(15565); // 819 ADC counts below "0 level" (aprox. -0.1 V)
    osc_SetThresholdChB(0);
    
    /* Hysterisis in threshold for channel A in ADC counts*/
    osc_SetHysteresisChA(0);
    /* Hysterisis in threshold for channel B in ADC counts*/
    osc_SetHysteresisChB(0);
    
    /* Set decimation value */
    osc_SetDecimation(1); // 125 msps
    
    /* Set trigger delay in decimated units */
    osc_SetTriggerDelay( 2 * POST_TRIGGER_POINTS  ); 
    
    return 0;
}



int main(int argc, char **argv)
{
    pa_config_t pa_config;

    if (ini_parse("test.ini", pa_config_handler, &pa_config) < 0) {
        printf("Can't load 'test.ini'\n");
        return 1;
    }
    printf("Config loaded from 'test.ini':\nPre_Trigger_Points=%i\nPos_Trigger_Points=%i\nCapture_time_secs=%i\nFile_Prefix=%s\nPulses_Per_File=%i\n",
        pa_config.Pre_Trigger_Points, pa_config.Pos_Trigger_Points, pa_config.Capture_time_secs,pa_config.File_Prefix,pa_config.Pulses_Per_File);
    return 0;
    
    signal(SIGINT, inthand);
    
    const uint16_t BuffSize = PRE_TRIGGER_POINTS + POST_TRIGGER_POINTS;

    uint16_t *PulseData = (uint16_t *)malloc( sizeof(uint16_t) * BuffSize );
    
    FILE *output_file;
    output_file = fopen("test.pad", "wb");
    
    pa_init();
    pa_settings();

    struct pa_info_s pa_run_info;
    pa_run_info.n_pulses = 0;
    pa_run_info.t_errors = 0;
    pa_run_info.e_time = 0;
    
    pthread_t timer_t_id;
    pthread_create(&timer_t_id, NULL, Timer, (void*)&pa_run_info);
    
    int c_error_count=0;
    uint32_t rate = 0;
    float avg_rate = 0;
    uint32_t l_count = 0;
    
    while( !stop )
    {
        
        /* Enabling writting data into memory */
        osc_WriteDataIntoMemory( true );

        
        /* Is necesary to wait until the buffer fills with
         * new samples.
         */
        
        usleep( MAX( (2*PRE_TRIGGER_POINTS*0.008), 1 ) );

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
            
            usleep( MAX( (2*POST_TRIGGER_POINTS*0.008), 1 ) );
            
            /* After capturing all the samples specifided
             * in osc_SetTriggerDelay, memory write stops
             * automaticaly. Then the data can be
             * retrived from buffer.
             * osc_GetWritePointerAtTrig gives the point
             * where the trigger condition was found. 
             */ 
            
            uint32_t TriggerPoint, StartPoint;
            osc_GetWritePointerAtTrig( &TriggerPoint );
 
            if( TriggerPoint >  PRE_TRIGGER_POINTS )
                StartPoint = TriggerPoint - PRE_TRIGGER_POINTS ;
            else
                StartPoint = ADC_BUFFER_SIZE - PRE_TRIGGER_POINTS + TriggerPoint;

            const volatile uint32_t *buff = osc_GetDataBufferChA();
            
            for(uint32_t i = 0; i < BuffSize; ++i){
                PulseData[i] = ( buff[(StartPoint + i) % ADC_BUFFER_SIZE] ) & ADC_BITS_MAK;
            }
        
            fwrite(PulseData, sizeof(uint16_t), BuffSize, output_file);
 
            fflush(output_file);
            
            pa_run_info.n_pulses++;
        }
        else
        {
            /* If countout reaches 0, means that trigger
             * was not raised. If this happens 5 times continuosly,
             * acquisition conditons are reset. If
             * happens 10 times, program execution is
             * aborted.
            */ 
        
            pa_run_info.t_errors++;
            c_error_count++;
            if(c_error_count == 5){
                printf("\nReseting acquisition parameters:\n");
                osc_WriteDataIntoMemory( false );
                osc_ResetWriteStateMachine();
                pa_settings();
            }
            if(c_error_count == 10){
                printf("Aborting...\n");
                break;
            }
        }
         if( ShowInfo && !stop ){
             rate = pa_run_info.n_pulses - l_count;
             l_count = pa_run_info.n_pulses;
             printf("\r| Elapsed time:\t%7i s | Pulse count:\t%7i | Rate:\t\t%4i Hz | Trigger error count:\t%7i |", pa_run_info.e_time, pa_run_info.n_pulses, rate, pa_run_info.t_errors);
             fflush(stdout);
             ShowInfo = false;
         }
    }
    
    /* Releasing resources */
    pa_stop();
    fclose(output_file);
    free(PulseData);
    pthread_join(timer_t_id, NULL);
    avg_rate = (float) pa_run_info.n_pulses / (float)pa_run_info.e_time;
    printf("\n|--------------------------------------- TOTALS ----------------------------------------|");
    printf("\n| Elapsed time:\t\t%7i s\n| Pulse count:\t\t%7i \n| Average rate:\t\t%5.2f Hz\n| Trigger error count:\t%7i", pa_run_info.e_time, pa_run_info.n_pulses, avg_rate, pa_run_info.t_errors);
    printf("\n|---------------------------------------------------------------------------------------|\n");
    return 0;
}

