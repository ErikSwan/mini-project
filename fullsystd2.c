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
  float hpgain;  // knobs, slave 1
  float mpgain; // knobs, slave 1
  float lpgain; // knobs, slave 1
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
InputData data = {1,.5,.5, .5, .5,0,0,1,0};
typedef struct biquad{
    float a0, a1, a2;
    float b1, b2;
    float hist1, hist2;
} biquad;


biquad lpf = {.003765, .007529, .003765, -1.819091,.834149, 0, 0 };
biquad hpf = {.666638, -1.333276, .666638, -1.218876, .447677, 0, 0};


int flag = 0;
void * thread_sampling(void * unused);
void gainfn(float *inputarr, int samples, int channels, float mult);
void L3bandEQ(float *in,int samples, int channels, biquad lpf, biquad hpf);
void R3bandEQ(float *in, int samples, int channels, biquad lpf, biquad hpf);
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

//   	sf_readf_float(inputf, filein , sfinf.frames);


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
       L3bandEQ(buffout, frames, sfinf[prevtrack].channels, lpf, hpf);
       R3bandEQ(buffout, frames, sfinf[prevtrack].channels, lpf, hpf);
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
      data.gain+=.1;
      if (data.gain >= 2) data.gain = 1.2;
      flag = 1;
    } else if (c == 'd'){
      data.gain-=.1;
      if (data.gain <= .2) data.gain = .2;
      flag = 1;
    }else if (c == 'n'){
        data.next = 1;
        data.trackno += 1;
        if (data.trackno >=  NUMFILES){
            data.next = 0;
            data.trackno = NUMFILES - 1;
        }
    }else if (c == 'b'){
        data.back = 1;
        data.trackno -= 1;
        if (data.trackno < 0){
            data.trackno = 0;
            data.back = 0;
        }
    }else if (c == '1'){
        data.trackno = 0;
    } else if (c == '2'){
        data.trackno = 1;
    }else if (c == 'l'){
        data.lpgain = data.lpgain + .1;
        if (data.lpgain >= 2) data.lpgain = 2;
    }else if (c == 'h'){
        data.hpgain = data.hpgain + .05;
        if (data.hpgain >= 1.1) data.hpgain = 1.1;
    }else if (c == 'm'){
        data.mpgain = data.mpgain +.1;
        if (data.mpgain >= 2) data.mpgain = 2;
    }else if (c == 'j'){
        data.lpgain -= .1;
        if (data.lpgain <= 0) data.lpgain = 0;
    }else if (c == 'k'){
        data.hpgain -= .1;
        if (data.hpgain <= 0) data.hpgain = 0;
    }else if (c == 'z'){
        data.mpgain -= .1;
        if (data.mpgain <= 0) data.mpgain = 0;
    }else if (c == 'q'){
        data.hpgain = 0;
    }else if (c == 'w'){
        data.lpgain = 0;
    }
  }
    return NULL;
}

void gainfn(float *inputarr, int samples, int channels , float mult){
    int i = 0;
    for (i = 0; i<channels*samples;i++){
        inputarr[i] = inputarr[i] * mult;
    }
}

void L3bandEQ(float *in, int samples, int channels, biquad lpf, biquad hpf){
    lpf.hist2 = in[0];
    lpf.hist1 = in[2];
    hpf.hist2 = in[0];
    hpf.hist1 = in[2];
    int i;
    float outl, outm, outh;
    float nhistl, nhisth;
    for (i = 4; i< samples*channels; i+=2){
        //do thw low pass
        outl = (lpf.a0)*in[i];
        outl = outl - (lpf .hist1) * (lpf . b1);//poles
        nhistl = outl - (lpf .hist2)*(lpf .b2);//poles
        outl = nhistl + (in[i -2]) *(lpf . a1);//zeros
        outl = outl + (in[i -4])*(lpf . a2);//zeros
       lpf.hist2 = lpf.hist1;
       lpf.hist1 = nhistl;
        
        //outl =( lpf.a0 * in[i]) + (lpf.a1 * in[i - 2]) + (lpf.a2 * in[i - 4]);
        //outl = outl - (lpf.b1 * lpf.hist1) - (lpf.b2 *lpf.hist2);
        
        //lpf . hist2 = lpf .hist1;
        //lpf .hist1 = outl;
        

        //do the high pass
        outh = (hpf.a0)*in[i];
        outh = outh - (hpf .hist1) * (hpf . b1);
        nhisth = outh - (hpf .hist2)*(hpf .b2);
        outh = nhistl + (in[i-2]) *(hpf . a1);
        outh = outh + (in[i-4])*(hpf . a2);
        hpf . hist2 = hpf .hist1;
        hpf .hist1 = nhisth;
        //outl =( hpf.a0 * in[i]) + (hpf.a1 * in[i - 2]) + (hpf.a2 * in[i - 4]);
        //outl = outl - (hpf.b1 * hpf.hist1) - (hpf.b2 *hpf.hist2);
        
        //hpf . hist2 = hpf .hist1;
        //hpf .hist1 = outl;



        outm = in[i] - outh - outl;

        outl = outl * data.lpgain;
        outm = outm*data.mpgain;
        outh = outh * data.hpgain;

        in[i] = outl + outm + outh;
    }
}

void R3bandEQ(float *in, int samples, int channels, biquad lpf, biquad hpf){
    lpf.hist2 = in[1];
    lpf.hist1 = in[3];
    hpf.hist2 = in[1];
    hpf.hist1 = in[3];
    int i;
    float outl, outm, outh;
    float nhistl, nhisth;
    for (i = 5; i< samples*channels; i+=2){
        //do thw low pass
        outl = (lpf.a0)*in[i];
        outl = outl - (lpf .hist1) * (lpf . b1);//poles
        nhistl = outl - (lpf .hist2)*(lpf .b2);//poles
        outl = nhistl + (in[i -2]) *(lpf . a1);//zeros
        outl = outl + (in[i -4])*(lpf . a2);//zeros
        lpf . hist2 = lpf .hist1;
        lpf .hist1 = nhistl;
        //outl =( lpf.a0 * in[i]) + (lpf.a1 * in[i - 2]) + (lpf.a2 * in[i - 4]);
        //outl = outl - (lpf.b1 * lpf.hist1) - (lpf.b2 *lpf.hist2);
        
        //lpf . hist2 = lpf .hist1;
        //lpf .hist1 = outl;
         

        //do the high pass
        outh = (hpf.a0)*in[i];
        outh = outh - (hpf .hist1) * (hpf . b1);
        nhisth = outh - (hpf .hist2)*(hpf .b2);
        outh = nhistl + (in[i-2]) *(hpf . a1);
        outh = outh + (in[i-4])*(hpf . a2);
        hpf . hist2 = hpf .hist1;
        hpf .hist1 = nhisth;
        //outl =( hpf.a0 * in[i]) + (hpf.a1 * in[i - 2]) + (hpf.a2 * in[i - 4]);
        //outl = outl - (hpf.b1 * hpf.hist1) - (hpf.b2 *hpf.hist2);
        
        //hpf . hist2 = hpf .hist1;
        //hpf .hist1 = outl;
         
        outm = in[i] - outh - outl;

        outl = outl * data.lpgain;
        outm = outm*data.mpgain;
        outh = outh * data.hpgain;

        in[i] = outl + outm + outh;
    }
}
