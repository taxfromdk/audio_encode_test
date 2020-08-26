#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>


extern "C"
{
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

}


static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output)
{
    int ret = 0; 

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        exit(1);
    }

    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) 
    {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            //printf("Ending\n");
            return;
        }        
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: %s aac or %s mp3\n", argv[0], argv[0]);
        exit(0);
    }

    const char* filename_aac = "raw.aac";
    const char* filename_mp3 = "raw.mp3";
    const char* filename;
    bool is_aac = false;
    if(strcmp(argv[1], "aac") == 0)
    {
        is_aac = true;
        printf("Using aac\n");
        filename = filename_aac;
    }
    else
    {
        printf("Using mp3\n");
        filename = filename_mp3;
    }

    AVCodec* audio_codec;
    if(is_aac)
    {
        audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }
    else
    {
        audio_codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
    
    if (audio_codec == NULL)
    {
        printf("Could not allocate audio_codec\n");
        exit(1);   
    };
    
    AVCodecContext  *audio_codec_context = avcodec_alloc_context3(audio_codec);
    
    audio_codec_context->bit_rate = 129000;
    audio_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    audio_codec_context->sample_rate = 48000;
    audio_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
    audio_codec_context->channels = 2;
    //audio_codec_context->profile = FF_PROFILE_AAC_MAIN;
    //audio_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    //audio_codec_context->time_base = (AVRational){1, audio_codec_context->sample_rate};
    //audio_codec_context->codec_type = AVMEDIA_TYPE_AUDIO;
    //audio_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(audio_codec_context, audio_codec, nullptr) < 0)
    {
        printf("Could not open audioCodecContext\n");
        exit(1);   
    }
    
    FILE* f = fopen(filename, "wb");

    AVPacket* pkt = av_packet_alloc();
    if(!pkt)
    {
        printf("Could not allocate packet\n");
        exit(1);    
    }


    //Create frame to feed audio data to encoder
    AVFrame *frame = av_frame_alloc();
    if (!frame) 
    {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    frame->nb_samples =     audio_codec_context->frame_size;
    frame->format =         audio_codec_context->sample_fmt;
    frame->channel_layout = audio_codec_context->channel_layout;
    frame->sample_rate    = audio_codec_context->sample_rate;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

    float t = 0.0;
    float tincr = (2.0*M_PI*440) / audio_codec_context->sample_rate;

    //Encode frames
    for(int i=0;i<100;i++) 
    {
        if(i % 1000 == 0)
        {
            printf("audioencode %d\n", i);
        }
        
        ret = av_frame_make_writable(frame);
        if (ret < 0)
        {
            exit(1);
        }

        //Make sound
        for (int j = 0; j < audio_codec_context->frame_size; j++) 
        {
            
            ((float*)frame->data[0])[j] = (sin(t));
            ((float*)frame->data[1])[j] = (sin(t));
            t += tincr;
        }
        
        encode(audio_codec_context, frame, pkt, f);
        
    }
    encode(audio_codec_context, NULL, pkt, f);
    fclose(f);
    
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&audio_codec_context);

    return 0;
    
}

