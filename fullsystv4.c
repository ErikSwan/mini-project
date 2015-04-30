#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <samplerate.h>
#include <pthread.h>
#include <bcm2835.h>

#define PCM_DEVICE "default"
#define converter 1
#define MAX_PLAY_RATE 5
#define NUMFILES 2

// GLOBAL STRUCT
typedef struct InputData {
  float speed;  // slide bar potentiometers, slave 1
  float gain;   // slide bar potentiometers, slave 1
  //uint8 high_eq;  // knobs, slave 1
  //uint8 mid_eq; // knobs, slave 1
  //uint8 low_eq; // knobs, slave 1
  //uint8 fx1;    // knobs, slave 1
  //uint8 fx2;    // knobs, slave 1
  //uint8 fx3;    // knobs, slave 1
  //uint8 encoder;  // turntable encoder count, slave 1
  int next;   // buttons, slave 2
  int back;   // buttons, slave 2
  int play_pause; // buttons, slave 2
  int trackno; //menu toggle encoder
  //uint8 fx_onoff; // buttons, slave 2
} InputData;
InputData data = {1,.5,0,0,1,0};
int flag = 0;
void * thread_sampling(void * unused);
void gainfn(float *inputarr, int samples, int channels, float mult);
/**********************************************************************/
main(int argc, char **argv) {
  
    // DECLARATIONS
    float *filein[NUMFILES];
    float *buffin = NULL;
    float *eofin;
    float *buffout;
    short *final;
    char *file[NUMFILES] = {"musik.wav", "wtm.wav"};
    //char *file2 = "got.wav";
    int pcm, pcmrc;
    int tmp;
    int i = 0;
    int *blah;
    
    //  SPI THREAD
    pthread_t * spithread = malloc(sizeof(pthread_t));
    
    // INITIALIZATIONS
    SNDFILE *inputf[NUMFILES];//SNDFILE *inputf2
    SF_INFO sfinf[NUMFILES];

    SRC_DATA datastr;
    SRC_STATE  *statestr;
    int error;

    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;

    // Open file and create pointer
    
    for(i = 0; i < NUMFILES; i++){
    inputf[i] = sf_open(file[i], SFM_READ, &(sfinf[i]));
    if (inputf[i] == NULL) printf("COULD NOT OPEN FILE SORRY \n\n");
        if ((filein[i] = malloc(sizeof(float)*sfinf[i].channels*sfinf[i].frames)) == NULL)
        {
           printf("MAN YOU OUT OF MEM for file %d\n\n", i);
           return 0;
        }
        sf_readf_float(inputf[i], filein[i], sfinf[i].frames);
  
    }   

//    sf_readf_float(inputf, filein , sfinf.frames);


    if (pcm =snd_pcm_open(&handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)< 0){
        printf("Error: CANNOT OPEN PCM DEVICE\n");
    }

    //allocate default parameters
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    
    //set parameters
    if (pcm = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)< 0){
        printf("Cannot set interleaved mode\n");
    }

    if (pcm = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE)< 0){
        printf("CANNOT SET FORMAT\n");
    }
    if (pcm = snd_pcm_hw_params_set_channels(handle, params, sfinf[1].channels)< 0){
        printf("CANNOT SET CHANNELS \n");
    }
    if (pcm = snd_pcm_hw_params_set_rate(handle, params, sfinf[1].samplerate, 0)< 0){
        printf("CANNOT SET SAMPLERATES \n");
    }
    
    //write parameters
    if (pcm = snd_pcm_hw_params(handle, params)< 0){
        printf("CANNOT SET HARDWARE PARAMETERS\n");
    }
    printf("PCm NAME: %s\n", snd_pcm_name(handle));
    printf("PCM state: %s \n", snd_pcm_state_name(snd_pcm_state(handle)));
    snd_pcm_hw_params_get_channels(params, &tmp);
    printf("channels: %i\n", tmp);
    snd_pcm_hw_params_get_rate(params, &tmp, 0);
    printf("rate: %d \n", tmp);
    snd_pcm_hw_params_get_period_size(params, &frames, 0);
    fprintf(stderr, "# frames in a period: %d\n", frames);

    buffout = malloc(frames*sfinf[1].channels * sizeof(float));//frames
    final = malloc(frames*sfinf[1].channels * sizeof(short));

    statestr = src_new (converter, sfinf[1].channels, &error);

    datastr.end_of_input = 0;
    datastr.data_out = buffout;
    datastr.output_frames = frames;//frames

    while (i != 50){
       memset(final, 0, frames*sfinf[1].channels);
       pcmrc = snd_pcm_writei(handle, final, frames);//frames
               if (pcmrc == -EPIPE){//this is where the error happens on the DAc for like 25 - 500 ms it underruns but recovers, so i dont know
            printf("UNDERRUN!\n");
            snd_pcm_prepare(handle);
        }
        else if(pcmrc < 0){
            printf("ERROR WRITING!!!\n");
        }
        i++;
    }

    // create the thread wooooo
    pthread_create(spithread, NULL,  thread_sampling, NULL);
   for (;;) {
    /***************** START PLAYING MUSIC *******************/
    int prevtrack = data.trackno;
    int finish = 0;;
    buffin = filein[prevtrack];
    eofin = filein[prevtrack] + sfinf[prevtrack].frames * sfinf[prevtrack].channels ;
    while(buffin != eofin){
        datastr.data_in = buffin;
        datastr.input_frames = frames*MAX_PLAY_RATE;
        if (data.play_pause == 1 && data.next == 0 && data.back == 0 && data.trackno == prevtrack){
           if (src_set_ratio(statestr, data.speed) != 0){
                printf("Could not reset ratio\n");
           }
           
           if ((eofin - buffin) < frames*MAX_PLAY_RATE) {
           /// if you read less than a full frame it says i am done
           memset(buffout, 0.0, frames*sfinf[prevtrack].channels*sizeof(float));//frames
           if( (eofin - buffin) < frames) {
               datastr.end_of_input = SF_TRUE;
               finish = 1;
               break;
           }
        }
       
        datastr.src_ratio = data.speed;   
        if ((error = src_process(statestr, &datastr))){
           printf("\n\nERRORR converting: %s\n\n", src_strerror(error));
           exit(1);
       }
       buffin = buffin + datastr.input_frames_used*sfinf[prevtrack].channels;
       //datastr.input_frames -= datastr.input_frames_used; 
       //printf("number of frames generated %d \n", datastr.output_frames_gen);
       gainfn(buffout, frames, sfinf[prevtrack].channels, data.gain);
       src_float_to_short_array(buffout, final, frames*sfinf[prevtrack].channels);//frames
       usleep(3000); //even with all the processing we can still sleep for like 3000;
       pcmrc = snd_pcm_writei(handle, final, frames);//frames
       if (pcmrc == -EPIPE){//this is where the error happens on the DAc for like 25 - 500 ms it underruns but recovers, so i dont know
            printf("UNDERRUN!\n");
            snd_pcm_prepare(handle);
        }
        else if(pcmrc < 0){
            printf("ERROR WRITING!!!\n");
        }
        else if (pcmrc != datastr.output_frames_gen){
            printf("WRITE != READ\n");
        }
      }
      
        else if (data.play_pause == 0){
        if (data.trackno != prevtrack) 
                break;
            continue;
      }
        else if (data.next == 1 || data.back == 1 || (data.trackno != prevtrack)) {
            break;
        }
    }
    data.next = 0;
    data.back = 0;
    if (finish){
        data.trackno++;
    }
    else continue;
   }

    free(filein);
    free(buffout);
    free(final);
    free(spithread);

   // free(data);
  return 0;

}

