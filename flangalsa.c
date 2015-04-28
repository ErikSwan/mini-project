// this gonna be our thing

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <samplerate.h>
#include <math.h>   


#define PCM_DEVICE "default"//"hw:1,0" for the dac  "default" for the onbuilt jack
#define MAX_PLAY_RATE 5//basically for buffer management so our input buffer is always big enough for the effective downsample happening
#define converter 1//0 for highest quality(dont do this), 1 for medium(this works), 2 for fastest sinc, 3 for ZOH, 4 for linear

#define PI 3.14159265359 

void flange(float *in, float *out, float flgain, float fdelmax, float flfreq, int size, double samplerate);


main(int argc, char **argv){
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    //the libsamplerate stuff
    SRC_DATA datastr;//holds the data(inpu and output),input frames used, output frames generated 
    SRC_STATE *statestr;//holds the state
    int error;
    //the lbsndfile stuff, one keeps the header the other the data
    SF_INFO sfinf;//has all the info like total frames,samplerate, channels, mode
    SNDFILE *input = NULL;
    //buffers used in computation to pass to the playback function
    float *buffin = NULL;//float into the converter
    float *filein = NULL;
    float *flangebuffout = NULL;//float to traverse the array for getting flange effects
    float *flangebase = NULL;//float to fix the array for initialising the flange effects
    float *buffout = NULL;//float out of the converter
    short *final = NULL;//final array to write to snd card

    float fldelmax = 600;
    float flfreq = 20;
    float flgain = .8;


    int pcm, readcount, pcmrc, blah;//pcm is used for error checking the alsa stuff, pcmrc is used in the main loop
    float *eofin;//to point to the end of the file's raw data
    int tmp;//used to hold information that is printed in stdio
    int i = 0;//index for initialising the output DAC
   // float fldelmax, flgain, flfreq;
    // pointer for the file input
    if (argc != 3){
        printf("Usage requires music file and src ratio, please try again\n\n");
        return 0;
    }
    char *inputname = argv[1];
    //int *filein = argv[1];

    input = sf_open(inputname, SFM_READ, &sfinf);
    if (input == NULL){
        printf("could not open file sorry \n\n");
        return 0;
    }
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
       return 0;
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
    //find size of period on hardware
    snd_pcm_hw_params_get_period_size(params, &frames, 0);
    fprintf(stderr, "# frames in a period: %d\n", frames);
  
    //buffers on buffers (buffout for float output from sample rate conversion), (final for short array to write to pcm) and flangebse for the flanger stuff
    
    buffout = malloc(frames*sfinf.channels * sizeof(float));//frames
    final = malloc(frames*sfinf.channels * sizeof(short));//frames
    flangebase = malloc(frames*sfinf.channels*MAX_PLAY_RATE * sizeof(short));
    if (flangebase == NULL) {
        printf("You could not allocate memory for the flanger base");
        return 0;
    }
    flangebuffout = flangebase;
    


    //next few lines initailise the converter stuff
    statestr = src_new (converter, sfinf.channels, &error);

       //datastr->end_of_input = 0;
    datastr.end_of_input = 0;
    datastr.src_ratio = atof(argv[2]);//hopefully plays at twice the speed
    datastr.data_out = buffout;
    datastr.output_frames = frames;//frames
    while (i != 50){
       memset(final, 0, frames*sfinf.channels);
       pcmrc = snd_pcm_writei(handle, final, frames);//frames
               if (pcmrc == -EPIPE){
            printf("UNDERRUN!\n");
            snd_pcm_prepare(handle);
        }
        else if(pcmrc < 0){
            printf("ERROR WRITING!!!\n");
        }
        i++;
    }
    
    
    i = 0;
    while(buffin != eofin){
      

      // if (datastr.input_frames == 0 ){datastr.input_frames = frames*MAX_PLAY_RATE;}//frames
       if (frames > 5){ //need to add mask of flanger on effect button
           memset(flangebase, 0.0, frames*sfinf.channels*MAX_PLAY_RATE*sizeof(float));
           flangebuffout = flangebase;
           flange(buffin, flangebuffout, flgain, fldelmax,flfreq, frames*MAX_PLAY_RATE, sfinf.samplerate);

       }



       datastr.data_in = flangebase;

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
       i++; 
    }
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffin);
    free(filein);
    free(buffout);
    free(final);
    free(flangebase);
    free(flangebuffout);
    return 0;
}

void flange(float *in, float *out,float flgain , float fldelmax, float flfreq, int size, double samplerate){
    int i = 0;
    int n = 0;
    float lchan, rchan, dlchan, drchan, nlchan, nrchan;
    float delay;
    int idelay;

    for (i = 0; i < 2*size; i+=2){
        lchan = *(in + i);
        rchan = *(in + 1 + i);

        delay = fldelmax*(1 + sin(2*PI*flfreq* (1.0/ samplerate)));
        idelay = (int) floor(delay);
        dlchan = flgain*(*(in + i - idelay));
        nlchan = dlchan + (1 - flgain)*lchan;
        *(out + i) = nlchan;

        drchan = flgain*(*(in + 1 + i - idelay));
        nrchan = drchan + (1 - flgain)*rchan;
        *(out + 1 + i) = nrchan;
        n++;

    }
}





