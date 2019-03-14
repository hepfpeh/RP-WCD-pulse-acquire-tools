/* Red Pitaya C API example Acquiring a signal from a buffer  
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include "common.h"
#include "oscilloscope.h"
#include "ini.h"
#include "version.h"
#include "build.h"

/* configuration file */

typedef struct pa_config_s
{
    uint32_t    Pre_Trigger_Points;
    uint32_t    Pos_Trigger_Points;
    int32_t     Trigger_Level;
    uint16_t    Trigger_Timeout_Secs;
    uint32_t    Capture_Time_Secs;
    char        File_Name_Prefix[20];
    char        File_Header_Comment[128];
    uint32_t    File_Time_Secs;
    bool        Parse_errors;
} pa_config_t;


/* ADC acquisition bits mask. */
static const int ADC_BITS_MAK = 0x3FFF;

#define PRE_TRIGGER_POINTS  5
#define POST_TRIGGER_POINTS 27
#define CAPTURING_TIME_SEC  10
//#define DATA_POINTS 64
//#define DATA_POINTS_2 (DATA_POINTS/2)


typedef struct pa_flags_s
{
    bool Running;
    bool ShowInfo; //Actualy unused
} pa_flags_t;

//volatile sig_atomic_t stop;




typedef struct pa_run_info_s 
{
    uint64_t n_pulses;
    uint32_t t_errors;
    uint32_t e_time;
    uint32_t n_file;
} pa_run_info_t;


typedef struct pa_timer_data_s
{
    uint32_t *T_time; // Pointer to capture time
    uint32_t *E_time; // Pointer to elapsed time
} pa_timer_data_t;


typedef struct pa_file_s
{
    FILE *Output_File;
    uint32_t *e_time_ptr;
    uint32_t *File_Time_Secs_ptr;
    uint32_t *n_file_ptr;
    char     *File_Name_Prefix_ptr;
    char     *File_Header_Comment_ptr;
    int32_t  *Trigger_Level_ptr;
    time_t   i_time;
    uint32_t p_size;
    uint32_t cf_pulses;
} pa_file_t;

/* Program flags must be globals */
// This must be of type `volatile` to prevent
// the compiler from optimizing away the
// while loop condition.

