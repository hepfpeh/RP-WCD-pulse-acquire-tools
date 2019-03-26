/*
 * pa.c
 * 
 * Pulse acquire utility for Red Pitaya.
 * 
 * 
 */

#include <stdio.h>
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

#include "pa.h"


int pa_config_handler(void* user, const char* section, const char* name, const char* value)
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
            pconfig->Parse_Errors = true;
        }
    } else if (MATCH("Capture", "Pos_Trigger_Points")) {
        int tmp = atoi(value);
        if( (tmp > 0) && (tmp < ADC_BUFFER_SIZE) )
            pconfig->Pos_Trigger_Points = tmp;
        else
        {
            printf("\nInvalid \"Pos_Trigger_Points\" value in config file (%i): Must be between 0 and %i\n", tmp, ADC_BUFFER_SIZE);
            pconfig->Parse_Errors = true;
        }
    } else if (MATCH("Capture", "Trigger_Level")) {
        int tmp = atoi(value);
        if( (tmp > -8193) && (tmp < 8192) )
            pconfig->Trigger_Level = tmp;
        else
        {
            printf("\nInvalid \"Trigger_Level\" value in config file (%i): Must be between -8192 and +8191\n",tmp);
            pconfig->Parse_Errors = true;
        }
    } else if (MATCH("Capture", "Trigger_Timeout_Secs")) {
        int tmp = atoi(value);
        if( tmp > 0 )
            pconfig->Trigger_Timeout_Secs = tmp;
        else
        {
            printf("\nInvalid \"Trigger_Timeout_Secs\" value in config file (%i): Must be greater than zero\n", tmp);
            pconfig->Parse_Errors = true;
        }
    } else if (MATCH("Capture", "Capture_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->Capture_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"Capture_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
            pconfig->Parse_Errors = true;
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
                pconfig->Parse_Errors = true;
            }
        else
        {
            printf("\nInvalid \"File_Name_Prefix\": Ivalid character(s) for a file name used.\n");
            pconfig->Parse_Errors = true;
        }
    } else if (MATCH("File", "File_Header_Comment")) {
        if( strlen(value) < 121 )
            strcpy( pconfig->File_Header_Comment, value );
        else
        {
            printf("\nInvalid \"File_Header_Comment\": File prefix must have less than 120 characters.\n");
            pconfig->Parse_Errors = true;
        }
        
    } else if (MATCH("File", "File_Time_Secs")) {
        int tmp = atoi(value);
        if( tmp >= 0 )
            pconfig->File_Time_Secs = tmp;
        else
        {
            printf("\nInvalid \"File_Time_Secs\" value in config file (%i): can't be negative\n", tmp);
            pconfig->Parse_Errors = true;
        }
    } else {
        pconfig->Parse_Errors = true;
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

void *pa_Timer_thr( void *targs )
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
        timer_data->Elapsed_Time = Current_clock.tv_sec - Start_clock.tv_sec;
        if( *timer_data->Capture_Time_Secs_ptr > 0 )
            if( timer_data->Elapsed_Time > ( *timer_data->Capture_Time_Secs_ptr - 1 ) ) pa_flags.Running = false;
    }
}


void *pa_DisplayInfo_thr( void *targs )
{
    pa_run_info_t *pa_run_info = (pa_run_info_t*)targs;
    
    uint32_t rate = 0;
    uint64_t l_count = 0;
    sleep(1);
    while( pa_flags.Running )
    {
        rate = (uint16_t)(pa_run_info->n_pulses - l_count);
        l_count = pa_run_info->n_pulses;
        printf("\r| ET:%7i s | PC:%11" PRIu64 " | R:%5i Hz | FN:%7i |", *pa_run_info->Elapsed_Time_ptr, pa_run_info->n_pulses, rate, *pa_run_info->File_Number_ptr);
        fflush(stdout);
        sleep(1);
    }
    
}

