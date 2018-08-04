	#include <bits/stdc++.h>
    #include <iostream>
    #include <cstdio>
    #include <cstdlib>
    #include <fstream>
    #include <math.h>

    #define __STDC_CONSTANT_MACROS
    //FFMPEG LIBRARIES
    extern "C"
    {
    #include "libavcodec/avcodec.h"
    #include "libswscale/swscale.h"
    #include "libavutil/opt.h"
    #include "libavutil/common.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/samplefmt.h"
    #include "libavutil/time.h"
    #include "libavformat/avformat.h"
    #include "libavformat/avio.h"
    #include "libavfilter/avfilter.h"
    #include "libavdevice/avdevice.h"
    #include "libavfilter/avfiltergraph.h"
    #include "libavfilter/buffersink.h"
    }

    using namespace std;

    void SaveMyFrame(AVFrame *sAVFrame , int swidth, int sheight, int iFrame)
    {
    FILE *pfile;
    char szFilename[32];
    int y;

    sprintf(szFilename , "frame%d.ppm" , iFrame);
    pfile = fopen(szFilename , "wb");
    if(pfile == NULL)
    {
      cout<<"\n\ncould'nt open file";
      return;
    }

    fprintf(pfile , "P6\n%d %d\n255\n" , swidth , sheight );

    for( y=0; y<sheight; y++)
    {
      fwrite(sAVFrame->data[0]+y*sAVFrame->linesize[0] , 1 , swidth*3 , pfile );
    }

    fclose(pfile);
    }

    int CaptureScene(int VideoStreamIndx ,
                       AVFormatContext *bAVFormatContext ,
                       AVCodecContext *bAVCodecContext,
                       AVCodec *bAVCodec )
    {
      AVPacket bAVPacket;
      AVFrame *bAVFrame = NULL;
      bAVFrame = av_frame_alloc();
      AVFrame *bAVFrameRGB = NULL;
      bAVFrameRGB = av_frame_alloc();

    if(bAVFrame == NULL)
    {
      cout<<"\n\nframe alloc failed";
    }

    if(bAVFrameRGB == NULL)
    {
      cout<<"\n\nframe alloc RGB failed";
    }

      int numBytes;
      uint8_t *buffer = NULL;

    numBytes =  av_image_get_buffer_size(AV_PIX_FMT_RGB24 , bAVCodecContext->width,bAVCodecContext->height, 32);  // avpicture_get_size  deprecated

    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    avpicture_fill((AVPicture *)bAVFrameRGB , buffer , AV_PIX_FMT_RGB24 , bAVCodecContext->width , bAVCodecContext->height);

    int framefinish;
    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext( bAVCodecContext->width,
                              bAVCodecContext->height,
                              bAVCodecContext->pix_fmt,
                              bAVCodecContext->width,
                              bAVCodecContext->height,
                              AV_PIX_FMT_RGB24,
                              SWS_BILINEAR,
                              NULL,NULL,NULL);
    int i =0;

    while(av_read_frame(bAVFormatContext,&bAVPacket) >=0)
    {
          if(bAVPacket.stream_index == VideoStreamIndx)
      {
          avcodec_decode_video2(bAVCodecContext , bAVFrame , &framefinish , &bAVPacket);
          if(framefinish)
        {
          // convert image from native format to RGB
          sws_scale(sws_ctx , (uint8_t const* const *)bAVFrame->data ,
          bAVFrame->linesize , 0, bAVCodecContext->height,
          bAVFrameRGB->data , bAVFrameRGB->linesize);
          // save frame to disk
          if(++i <= 100)
          {
          SaveMyFrame(bAVFrameRGB , bAVCodecContext->width , bAVCodecContext->height , i );
          cout << "saving frame " << i << endl;
          
          }
          else
          {
          break;
          }

        }

      }

    }

    av_free(bAVFrame);
    av_free(bAVFrameRGB);
    }

    int main()
    {

      avdevice_register_all();
      avcodec_register_all();
      av_register_all();

      char *dev_name = "/dev/video0";

     int VideoStreamIndx = -1;
     AVCodecContext *pAVCodecContext = NULL;
     AVCodec *pAVCodec = NULL;
     AVInputFormat *inputFormat =av_find_input_format("v4l2");
     AVDictionary *options = NULL;
     av_dict_set(&options, "framerate", "20", 0);

     AVFormatContext *pAVFormatContext = NULL;

     if(avformat_open_input(&pAVFormatContext, dev_name, inputFormat, NULL) != 0)
     {
       cout<<"\nError : could'nt open video source\n\n";
       return -1;
     }

     if( avformat_find_stream_info( pAVFormatContext , NULL) < 0)
     {
     cout<<"Error : streams not found";
      return -1;
     }

      av_dump_format(pAVFormatContext , 0 , "/dev/video0" , 0 );

     for(int i=0; i<pAVFormatContext->nb_streams ;i++ )
     {
      if( pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) // if video stream found then get the index.
      {
        VideoStreamIndx = i;
        break;
      }
     }

    if((VideoStreamIndx) == -1)
    {
      cout<<"Error : video streams index not found";
      return -1;
    }

    pAVCodecContext = pAVFormatContext->streams[VideoStreamIndx]->codec;

    pAVCodec = avcodec_find_decoder( pAVCodecContext->codec_id );

    if(pAVCodec == NULL)
    {
     fprintf(stderr,"Unsupported codec !");
     return -1;
    }

    int value = avcodec_open2(pAVCodecContext , pAVCodec , NULL);
    if( value < 0)
    {
      cout<<"Error : Could not open codec";
      return -1;
    }

    int Vwidth , Vheight , videoFPS , videoBaseTime , duration_2 ;
    int sframe , length , Fheight;

    /*
    To fetch/display some media information programatically
    */

    //int64_t duration_1 = pAVFormatContext->duration;
    videoFPS = av_q2d(pAVFormatContext->streams[VideoStreamIndx]->r_frame_rate);
    videoBaseTime = av_q2d(pAVFormatContext->streams[VideoStreamIndx]->time_base);
    Vwidth = pAVFormatContext->streams[VideoStreamIndx]->codec->width;
    Vheight = pAVFormatContext->streams[VideoStreamIndx]->codec->height;
    //duration_2 = (unsigned long)pAVFormatContext->streams[VideoStreamIndx]->duration*(videoFPS*videoBaseTime);

    cout<<"Video FPS :"<<videoFPS;
    cout<<"\n\n width : "<<Vwidth;
    cout<<"\n\n height : "<<Vheight;
    cout<<"\n\n time base"<<videoBaseTime;
    //cout<<"\n\nduration (1): "<<duration_1;
    //cout<<"\n\nduration (2): "<<duration_2;

    CaptureScene( VideoStreamIndx , pAVFormatContext , pAVCodecContext , pAVCodec );

    avcodec_close(pAVCodecContext);
    avformat_close_input(&pAVFormatContext);

    return 0;
    }