volatile pa_flags_t  pa_flags;


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
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Pos_Trigger_Points")) {
        int tmp = atoi(value);
        if( (tmp > 0) && (tmp < ADC_BUFFER_SIZE) )
            pconfig->Pos_Trigger_Points = tmp;
        else
        {
            printf("\nInvalid \"Pos_Trigger_Points\" value in config file (%i): Must be between 0 and %i\n", tmp, ADC_BUFFER_SIZE);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Trigger_Level")) {
        int tmp = atoi(value);
        if( (tmp > -8193) && (tmp < 8192) )
            pconfig->Trigger_Level = tmp;
        else
        {
            printf("\nInvalid \"Trigger_Level\" value in config file (%i): Must be between -8192 and +8191\n",tmp);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Trigger_Timeout_Secs")) {
        int tmp = atoi(value);
        if( tmp > 0 )
            pconfig->Trigger_Timeout_Secs = tmp;
        else
        {
            printf("\nInvalid \"Trigger_Timeout_Secs\" value in config file (%i): Must be greater than zero\n", tmp);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("Capture", "Capture_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->Capture_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"Capture_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("File", "File_Name_Prefix")) {
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
                strcpy( pconfig->File_Name_Prefix, value );
            else
            {
                printf("\nInvalid \"File_Name_Prefix\": File prefix must have less than 20 characters.\n");
                pconfig->Parse_errors = true;
            }
        else
        {
            printf("\nInvalid \"File_Name_Prefix\": Ivalid character(s) for a file name used.\n");
            pconfig->Parse_errors = true;
        }
    } else if (MATCH("File", "File_Header_Comment")) {
        if( strlen(value) < 121 )
            strcpy( pconfig->File_Header_Comment, value );
        else
        {
            printf("\nInvalid \"File_Header_Comment\": File prefix must have less than 120 characters.\n");
            pconfig->Parse_errors = true;
        }
        
    } else if (MATCH("File", "File_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->File_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"File_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
            pconfig->Parse_errors = true;
        }
    } else {
        pconfig->Parse_errors = true;
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

// Called when we recieve SIGINT (CTRL+C)
// which then stops the infinite loop in main().

void inthand(int signum) 
{
   pa_flags.Running = false;
}

// timer
volatile bool ShowInfo = false;


void *Timer( void *targs )
{
    pa_timer_data_t *timer_data = (pa_timer_data_t*)targs;
    struct timespec OneSec, Start_clock, Current_clock;
    OneSec.tv_sec=1;
    OneSec.tv_nsec=0;
    clock_gettime(CLOCK_REALTIME, &Start_clock);
    while( pa_flags.Running )
    {
        clock_nanosleep(CLOCK_REALTIME, 0, &OneSec, NULL);
        clock_gettime(CLOCK_REALTIME, &Current_clock);
        *timer_data->E_time = Current_clock.tv_sec - Start_clock.tv_sec;
        if( *timer_data->T_time > 0 )
            if( *timer_data->E_time > ( *timer_data->T_time - 1 ) ) pa_flags.Running = false;
    }
}


void *pa_DisplayInfo( void *targs)
{
    pa_run_info_t *pa_run_info = (pa_run_info_t*)targs;
    
    uint32_t rate = 0;
    uint64_t l_count = 0;
    sleep(1);
    while( pa_flags.Running )
    {
        rate = (uint16_t)(pa_run_info->n_pulses - l_count);
        l_count = pa_run_info->n_pulses;
        printf("\r| ET:%7i s | PC:%11" PRIu64 " | R:%5i Hz | TEC:%7i | FN:%7i |", pa_run_info->e_time, pa_run_info->n_pulses, rate, pa_run_info->t_errors, pa_run_info->n_file);
        fflush(stdout);
        sleep(1);
    }
    
}


int pa_SetDefaults( pa_config_t *config, volatile pa_flags_t *flags, pa_run_info_t *info, pa_timer_data_t *timer_data, pa_file_t *file )
{
    config->Pre_Trigger_Points   = 5;
    config->Pos_Trigger_Points   = 27;
    config->Trigger_Level        = -819;
    config->Trigger_Timeout_Secs = 5;
    config->Capture_Time_Secs    = 1;
    strcpy( config->File_Name_Prefix, "pa_out-");
    strcpy( config->File_Header_Comment, "(None)");
    config->File_Time_Secs       = 60;
    config->Parse_errors         = false;
    
    flags->Running  = false;
    flags->ShowInfo = false;
    
    info->n_pulses = 0;
    info->t_errors = 0;
    info->e_time   = 0;
    info->n_file   = 0;
    
    timer_data->T_time = &config->Capture_Time_Secs;
    timer_data->E_time = &info->e_time;
    
    file->Output_File        = NULL;
    file->e_time_ptr         = &info->e_time;
    file->File_Time_Secs_ptr = &config->File_Time_Secs;
    file->File_Name_Prefix_ptr       = config->File_Name_Prefix;
    file->File_Header_Comment_ptr    = config->File_Header_Comment;
    file->n_file_ptr         = &info->n_file;
    file->Trigger_Level_ptr  = &config->Trigger_Level;
    file->i_time             = 0;
    file->p_size             = 0;
    file->cf_pulses          = 0;

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

int pa_settings( pa_config_t *config )
{
    /* Setting parameters */

    uint32_t TL=0;
    
    if( config->Trigger_Level >= 0 )
        TL = config->Trigger_Level;
    else
        TL = 16384 + config->Trigger_Level;
    
    /* Threshold level for trigger in channel A in ADC counts.
     * 
     * Notice that a value of 0 is in the "middle" of ADC range,
     * i.e. O V in input. For LV gain +1 V is 8191. Negative values
     * are expressed as 2-complement to 2^14, thus -1 V is 8192
    */
    osc_SetThresholdChA( TL );
    osc_SetThresholdChB(0);
    
    /* Hysterisis in threshold for channel A in ADC counts*/
    osc_SetHysteresisChA(0);
    /* Hysterisis in threshold for channel B in ADC counts*/
    osc_SetHysteresisChB(0);
    
    /* Set decimation value */
    osc_SetDecimation(1); // 125 msps
    
    /* Set trigger delay in decimated units */
    osc_SetTriggerDelay( MIN( 3*config->Pos_Trigger_Points, ADC_BUFFER_SIZE )  ); 
    
    return 0;
}


int pa_InitDataFile( pa_file_t *file )
{
    if( (file->Output_File) == NULL )
    {
        char DateTime[15];
        file->i_time = time(NULL);
        struct tm *t = localtime(&file->i_time);
        strftime(DateTime, sizeof(DateTime)-1, "%d%m%y-%H%M%S", t);
    
        char FileName[40];
        strcpy(FileName, file->File_Name_Prefix_ptr);
        strcat(FileName, "-");
        strcat(FileName, DateTime);
        strcat(FileName, ".paa");
    
        file->Output_File = fopen(FileName, "wb");
        
        if( (file->Output_File) == NULL )
        {
            printf("\nError: Can't open file for output\n");
            exit(0);
        }
        
        fseek( file->Output_File, 640, SEEK_SET);
        
        file->cf_pulses = 0;
        
        (*file->n_file_ptr)++;
        
    }
    
    return 0;
}

int pa_CloseDataFile( pa_file_t *file )
{
    if( (file->Output_File) != NULL )
    {
        /*
         * Structure for PAA file
         * 
         * Offset | Size in bytes | Descrtiption
         * --------------------------------------
         * 0      | 8             | File identifier: Must be 'PAA 01\n' (with end line \n) 
         *        |               | for this file structure.
         *        |               | Change if the file structure is modified.
         * --------------------------------------
         * 8      | 512           | Text headers: 5 lines
         *        |               | - Program description
         *        |               | - version
         *        |               | - build
         *        |               | - date of file creation
         *        |               | - comentaries form config file.
         * --------------------------------------
         * 520    | 4             | Endianness check number: Is an unsinged int of 32 bit with value 0x10203040
         *---------------------------------------
         * 524    | 4             | Number of points per pulse (PS).  Unsinged int of 32 bit.
         *---------------------------------------
         * 528    | 4             | Number of pulses in recorded in this file (PC). Unsinged int of 32 bit.
         * --------------------------------------
         * 532    | 4             | Threshold level used to trigger as specified in config file. Singed int of 32 bit.
         * --------------------------------------
         * 640    | 2*PS*PC       | Pulse data.
         * 
         */
        
        char itDateTime[30];
        struct tm *it = localtime(&file->i_time);
        strftime(itDateTime, sizeof(itDateTime)-1, "%c %Z", it);
        
        fseek( file->Output_File, 0, SEEK_SET);
        
        fprintf(file->Output_File, "PAA 01\n");                                            //   7 bytes
                                                                
        
        fseek( file->Output_File, 8, SEEK_SET);
        
        fprintf(file->Output_File, "Pulse Acquire tool for Red Pitaya\n");                //  34 bytes
        fprintf(file->Output_File, "Version: %d.%d.%d\n", _VERSION_MAJOR, _VERSION_MINOR, _VERSION_PATCH);
                                                                                          //  19 bytes
        fprintf(file->Output_File, "Build: %s\n", _BUILD);
                                                                                          // 128 bytes
        fprintf(file->Output_File, "%s\n",itDateTime);                                    //  29 bytes
        fprintf(file->Output_File, "%s\n",file->File_Header_Comment_ptr);                 // 129 bytes
                                                                                          
        fseek( file->Output_File, 520, SEEK_SET);
        
        uint32_t eci = 0x10203040;
        fwrite(&eci, sizeof(uint32_t), 1, file->Output_File );                           //    4 bytes
        fwrite(&file->p_size, sizeof(uint32_t), 1, file->Output_File );                  //    4 bytes
        fwrite(&file->cf_pulses, sizeof(uint32_t), 1, file->Output_File );               //    4 bytes
        fwrite(file->Trigger_Level_ptr, sizeof(int32_t), 1, file->Output_File );        //    4 bytes
                                                                           
        fflush( file->Output_File );
        fclose( file->Output_File );
        
        file->Output_File = NULL;
    }
    
    return 0;
}


int pa_GetFileName( pa_file_t *file )
{
    if( *file->e_time_ptr > (*file->n_file_ptr)*(*file->File_Time_Secs_ptr) )
    {
        pa_CloseDataFile( file );
        pa_InitDataFile( file );
    }
    
    return 0;
}


int main(int argc, char **argv)
{
    if(argc < 2){
        printf("\n  Usage: %s Config_file\n\n", argv[0]);
        return 1;
    }
    
    pa_config_t     pa_config;
    pa_run_info_t   pa_run_info;
    pa_timer_data_t pa_timer_data;
    pa_file_t       pa_file;
    
    
    pa_SetDefaults( &pa_config, &pa_flags, &pa_run_info, &pa_timer_data, &pa_file );
    
    

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
    
    
    printf("\n|--------------------------- Pulse Acquire Tool ------------------------------|");
    printf("\n| Configuration values");
    printf("\n| ====================");
    printf("\n|   %-25s%i", "Pre_Trigger_Points:", pa_config.Pre_Trigger_Points);
    printf("\n|   %-25s%i", "Pos_Trigger_Points:", pa_config.Pos_Trigger_Points);
    printf("\n|   %-25s%i", "Trigger_Level:", pa_config.Trigger_Level);
    printf("\n|   %-25s%i", "Trigger_Timeout_Secs:", pa_config.Trigger_Timeout_Secs);
    printf("\n|   %-25s%i", "Capture_Time_Secs:", pa_config.Capture_Time_Secs);
    printf("\n|   %-25s%s", "File_Name_Prefix:", pa_config.File_Name_Prefix);
    printf("\n|   %-25s%s", "File_Header_Comment:", pa_config.File_Header_Comment);
    printf("\n|   %-25s%i", "File_Time_Secs:", pa_config.File_Time_Secs);
    printf("\n|");
    printf("\n| Press CTRL-C to stop");
    printf("\n|");
    printf("\n|-----------------------------------------------------------------------------|\n");
    
    
    
//    return 0;
    
    signal(SIGINT, inthand);
    
    const uint16_t BuffSize = pa_config.Pre_Trigger_Points + pa_config.Pos_Trigger_Points;
    
    uint16_t *PulseData = (uint16_t *)malloc( sizeof(uint16_t) * BuffSize );
    
    pa_file.p_size = BuffSize;
    
    
    pa_init();
    pa_settings( &pa_config );

    
    
    pa_flags.Running = true;
    
    pthread_t Timer_t_id, pa_DisplayInfo_t_id;
    pthread_create(&Timer_t_id, NULL, Timer, (void*)&pa_timer_data);
    pthread_create(&pa_DisplayInfo_t_id, NULL, pa_DisplayInfo, (void*)&pa_run_info);
    
    int c_error_count=0;
    float avg_rate = 0;
    
 //   uint16_t Trigger_timeout = 60;
    struct timespec LTClock, EClock;
    clock_gettime(CLOCK_REALTIME, &LTClock);
    
    pa_InitDataFile( &pa_file );
    
//     while( !stop )
    while( pa_flags.Running )
    {
        
        /* Enabling writting data into memory */
        osc_WriteDataIntoMemory( true );

        
        /* Is necesary to wait until the buffer fills with
         * new samples.
         */
        
        //usleep( MAX( (2*PRE_TRIGGER_POINTS*0.008), 1 ) );
        usleep( MAX( (2*pa_config.Pre_Trigger_Points*0.008), 1 ) );

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
            
            usleep( MAX( (2*pa_config.Pos_Trigger_Points*0.008), 1 ) );
            
            /* After capturing all the samples specifided
             * in osc_SetTriggerDelay, memory write stops
             * automaticaly. Then the data can be
             * retrived from buffer.
             * osc_GetWritePointerAtTrig gives the point
             * where the trigger condition was found. 
             */ 
            
            uint32_t TriggerPoint, StartPoint;
            osc_GetWritePointerAtTrig( &TriggerPoint );
 

            if( TriggerPoint >  pa_config.Pre_Trigger_Points )
                StartPoint = TriggerPoint - pa_config.Pre_Trigger_Points ;
            else
                StartPoint = ADC_BUFFER_SIZE - pa_config.Pre_Trigger_Points + TriggerPoint;

            const volatile uint32_t *buff = osc_GetDataBufferChA();
            
            for(uint32_t i = 0; i < BuffSize; ++i){
                PulseData[i] = ( buff[(StartPoint + i) % ADC_BUFFER_SIZE] ) & ADC_BITS_MAK;
            }
        

            pa_GetFileName( &pa_file );
            
            fwrite(PulseData, sizeof(uint16_t), BuffSize, pa_file.Output_File );
 
            fflush( pa_file.Output_File );
            
            pa_file.cf_pulses++;
            
            pa_run_info.n_pulses++;
            
            clock_gettime(CLOCK_REALTIME, &LTClock);
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
            if( (c_error_count % 35) == 0 ){
                clock_gettime(CLOCK_REALTIME, &EClock);
                uint16_t cttime = EClock.tv_sec - LTClock.tv_sec;
                if( cttime > pa_config.Trigger_Timeout_Secs)
                {
                    printf("\nTrigger timeout. Aborting...\n");
                    pa_flags.Running = false;
                    break;
                }
            }
        }
    }
    
    /* Releasing resources */
    pa_stop();
    pa_CloseDataFile( &pa_file );
    free(PulseData);
    pthread_join(pa_DisplayInfo_t_id, NULL);
    pthread_join(Timer_t_id, NULL);
    avg_rate = (float) pa_run_info.n_pulses / (float)pa_run_info.e_time;
    printf("\n|---------------------------------- TOTALS -----------------------------------|");
    printf("\n| Elapsed time:\t\t%7i s\n| Pulse count:\t\t%7i \n| Average rate:\t\t%5.2f Hz\n| Trigger error count:\t%7i\n| Files writed:\t\t%7i", pa_run_info.e_time, pa_run_info.n_pulses, avg_rate, pa_run_info.t_errors, pa_run_info.n_file);
    printf("\n|-----------------------------------------------------------------------------|\n");
    return 0;
}