void *pa_Logger_thr( void *targs )
{
    pa_logger_t *pa_logger = (pa_logger_t*)targs;
    
    if( pa_logger->Log_File_ptr->Log_File == NULL )
    {
        printf("Error: Log file not initialized");
        exit(0);
    }
    
    
    char DateTime[20];
    time_t tnow;
    float s_rate = 0.0;
    float m_rate = 0.0;
    float s_rate_min = 1e10;
    float s_rate_max = 0.0;
    uint64_t ls_count = 0;
    uint64_t lm_count = 0;
    int counter = 0;
    sleep(1);

    while( pa_flags.Running )
    {
        counter++;
        if(counter == 60)
        {
            counter = 0;
            tnow = time(NULL);
            struct tm *t = localtime(&tnow);
            strftime(DateTime, sizeof(DateTime)-1, "%d%m%y %H%M%S", t);
            
            m_rate = (float)(pa_logger->Run_Info_ptr->n_pulses - lm_count)/60.0;
            lm_count = pa_logger->Run_Info_ptr->n_pulses;
            
            s_rate = (float)(pa_logger->Run_Info_ptr->n_pulses - ls_count);
            if( s_rate > s_rate_max ) s_rate_max = s_rate;
            if( s_rate < s_rate_min ) s_rate_min = s_rate;
            
            ls_count = pa_logger->Run_Info_ptr->n_pulses;
            
            fprintf(pa_logger->Log_File_ptr->Log_File,"M %s %11" PRIu64 " %5.2f %5.2f %5.2f\n",  DateTime, pa_logger->Run_Info_ptr->n_pulses, m_rate, s_rate_max, s_rate_min );
            fflush(pa_logger->Log_File_ptr->Log_File);
            
            
            s_rate_min = 1e10;
            s_rate_max = 0.0;
    
        } else
        {
            s_rate = (float)(pa_logger->Run_Info_ptr->n_pulses - ls_count);
            if( s_rate > s_rate_max ) s_rate_max = s_rate;
            if( s_rate < s_rate_min ) s_rate_min = s_rate;
            
            ls_count = pa_logger->Run_Info_ptr->n_pulses;
            
        }
        sleep(1);
    }
}

int pa_InitVars( pa_config_t *config, pa_run_info_t *info, pa_timer_data_t *timer_data, pa_log_file_t *log_file, pa_data_file_t *data_file, pa_logger_t *logger )
{
    
    strcpy( config->Config_File_Name,   "none.conf");
    config->Pre_Trigger_Points          = 5;
    config->Pos_Trigger_Points          = 27;
    config->Trigger_Level               = -819;
    config->Trigger_Timeout_Secs        = 5;
    config->Capture_Time_Secs           = 1;
    strcpy( config->File_Name_Prefix,   "pa");
    strcpy( config->File_Header_Comment,"(None)");
    config->File_Time_Secs              = 60;
    config->Parse_Errors                = false;
    
    info->n_pulses                      = 0;
    info->t_errors                      = 0;
    info->Elapsed_Time_ptr              = &timer_data->Elapsed_Time;
    info->File_Number_ptr               = &data_file->File_Number;
    
    timer_data->Elapsed_Time            = 0;
    timer_data->Capture_Time_Secs_ptr   = &config->Capture_Time_Secs;
    
    log_file->Log_File                  = NULL;
    log_file->File_Name_Prefix_ptr      = config->File_Name_Prefix;
    
    data_file->Output_File              = NULL;
    strcpy( data_file->Output_File_Name,"none.paa");
    data_file->File_Number              = 0;
    data_file->i_time                   = 0;
    data_file->p_size                   = 0;
    data_file->cf_pulses                = 0;
    data_file->Elapsed_Time_ptr         = &timer_data->Elapsed_Time;
    data_file->File_Time_Secs_ptr       = &config->File_Time_Secs;
    data_file->File_Name_Prefix_ptr     = config->File_Name_Prefix;
    data_file->File_Header_Comment_ptr  = config->File_Header_Comment;
    data_file->Trigger_Level_ptr        = &config->Trigger_Level;
    
    logger->Run_Info_ptr                = info;
    logger->Log_File_ptr                = log_file;
    

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

int pa_InitRP()
{
    /* Init procedures */
    cmn_Init();
    osc_Init();
    osc_SetAveraging(true);
    osc_ResetWriteStateMachine();
    return 0;
}

int pa_StopRP()
{
    osc_Release();
    cmn_Release();
    return 0;
}

int pa_SettingsRP( pa_config_t *config )
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

int pa_LogFileEntry( pa_log_file_t *file, const char *Entry )
{
    if( (file->Log_File) != NULL )
    {
        char DateTime[20];
        time_t tnow = time(NULL);
        struct tm *t = localtime(&tnow);
        strftime(DateTime, sizeof(DateTime)-1, "%d%m%y %H%M%S", t);
        
        fprintf( file->Log_File , "I %s %s\n", DateTime, Entry );
        fflush( file->Log_File );
    }
    return 0;
}

int pa_InitLogFile( pa_log_file_t *file )
{
    if( (file->Log_File) == NULL )
    {
     
        char FileName[40];
        strcpy(FileName, file->File_Name_Prefix_ptr);
        strcat(FileName, "-run.log");
    
        file->Log_File = fopen(FileName, "w");
        
        if( (file->Log_File) == NULL )
        {
            printf("\nError: Can't open file for output\n");
            exit(0);
        }
        
        pa_LogFileEntry( file, "Log file started");
        
        char TZ[20];
        time_t tnow = time(NULL);
        struct tm *t = localtime(&tnow);
        strftime(TZ, sizeof(TZ)-1, "%Z", t);
        
        char log_entry[200];
        sprintf(log_entry, "Time Zone is %s", TZ);
        pa_LogFileEntry( file, log_entry );
    }
    return 0;
}


int pa_CloseLogFile( pa_log_file_t *file )
{
    if( (file->Log_File) != NULL )
    {
        pa_LogFileEntry(file, "Log file stopped" );
        
        fflush( file->Log_File );
        fclose( file->Log_File );
    }
    return 0;
}


int pa_InitDataFile( pa_data_file_t *file )
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
        
        file->File_Number++;
        
        
        strcpy( file->Output_File_Name, FileName);
        
    }
    
    return 0;
}

