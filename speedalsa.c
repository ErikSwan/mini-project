// this gonna be our thing

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <samplerate.h>



#define PCM_DEVICE "hw:1,0"//"hw:1,0" for the dac  "default" for the onbuilt jack
#define MAX_PLAY_RATE 5//basically for buffer management so our input buffer is always big enough for the effective downsample happening
#define converter 1//0 for highest quality(dont do this), 1 for medium(this works), 2 for fastest sinc, 3 for ZOH, 4 for linear 
//void playback(char cardname, int freq, int frame, int *buf); 
//void playback(snd_pcm_t *handle, snd_pcm_hw_params_t *params, int *buff);
//long fib(long n);
main(int argc, char **argv){
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    //the libsamplerate stuff
    SRC_DATA datastr;
    SRC_STATE *statestr;
    int error;
    //the lbsndfile stuff, one keeps the header the other the data
    SF_INFO sfinf;
    SNDFILE *input = NULL;
    //buffer to pass to the playback function
    float *buffin = NULL;//float into the converter
    float *filein = NULL;
    float *buffout = NULL;//float out of the converter
    short *final = NULL;//final array to write to snd card
    int pcm, readcount, pcmrc, blah;
    float *eofin;
    int tmp;
    long tempf;
    float seconds;
    int minf, maxf;    
    // pointer for the file input
    char *inputname = argv[1];
    //int *filein = argv[1];

    input = sf_open(inputname, SFM_READ, &sfinf);
    fprintf(stderr, "Channels : %d\n", sfinf.channels);
    fprintf(stderr, "Sample rate; %d\n", sfinf.samplerate);
    fprintf(stderr, "Sections: %d\n", sfinf.sections);
    fprintf(stderr, "Format: %d\n", sfinf.format);
   // fprintf(stderr, "Frame Count: %d\n", sfinf.frames);
    //fprintf(stderr, "size: %d\n", sizeof(short));
   //open sndcard
   if ((filein = malloc(sizeof(float)*sfinf.channels*sfinf.frames)) == NULL)
   {
       printf("MAN YOU OUT OF MEM");
       return(0);
   }
   blah = sf_readf_float(input, filein, sfinf.frames);
   buffin = filein;
   eofin = filein + (sfinf.frames)*sfinf.channels;




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
    if (pcm = snd_pcm_hw_params_set_periods(handle, params,1024, 0) < 0){
        printf("CANNOT SET PERIOD TO 128");
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
    //find size of frames
    snd_pcm_hw_params_get_period_size(params, &frames, 0);
 //   minf = snd_pcm_hw_params_get_period_size_min(params, &frames, 0);
   // maxf = snd_pcm_hw_params_get_period_size_max(params, &frames, 0);
    fprintf(stderr, "# frames in a period: %d\n", frames);
    //buffin = malloc(frames*sfinf.channels * sizeof(float));
    buffout = malloc(frames*sfinf.channels * sizeof(float));//frames
    final = malloc(frames*sfinf.channels * sizeof(short));//frames
    
    //next few lines initailise the converter stuff
    statestr = src_new (converter, sfinf.channels, &error);

       //datastr->end_of_input = 0;
    datastr.end_of_input = 0;
    datastr.src_ratio = atof(argv[2]);//hopefully plays at twice the speed
    datastr.data_out = buffout;
    datastr.output_frames = frames;//frames
    //the while loops basically processes the music file
    //
    //
    //
    //
    while(buffin != eofin){
       datastr.data_in = buffin;
      // if (datastr.input_frames == 0 ){datastr.input_frames = frames*MAX_PLAY_RATE;}//frames
       datastr.input_frames = frames*MAX_PLAY_RATE;
       if ((eofin - buffin) < frames*MAX_PLAY_RATE) {
           //datastr.end_of_input = SF_TRUE; // if you read less than a full frame it says i am done
           memset(buffout, 0.0, frames*sfinf.channels*sizeof(float));//frames
           if( (eofin - buffin) < frames) {
               datastr.end_of_input = SF_TRUE;
               break;
           }
       }
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
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffin);
    free(filein);
    free(buffout);
    free(final);
    return 0;
}


