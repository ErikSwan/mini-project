#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <samplerate.h>
#include <pthread.h>

#define PCM_DEVICE "default"
#define converter 1
#define MAX_PLAY_RATE 5
#define NUMFILES 5

// GLOBAL STRUCT
typedef struct InputData {
  float speed;  // slide bar potentiometers, slave 1
  int gain;   // slide bar potentiometers, slave 1
  //uint8 high_eq;  // knobs, slave 1
  //uint8 mid_eq; // knobs, slave 1
  //uint8 low_eq; // knobs, slave 1
  //uint8 fx1;    // knobs, slave 1
  //uint8 fx2;    // knobs, slave 1
  //uint8 fx3;    // knobs, slave 1
  //uint8 encoder;  // turntable encoder count, slave 1
  //uint8 next;   // buttons, slave 2
  //uint8 prev;   // buttons, slave 2
  int play_pause; // buttons, slave 2
  //uint8 fx_onoff; // buttons, slave 2
} InputData;

InputData data = {1,5,1};
int flag = 0;
void * thread_sampling(void * unused);


//void fileloadmem(float **filebase, float **filepos, float **eofin, SNDFILE **input, SF_INFO *sfinf);
//void fileopen(SNDFILE **input, SF_INFO *sfinf, char *filename);



main(int argc, char **argv){
	float *filein = NULL;
	float *buffin = NULL;
	float *eofin;
	float *buffout;
	short *final;
	char *file1 = "musik.wav";
	char *file2 = "got.wav";
	int pcm, pcmrc;
	int tmp;
	int i = 0;
  int *blah;
  // create the SPI thread
  pthread_t * spithread = malloc(sizeof(pthread_t));
  
      

	SNDFILE *inputf;
	SF_INFO sfinf;

	SRC_DATA datastr;
	SRC_STATE  *statestr;
	int error;

	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

//	fileopen(&inputf, &sfinf, file1);
	inputf = sf_open(file2, SFM_READ, &sfinf);
	if (inputf == NULL) printf("COULD NOT OPEN FILE SORRY \n\n");
    if ((filein = malloc(sizeof(float)*sfinf.channels*sfinf.frames)) == NULL)
   {
       printf("MAN YOU OUT OF MEM");
       return 0;
   }
	//load file into memory
//	fileloadmem(&filein, &buffin, &eofin, &inputf, &sfinf);
   	sf_readf_float(inputf, filein , sfinf.frames);
	buffin = filein;
	eofin = filein + sfinf.frames * sfinf.channels ;


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
    if (pcm = snd_pcm_hw_params_set_channels(handle, params, sfinf.channels)< 0){
        printf("CANNOT SET CHANNELS \n");
    }
    if (pcm = snd_pcm_hw_params_set_rate(handle, params, sfinf.samplerate, 0)< 0){
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

    buffout = malloc(frames*sfinf.channels * sizeof(float));//frames
    final = malloc(frames*sfinf.channels * sizeof(short));

    statestr = src_new (converter, sfinf.channels, &error);

       //datastr->end_of_input = 0;
    datastr.end_of_input = 0;
    datastr.data_out = buffout;
    datastr.output_frames = frames;//frames

    while (i != 50){
       memset(final, 0, frames*sfinf.channels);
       pcmrc = snd_pcm_writei(handle, final, frames);//frames
               if (pcmrc == -EPIPE){//this is where the error happens on the DAc for like 25 - 500 ms it underruns but recovers, so i dont know
            printf("UNDERRUN!\n");
            snd_pcm_prepare(handle);
        }
        else if(pcmrc < 0){
            printf("ERROR WRITING!!!\n");
        }
       // else if (pcmrc != datastr.output_frames_gen){
         //   printf("WRITE != READ\n");
       // }
        i++;
    }

    pthread_create(spithread, NULL,  thread_sampling, NULL);
    while(buffin != eofin){
        datastr.data_in = buffin;
    //   datastr.src_ratio = src_ratio;
      // if (datastr.input_frames == 0 ){datastr.input_frames = frames*MAX_PLAY_RATE;}//frames
       datastr.input_frames = frames*MAX_PLAY_RATE;
       if (data.play_pause == 1){
          // datastr.src_ratio = datamain.src_r;
         // printf("SRC RATIO: %f\n\n", data.src_r);
           if (src_set_ratio(statestr, data.speed) != 0){
                printf("Could not reset ratio\n");
           }
           
           
           if ((eofin - buffin) < frames*MAX_PLAY_RATE) {
           //datastr.end_of_input = SF_TRUE; // if you read less than a full frame it says i am done
           memset(buffout, 0.0, frames*sfinf.channels*sizeof(float));//frames
           if( (eofin - buffin) < frames) {
               datastr.end_of_input = SF_TRUE;
               break;
           }
       }
       
        datastr.src_ratio = data.speed;   
        if ((error = src_process(statestr, &datastr)))
       {
           printf("\n\nERRORR converting: %s\n\n", src_strerror(error));
           exit(1);
       }
       buffin = buffin + datastr.input_frames_used*sfinf.channels;
       //datastr.input_frames -= datastr.input_frames_used; 
       //printf("number of frames generated %d \n", datastr.output_frames_gen);
       src_float_to_short_array(buffout, final, frames*sfinf.channels);//frames
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
    	
    	if (data.play_pause == 0){
    		continue;
    	}

        
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
  // DECLARATIONS
  char c = ' ';
  
  while (1) {
    c = getc(stdin);
    getc(stdin);
    
    // CONDITIONAL ADJUSTMENTS  
    if (c == 'p'){
      data.play_pause = !(data.play_pause);
      flag = 1;
    } else if (c == 'f'){
      data.speed = data.speed - .1;
      flag = 1;
    } else if (c == 's'){
      data.speed = data.speed + .1;
      flag = 1;
    } else if (c == 'u'){
      data.gain++;
      flag = 1;
    } else if (c == 'd'){
      data.gain--;
      flag = 1;
    }
    
    /*
    printf("data.speed = %.1f\n",data.speed);
    printf("data.gain = %d\n",data.gain);
    printf("data.play_pause = %d\n",data.play_pause);
    */
  }

  return NULL;
}

/*void fileopen(SNDFILE **input, SF_INFO *sfinf, char *filename){
	input = sf_open(filename, SFM_READ, &sfinf);
	if (input == NULL) printf("COULD NOT OPEN FILE SORRY \n\n");
	return;
}*/

/*void fileloadmem(float **filebase, float **filepos, float **eofin, SNDFILE **input, SF_INFO *sfinf){
	sf_readf_float(input, filebase , sfinf.frames);
	filepos = filebase;
	eofin = filebase + sfinf.frames * sfinf.channels ;
	return;	
}*/


