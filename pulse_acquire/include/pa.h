/*
 * pa.h
 * 
 * Pulse acquire utility for Red Pitaya.
 * 
 * 
 */

#ifndef _PA_H
#define _PA_H

#include <inttypes.h>
#include <time.h>
#include <stdbool.h>

/* configuration file */

/* ADC acquisition bits mask. */
static const int ADC_BITS_MAK = 0x3FFF;

typedef struct pa_config_s
{
    char        Config_File_Name[128];
    uint32_t    Pre_Trigger_Points;
    uint32_t    Pos_Trigger_Points;
    int32_t     Trigger_Level;
    uint16_t    Trigger_Timeout_Secs;
    uint32_t    Capture_Time_Secs;
    char        File_Name_Prefix[20];
    char        File_Header_Comment[128];
    uint32_t    File_Time_Secs;
    bool        Parse_Errors;
} pa_config_t;


typedef struct pa_flags_s
{
    bool Running;
    bool ShowInfo; //Actualy unused
} pa_flags_t;


typedef struct pa_run_info_s 
{
    uint64_t n_pulses;
    uint32_t t_errors;
    const uint32_t *Elapsed_Time_ptr;
    const uint32_t *File_Number_ptr;
} pa_run_info_t;


typedef struct pa_timer_data_s
{
    uint32_t Elapsed_Time;
    const uint32_t *Capture_Time_Secs_ptr; // Pointer to capture time
} pa_timer_data_t;


typedef struct pa_data_file_s
{
    FILE *Output_File;
    char Output_File_Name[128];
    uint32_t File_Number;
    time_t   i_time;
    uint32_t p_size;
    uint32_t cf_pulses;
    const uint32_t *Elapsed_Time_ptr;
    const uint32_t *File_Time_Secs_ptr;
    const char     *File_Name_Prefix_ptr;
    const char     *File_Header_Comment_ptr;
    const int32_t  *Trigger_Level_ptr;
} pa_data_file_t;

typedef struct pa_log_file_s
{
    FILE *Log_File;
    const char     *File_Name_Prefix_ptr;
} pa_log_file_t;

typedef struct pa_logger_s
{
    const pa_run_info_t *Run_Info_ptr;
    const pa_log_file_t *Log_File_ptr;
} pa_logger_t;


extern volatile pa_flags_t pa_flags;


int pa_config_handler(void* user, const char* section, const char* name, const char* value);

void *pa_Timer_thr( void *targs );

void *pa_DisplayInfo_thr( void *targs );

void *pa_Logger_thr( void *targs );

int pa_InitVars( pa_config_t *config, pa_run_info_t *info, pa_timer_data_t *timer_data, pa_log_file_t *log_file, pa_data_file_t *data_file, pa_logger_t *logger );

int pa_CheckConfig( pa_config_t *config );

int pa_InitRP();

int pa_StopRP();

int pa_SettingsRP( pa_config_t *config );

int pa_LogFileEntry( pa_log_file_t *file, const char *Entry );

int pa_InitLogFile( pa_log_file_t *file );

int pa_CloseLogFile( pa_log_file_t *file );

int pa_InitDataFile( pa_data_file_t *file );

int pa_CloseDataFile( pa_data_file_t *data_file, pa_log_file_t *log_file );

int pa_GetFileName( pa_data_file_t *data_file, pa_log_file_t *log_file );


#endif //__PA_H
