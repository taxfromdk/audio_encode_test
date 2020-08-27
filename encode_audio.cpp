#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

extern "C"
{
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
}

int encoded_pkt_counter = 0;



static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, AVFormatContext* adts_container_ctx)
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
    if(ret == 0) 
    {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return;
        }        
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
        
        //Send AAC to ADTS muxer
        if (av_write_frame(adts_container_ctx, pkt) < 0) 
        {
            printf("Error calling av_write_frame() (error '%s')\n");
        }
        else
        {
            printf( "Encoded AAC packet %d, size=%d\n", encoded_pkt_counter++, pkt->size);
        }
        
        av_packet_unref(pkt);
    }
}

static int write_adts_muxed_data(void *opaque, uint8_t *adts_data, int size)
{
    printf("write_adts_muxed_data\n");
    FILE *f = (FILE *)opaque;
    fwrite(adts_data, 1, size, f);
    return size;
}

int getAudio(float t, float* L, float* R)
{
    float Hz = 440;
    *L = sin(t*2*M_PI*Hz);
    *R = sin(t*2*M_PI*Hz);
}

int main(int argc, char **argv)
{
    int ret;

    AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
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
    if (avcodec_open2(audio_codec_context, audio_codec, nullptr) < 0)
    {
        printf("Could not open audioCodecContext\n");
        exit(1);   
    }
    
    FILE* encoded_audio_file = fopen("AACinADTS.aac", "wb");
    

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

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

    //Create ADTS wrapper
    AVOutputFormat *adts_container = av_guess_format("adts", NULL, NULL);
    if (!adts_container) {
        av_log(NULL, AV_LOG_ERROR, "Could not find adts output format\n");       
        exit(1);
    }     
    
    AVFormatContext *adts_container_ctx = NULL;
    if ((ret = avformat_alloc_output_context2(&adts_container_ctx, adts_container, "", NULL)) < 0) {
        printf("Could not create output context\n");
        exit(1);
    }
    
    size_t adts_container_buffer_size = 4096;
    uint8_t *adts_container_buffer = NULL;
    if (!(adts_container_buffer = (uint8_t*)av_malloc(adts_container_buffer_size))) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate a buffer for the I/O output context\n");       
        exit(1);
    }
    
    AVIOContext *adts_avio_ctx = NULL;
    if (!(adts_avio_ctx = avio_alloc_context(adts_container_buffer, adts_container_buffer_size, 1, encoded_audio_file, NULL , &write_adts_muxed_data, NULL))) {
        av_log(NULL, AV_LOG_ERROR, "Could not create I/O output context\n");
        exit(1);
    }
    adts_container_ctx->pb = adts_avio_ctx;
    
    AVStream *adts_stream = NULL;
    if (!(adts_stream = avformat_new_stream(adts_container_ctx, NULL))) {
        av_log(NULL, AV_LOG_ERROR, "Could not create new stream\n");       
        exit(1);
    }    
    adts_stream->id = adts_container_ctx->nb_streams-1;

    avcodec_parameters_from_context(adts_stream->codecpar, audio_codec_context);    

    // Allocate the stream private data and write the stream header
    if (avformat_write_header(adts_container_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "avformat_write_header() error\n");
        exit(1);
    }
    
    //Time in seconds
    float t = 0.0;

    //Every sample increments time
    float tincr = 1.0 / audio_codec_context->sample_rate;

    //Encode frames
    for(int i=0;i<100;i++) 
    {
        if(i % 10 == 0)
        {
            printf("Sample: %d\n", i);
        }
        
        ret = av_frame_make_writable(frame);
        if (ret < 0)
        {
            exit(1);
        }

        //Make sound
        for (int j = 0; j < audio_codec_context->frame_size; j++) 
        {
            getAudio(t, &((float*)frame->data[0])[j], &((float*)frame->data[1])[j]);             
            t += tincr;
        }
        encode(audio_codec_context, frame, pkt, adts_container_ctx);
    }
    encode(audio_codec_context, NULL, pkt, adts_container_ctx);
    
    //File
    fclose(encoded_audio_file);

    //AAC related
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&audio_codec_context);
    
    //ADTS related
    avformat_free_context(adts_container_ctx);
    av_freep(&adts_avio_ctx);  
    av_freep(&adts_container_buffer); 
    return 0;
}

