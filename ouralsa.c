// this gonna be our thing

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#define PCM_DEVICE "hw:1,0"
//void playback(char cardname, int freq, int frame, int *buf); 
//void playback(snd_pcm_t *handle, snd_pcm_hw_params_t *params, int *buff);
long fib(long n);
main(int argc, char **argv){
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    //the lbsndfile stuff, one keeps the header the other the data
    SF_INFO sfinf;
    SNDFILE *input = NULL;
    //buffer to pass to the playback function
    short *buff = NULL;
    int pcm, readcount, pcmrc;
    int tmp;
    long tempf;
    float seconds;
    
    // pointer for the file input
    char *inputname = argv[1];
    //int *filein = argv[1];

    input = sf_open(inputname, SFM_READ, &sfinf);
    fprintf(stderr, "Channels : %d\n", sfinf.channels);
    fprintf(stderr, "Sample rate; %d\n", sfinf.samplerate);
    fprintf(stderr, "Sections: %d\n", sfinf.sections);
    fprintf(stderr, "Format: %d\n", sfinf.format);
    //fprintf(stderr, "size: %d\n", sizeof(short));
   //open sndcard
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
    //find size of frames
    snd_pcm_hw_params_get_period_size(params, &frames, 0);
    fprintf(stderr, "# frames in a period: %d\n", frames);
    buff = malloc(frames*sfinf.channels * sizeof(short));
    while((readcount = sf_readf_short(input, buff, frames)) > 0){
        //clock_t start = clock();
        //tempf = fib(25);
        usleep(3113);
        pcmrc = snd_pcm_writei(handle, buff, readcount);
        if (pcmrc == -EPIPE){
            printf("UNDERRUN!\n");
            snd_pcm_prepare(handle);
        }
        else if(pcmrc < 0){
            printf("ERROR WRITING!!!\n");
        }
        else if (pcmrc != readcount){
            printf("WRITE != READ\n");
        }
        
        //clock_t stop = clock();
        //seconds = (float)(stop - start)/CLOCKS_PER_SEC;
        //printf("Result: %.9f\n", seconds);
    }
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buff);
    return 0;
}

long fib(long n){
    if ( n == 0) return 0;
    else if (n == 1) return 1;
    else return (fib(n - 1) + fib(n -2));
}

//void playback(char cardname, int freq, int frame, int *buf){
    //snd_pcm_t *handle;
  //  snd_pcm_hw_params_t *params;
//}