int pa_CloseDataFile( pa_data_file_t *data_file, pa_log_file_t *log_file )
{
    if( (data_file->Output_File) != NULL )
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
        struct tm *it = localtime(&data_file->i_time);
        strftime(itDateTime, sizeof(itDateTime)-1, "%c %Z", it);
        
        fseek( data_file->Output_File, 0, SEEK_SET);
        
        fprintf(data_file->Output_File, "PAA 01\n");                                            //   7 bytes
                                                                
        
        fseek( data_file->Output_File, 8, SEEK_SET);
        
        fprintf(data_file->Output_File, "Pulse Acquire tool for Red Pitaya\n");                //  34 bytes
        fprintf(data_file->Output_File, "Version: %d.%d.%d\n", _VERSION_MAJOR, _VERSION_MINOR, _VERSION_PATCH);
                                                                                               //  19 bytes
        fprintf(data_file->Output_File, "Build: %s\n", _BUILD);
                                                                                               // 128 bytes
        fprintf(data_file->Output_File, "%s\n",itDateTime);                                    //  29 bytes
        fprintf(data_file->Output_File, "%s\n",data_file->File_Header_Comment_ptr);            // 129 bytes
                                                                                          
        fseek( data_file->Output_File, 520, SEEK_SET);
        
        uint32_t eci = 0x10203040;
        fwrite(&eci, sizeof(uint32_t), 1, data_file->Output_File );                           //    4 bytes
        fwrite(&data_file->p_size, sizeof(uint32_t), 1, data_file->Output_File );             //    4 bytes
        fwrite(&data_file->cf_pulses, sizeof(uint32_t), 1, data_file->Output_File );          //    4 bytes
        fwrite(data_file->Trigger_Level_ptr, sizeof(int32_t), 1, data_file->Output_File );    //    4 bytes
                                                                           
        fflush( data_file->Output_File );
        fclose( data_file->Output_File );
        
        data_file->Output_File = NULL;
        
        char log_entry[200];
        sprintf(log_entry, "File %s writted with %d pulses", data_file->Output_File_Name, data_file->cf_pulses);
        pa_LogFileEntry( log_file, log_entry );
    }
    
    return 0;
}


int pa_GetFileName( pa_data_file_t *data_file, pa_log_file_t *log_file )
{
    if( *data_file->Elapsed_Time_ptr > (data_file->File_Number)*(*data_file->File_Time_Secs_ptr) )
    {
        pa_CloseDataFile( data_file, log_file );
        pa_InitDataFile( data_file );
    }
    
    return 0;
}
