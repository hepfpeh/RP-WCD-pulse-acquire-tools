/**
 * rp_m.h
 * 
 * Modifided version of rp.h from Red Pitaya api.
 * 
 * https://github.com/RedPitaya/RedPitaya
 * 
 */

#ifndef __RP_H
#define __RP_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#define ADC_BUFFER_SIZE             (16*1024)

/** @name Error codes
 *  Various error codes returned by the API.
 */
///@{

/** Success */
#define RP_OK     0
/** Failed to Open EEPROM Device */
#define RP_EOED   1
/** Failed to Open Memory Device */
#define RP_EOMD   2
/** Failed to Close Memory Device*/
#define RP_ECMD   3
/** Failed to Map Memory Device */
#define RP_EMMD   4
/** Failed to Unmap Memory Device */
#define RP_EUMD   5
/** Value Out Of Range */
#define RP_EOOR   6
/** LED Input Direction is not valid */
#define RP_ELID   7
/** Modifying Read Only field */
#define RP_EMRO   8
/** Writing to Input Pin is not valid */
#define RP_EWIP   9
/** Invalid Pin number */
#define RP_EPN    10
/** Uninitialized Input Argument */
#define RP_UIA    11
/** Failed to Find Calibration Parameters */
#define RP_FCA    12
/** Failed to Read Calibration Parameters */
#define RP_RCA    13
/** Buffer too small */
#define RP_BTS    14
/** Invalid parameter value */
#define RP_EIPV   15
/** Unsupported Feature */
#define RP_EUF    16
/** Data not normalized */
#define RP_ENN    17
/** Failed to open bus */
#define RP_EFOB   18
/** Failed to close bus */
#define RP_EFCB   19
/** Failed to acquire bus access */
#define RP_EABA   20
/** Failed to read from the bus */
#define RP_EFRB   21
/** Failed to write to the bus */
#define RP_EFWB   22
/** Extension module not connected */
#define RP_EMNC   23

#define SPECTR_OUT_SIG_LEN (2*1024)

///@}

/**
 * Type representing digital input output pins.
 */
typedef enum {
    RP_LED0,       //!< LED 0
    RP_LED1,       //!< LED 1
    RP_LED2,       //!< LED 2
    RP_LED3,       //!< LED 3
    RP_LED4,       //!< LED 4
    RP_LED5,       //!< LED 5
    RP_LED6,       //!< LED 6
    RP_LED7,       //!< LED 7
    RP_DIO0_P,     //!< DIO_P 0
    RP_DIO1_P,     //!< DIO_P 1
    RP_DIO2_P,     //!< DIO_P 2
    RP_DIO3_P,     //!< DIO_P 3
    RP_DIO4_P,     //!< DIO_P 4
    RP_DIO5_P,     //!< DIO_P 5
    RP_DIO6_P,     //!< DIO_P 6
    RP_DIO7_P,	   //!< DIO_P 7
    RP_DIO0_N,     //!< DIO_N 0
    RP_DIO1_N,     //!< DIO_N 1
    RP_DIO2_N,     //!< DIO_N 2
    RP_DIO3_N,     //!< DIO_N 3
    RP_DIO4_N,     //!< DIO_N 4
    RP_DIO5_N,     //!< DIO_N 5
    RP_DIO6_N,     //!< DIO_N 6
    RP_DIO7_N      //!< DIO_N 7
} rp_dpin_t;

/**
 * Type representing pin's high or low state (on/off).
 */
typedef enum {
    RP_LOW, //!< Low state
    RP_HIGH //!< High state
} rp_pinState_t;

/**
 * Type representing pin's input or output direction.
 */
typedef enum {
    RP_IN, //!< Input direction
    RP_OUT //!< Output direction
} rp_pinDirection_t;

/**
 * Type representing analog input output pins.
 */
typedef enum {
    RP_AOUT0,      //!< Analog output 0
    RP_AOUT1,      //!< Analog output 1
    RP_AOUT2,      //!< Analog output 2
    RP_AOUT3,      //!< Analog output 3
    RP_AIN0,       //!< Analog input 0
    RP_AIN1,       //!< Analog input 1
    RP_AIN2,       //!< Analog input 2
    RP_AIN3        //!< Analog input 3
} rp_apin_t;

typedef enum {
    RP_WAVEFORM_SINE,       //!< Wave form sine
    RP_WAVEFORM_SQUARE,     //!< Wave form square
    RP_WAVEFORM_TRIANGLE,   //!< Wave form triangle
    RP_WAVEFORM_RAMP_UP,    //!< Wave form sawtooth (/|)
    RP_WAVEFORM_RAMP_DOWN,  //!< Wave form reversed sawtooth (|\)
    RP_WAVEFORM_DC,         //!< Wave form dc
    RP_WAVEFORM_PWM,        //!< Wave form pwm
    RP_WAVEFORM_ARBITRARY   //!< Use defined wave form
} rp_waveform_t;

typedef enum {
    RP_GEN_MODE_CONTINUOUS, //!< Continuous signal generation
    RP_GEN_MODE_BURST,      //!< Signal is generated N times, wher N is defined with rp_GenBurstCount method
    RP_GEN_MODE_STREAM      //!< User can continuously write data to buffer
} rp_gen_mode_t;