// entry function for the sampling thread
void * thread_sampling(void * unused)
{
// INITIALIZE SPI FOR WRITER
  if (!bcm2835_init()) {return NULL;}

  bcm2835_spi_begin();
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128); // The default
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

  // WRITER SPI
  int i=0;
  uint8_t send_data[11] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0,0xB0};
  uint8_t rec_data[11] = {0,0,0,0,0,0,0,0,0,0};
  uint8_t read_data = 0;

  for (i=0;i<11;i++){
      read_data = bcm2835_spi_transfer(send_data[i]);
      usleep(2000);
      printf("Sent to SPI: 0x%02X. Read back from SPI: 0x%02X.\n", send_data[i], read_data);
      rec_data[i] = read_data;
      printf("Received data = 0x%02X\n",read_data); 
      read_data = 0;
  }

  while (1) {
      for (i=0;i<11;i++){
          read_data = bcm2835_spi_transfer(send_data[i]);
          //printf("Sent to SPI: 0x%02X. Read back from SPI: 0x%02X.\n", send_data[i], read_data);
          rec_data[i] = read_data;
          //printf("Received data = 0x%02X\n",read_data); 
          read_data = 0;
      }
      usleep(2000);
      data.speed = ((1.4/255) * rec_data[0]) + 0.3;
      //printf("Speed = %.1f\n");
      data.gain = ((1.8/255) * rec_data[1]) + .2;
      //data.play_pause = rec_data[10] & 0x80;
  }

  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}

void gainfn(float *inputarr, int samples, int channels , float mult){
    int i = 0;
    for (i = 0; i<channels*samples;i++){
        inputarr[i] = inputarr[i] * mult;
    }
}
