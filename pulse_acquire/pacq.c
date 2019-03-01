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
    int32_t     Trigger_Level;
    uint32_t    Capture_Time_Secs;
    char        File_Prefix[20];
    uint32_t    File_Time_Secs;
    bool        Parse_errors;
} pa_config_t;

static int pa_config_handler(void* user, const char* section, const char* name,
                   const char* value)
{
    pa_config_t* pconfig = (pa_config_t*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("Capture", "Pre_Trigger_Points")) {
        int tmp = atoi(value);
        if( (tmp > 0) && (tmp < ADC_BUFFER_SIZE) )
            pconfig->Pre_Trigger_Points = tmp;
        else
        {
            printf("\nInvalid \"Pre_Trigger_Points\" value in config file (%i): Must be between 0 and %i\n", tmp, ADC_BUFFER_SIZE);
//             printf("\tKeeping default (%i).\n", pconfig->Pre_Trigger_Points);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Pos_Trigger_Points")) {
        int tmp = atoi(value);
        if( (tmp > 0) && (tmp < ADC_BUFFER_SIZE) )
            pconfig->Pos_Trigger_Points = tmp;
        else
        {
            printf("\nInvalid \"Pos_Trigger_Points\" value in config file (%i): Must be between 0 and %i\n", tmp, ADC_BUFFER_SIZE);
//             printf("\tKeeping default (%i).\n", pconfig->Pos_Trigger_Points);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Trigger_Level")) {
        int tmp = atoi(value);
        if( (tmp > -8193) && (tmp < 8192) )
            pconfig->Trigger_Level = tmp;
        else
        {
            printf("\nInvalid \"Trigger_Level\" value in config file (%i): Must be between -8192 and +8191\n",tmp);
//             printf("\tKeeping default (%i).\n", pconfig->Trigger_Level);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Capture_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->Capture_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"Capture_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
//             printf("\tKeeping default (%i).\n", pconfig->Capture_Time_Secs);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("File", "File_Prefix")) {
        //char bad_chars[] = "!@%^*~|";
        char bad_chars[] = "/\0";
        bool invalid_found = false;
        int i;
        for (i = 0; i < strlen(bad_chars); ++i) {
            if (strchr(value, bad_chars[i]) != NULL) {
                invalid_found = true;
                break;
            }
        }
        if (!invalid_found)
            if( strlen(value) < 20 )
                strcpy( pconfig->File_Prefix, value );
            else
            {
                printf("\nInvalid \"File_Prefix\": File prefix must have less than 20 characters.\n");
//                 printf("\tKeeping default (%s).\n", pconfig->File_Prefix);
                pconfig->Parse_errors = true;
            }
        else
        {
            printf("\nInvalid \"File_Prefix\": Ivalid character(s) for a file name used.\n");
//             printf("\tKeeping default (%s).\n", pconfig->File_Prefix);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("File", "File_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->File_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"File_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
//             printf("\tKeeping default (%i).\n", pconfig->File_Time_Secs);
            pconfig->Parse_errors = true;
        }
    } else {
        pconfig->Parse_errors = true;
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

typedef struct pa_flags_s
{
    bool Stop;
    bool ShowInfo;
} pa_flags_t;

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

int pa_SetDefaults( pa_config_t *config, volatile pa_flags_t *flags, pa_info_t *info )
{
    config->Pre_Trigger_Points = 5;
    config->Pos_Trigger_Points = 27;
    config->Trigger_Level      = -819;
    config->Capture_Time_Secs  = 1;
    strcpy( config->File_Prefix, "pa_out-");
    config->File_Time_Secs     = 60;
    config->Parse_errors       = false;
    
    flags->Stop     = false;
    flags->ShowInfo = false;
    
    info->n_pulses = 0;
    info->t_errors = 0;
    info->e_time   = 0;

    return 0;
}

int pa_CheckConfig( pa_config_t *config )
{
    if( ( config->Pre_Trigger_Points + config->Pos_Trigger_Points )  > ADC_BUFFER_SIZE ){
        printf("\nError: ( Pre_Trigger_Points + Pos_Trigger_Points ) must be less than %i\n\n", ADC_BUFFER_SIZE);
        return 1;
    }
    return 0;
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
    if(argc < 2){
        printf("\n  Usage: %s Config_file\n\n", argv[0]);
        return 1;
    }
    
    pa_config_t pa_config;
    volatile pa_flags_t  pa_flags;
    pa_info_t   pa_info;
    
    pa_SetDefaults( &pa_config, &pa_flags, &pa_info );

    if (ini_parse(argv[1], pa_config_handler, &pa_config) != 0) {
        printf("\nCan't load '%s' or has syntax errors\n\n",argv[1]);
        return 1;
    }
    
    
    if( pa_config.Parse_errors ){
        printf("\nBad configuration file\n\n");
        return 1;
    }
    
    if( pa_CheckConfig( &pa_config ) != 0 )
        return 1;
    
    
    printf("\n|-------------------------------- Pulse Acquire Tool -----------------------------------|");
    printf("\n| Configuration values");
    printf("\n| ====================");
    printf("\n|   %-25s%i", "Pre_Trigger_Points:", pa_config.Pre_Trigger_Points);
    printf("\n|   %-25s%i", "Pos_Trigger_Points:", pa_config.Pos_Trigger_Points);
    printf("\n|   %-25s%i", "Trigger_Level:", pa_config.Trigger_Level);
    printf("\n|   %-25s%i", "Capture_Time_Secs:", pa_config.Capture_Time_Secs);
    printf("\n|   %-25s%s", "File_Prefix:", pa_config.File_Prefix);
    printf("\n|   %-25s%i", "File_Time_Secs:", pa_config.File_Time_Secs);
    printf("\n|---------------------------------------------------------------------------------------|\n");
    
    
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