typedef enum {
    RP_GEN_TRIG_SRC_INTERNAL = 1,   //!< Internal trigger source
    RP_GEN_TRIG_SRC_EXT_PE = 2,     //!< External trigger source positive edge
    RP_GEN_TRIG_SRC_EXT_NE = 3,     //!< External trigger source negative edge
    RP_GEN_TRIG_GATED_BURST     //!< External trigger gated burst
} rp_trig_src_t;

/**
 * Type representing Input/Output channels.
 */
typedef enum {
    RP_CH_1, //!< Channel A
    RP_CH_2  //!< Channel B
} rp_channel_t;

/**
 * Type representing acquire signal sampling rate.
 */
typedef enum {
    RP_SMP_125M,     //!< Sample rate 125Msps; Buffer time length 131us; Decimation 1
    RP_SMP_15_625M,  //!< Sample rate 15.625Msps; Buffer time length 1.048ms; Decimation 8
    RP_SMP_1_953M,   //!< Sample rate 1.953Msps; Buffer time length 8.388ms; Decimation 64
    RP_SMP_122_070K, //!< Sample rate 122.070ksps; Buffer time length 134.2ms; Decimation 1024
    RP_SMP_15_258K,  //!< Sample rate 15.258ksps; Buffer time length 1.073s; Decimation 8192
    RP_SMP_1_907K    //!< Sample rate 1.907ksps; Buffer time length 8.589s; Decimation 65536
} rp_acq_sampling_rate_t;


/**
 * Type representing decimation used at acquiring signal.
 */
typedef enum {
    RP_DEC_1,     //!< Sample rate 125Msps; Buffer time length 131us; Decimation 1
    RP_DEC_8,     //!< Sample rate 15.625Msps; Buffer time length 1.048ms; Decimation 8
    RP_DEC_64,    //!< Sample rate 1.953Msps; Buffer time length 8.388ms; Decimation 64
    RP_DEC_1024,  //!< Sample rate 122.070ksps; Buffer time length 134.2ms; Decimation 1024
    RP_DEC_8192,  //!< Sample rate 15.258ksps; Buffer time length 1.073s; Decimation 8192
    RP_DEC_65536  //!< Sample rate 1.907ksps; Buffer time length 8.589s; Decimation 65536
} rp_acq_decimation_t;


/**
 * Type representing different trigger sources used at acquiring signal.
 */
typedef enum {
    RP_TRIG_SRC_DISABLED, //!< Trigger is disabled
    RP_TRIG_SRC_NOW,      //!< Trigger triggered now (immediately)
    RP_TRIG_SRC_CHA_PE,   //!< Trigger set to Channel A threshold positive edge
    RP_TRIG_SRC_CHA_NE,   //!< Trigger set to Channel A threshold negative edge
    RP_TRIG_SRC_CHB_PE,   //!< Trigger set to Channel B threshold positive edge
    RP_TRIG_SRC_CHB_NE,   //!< Trigger set to Channel B threshold negative edge
    RP_TRIG_SRC_EXT_PE,   //!< Trigger set to external trigger positive edge (DIO0_P pin)
    RP_TRIG_SRC_EXT_NE,   //!< Trigger set to external trigger negative edge (DIO0_P pin)
    RP_TRIG_SRC_AWG_PE,   //!< Trigger set to arbitrary wave generator application positive edge
    RP_TRIG_SRC_AWG_NE    //!< Trigger set to arbitrary wave generator application negative edge
} rp_acq_trig_src_t;


/**
 * Type representing different trigger states.
 */
typedef enum {
    RP_TRIG_STATE_TRIGGERED, //!< Trigger is triggered/disabled
    RP_TRIG_STATE_WAITING,   //!< Trigger is set up and waiting (to be triggered)
} rp_acq_trig_state_t;


/**
 * Calibration parameters, stored in the EEPROM device
 */
typedef struct {
    uint32_t fe_ch1_fs_g_hi; //!< High gain front end full scale voltage, channel A
    uint32_t fe_ch2_fs_g_hi; //!< High gain front end full scale voltage, channel B
    uint32_t fe_ch1_fs_g_lo; //!< Low gain front end full scale voltage, channel A
    uint32_t fe_ch2_fs_g_lo; //!< Low gain front end full scale voltage, channel B
    int32_t  fe_ch1_lo_offs; //!< Front end DC offset, channel A
    int32_t  fe_ch2_lo_offs; //!< Front end DC offset, channel B
    uint32_t be_ch1_fs;      //!< Back end full scale voltage, channel A
    uint32_t be_ch2_fs;      //!< Back end full scale voltage, channel B
    int32_t  be_ch1_dc_offs; //!< Back end DC offset, channel A
    int32_t  be_ch2_dc_offs; //!< Back end DC offset, on channel B
	uint32_t magic;			 //!
    int32_t  fe_ch1_hi_offs; //!< Front end DC offset, channel A
    int32_t  fe_ch2_hi_offs; //!< Front end DC offset, channel B
} rp_calib_params_t;



#ifdef __cplusplus
}
#endif

#endif //__RP_H
