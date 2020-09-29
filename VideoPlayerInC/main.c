// tutorial02.c
// A pedagogical video player that will stream through every video frame as fast as it can.
//
// This tutorial was written by Stephen Dranger (dranger@gmail.com).
//
// Code based on FFplay, Copyright (c) 2003 Fabrice Bellard,
// and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
//
// Use the Makefile to build all examples.
//
// Run using
// tutorial02 myvideofile.mpg
//
// to play the video stream on your screen.


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

//#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
//#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVPacket        packet;
    int             frameFinished;
    //float           aspect_ratio;
    
    AVDictionary    *optionsDict = NULL;
    struct SwsContext *sws_ctx = NULL;
    
    SDL_Overlay     *bmp = NULL;
    SDL_Surface     *screen = NULL;
    SDL_Rect        rect;
    SDL_Event       event;
    
    if(argc < 2) {
        fprintf(stderr, "Usage: test <file>\n");
        exit(1);
    }
    // Register all formats and codecs
    av_register_all();
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }
    
    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
        return -1; // Couldn't open file
    
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information
    
    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    
    // Find the first video stream
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1; // Didn't find a video stream
    
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
        return -1; // Could not open codec
    
    // Allocate video frame
    pFrame=av_frame_alloc();
    
    // Make a screen to put our video
#ifndef __DARWIN__
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
    if(!screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }
    
    // Allocate a place to put our YUV image on that screen
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
                               pCodecCtx->height,
                               SDL_YV12_OVERLAY,
                               screen);
    
    sws_ctx =
    sws_getContext
    (
     pCodecCtx->width,
     pCodecCtx->height,
     pCodecCtx->pix_fmt,
     pCodecCtx->width,
     pCodecCtx->height,
     AV_PIX_FMT_YUV420P,
     SWS_BILINEAR,
     NULL,
     NULL,
     NULL
     );
    
    // Read frames and save first five frames to disk
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                                  &packet);
            
            // Did we get a video frame?
            if(frameFinished) {
                SDL_LockYUVOverlay(bmp);
                
                AVPicture pict;
                pict.data[0] = bmp->pixels[0];
                pict.data[1] = bmp->pixels[2];
                pict.data[2] = bmp->pixels[1];
                
                pict.linesize[0] = bmp->pitches[0];
                pict.linesize[1] = bmp->pitches[2];
                pict.linesize[2] = bmp->pitches[1];
                
                // Convert the image into YUV format that SDL uses
                sws_scale
                (
                 sws_ctx,
                 (uint8_t const * const *)pFrame->data,
                 pFrame->linesize,
                 0,
                 pCodecCtx->height,
                 pict.data,
                 pict.linesize
                 );
                
                SDL_UnlockYUVOverlay(bmp);
                
                rect.x = 0;
                rect.y = 0;
                rect.w = pCodecCtx->width;
                rect.h = pCodecCtx->height;
                SDL_DisplayYUVOverlay(bmp, &rect);
                
            }
        }
        
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        SDL_PollEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;
            default:
                break;
        }
        
    }
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
    
    return 0;
}

////
////  main.c
////  VideoPlayerInC
////
////  Created by Purple on 28/08/2018.
////  Copyright © 2018 Purple. All rights reserved.
////
//
//#include <stdio.h>
//
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
//
//#include "SDL.h"
//#include "SDL_thread.h"
//
//#undef main
//
//#include <assert.h>
//
//
//// compatibility with newer API
//#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
//#define av_frame_alloc avcodec_alloc_frame
//#define av_frame_free avcodec_free_frame
//#endif
//
//#define defer_merge(a,b) a##b
//#define defer_varname(a) defer_merge(defer_scopevar_, a)
//#define defer __attribute__((__unused__ __cleanup__(defer_cleanup))) void (^defer_varname(__COUNTER__))(void) = ^
//static inline void
//defer_cleanup (void (^*b) (void)) {
//    (*b) ();
//}
//#define destructor(destructor) __attribute__((__cleanup__(destructor)))
//
//
//#define guard(condition) if(condition){}
//
//#define let const __auto_type
//#define var __auto_type
//
//typedef _Bool Bool;
//
//static let true = 1;
//
//static let false = 0;
//
//static let noErr = 0;
//
//static let nil = NULL;
//
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wdeprecated-declarations"
//
//
//void saveFrame(const AVFrame* const frame, const int width, const int height, const int iFrame);
//
//
//int main(int argc, const char* argv[]) {
//
//    guard (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == noErr) else {
//        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
//        return -1;
//    }
//
//
//    let fileName = "bunny.mp4";
//
//    av_register_all();
//
//    AVFormatContext* formatContext = nil;
//
//    guard (avformat_open_input(&formatContext, fileName, nil, nil) == noErr) else {
//        return -1;
//    }
//
//    assert(formatContext != nil);
//
//    defer { avformat_close_input((AVFormatContext** )&formatContext); };
//
//    guard (avformat_find_stream_info(formatContext, nil) == noErr) else {
//        return -1;
//    }
//
//    av_dump_format(formatContext, 0, fileName, 0);
//
//    var videoStream = -1;
//
//    for(var i = 0 ; i < formatContext -> nb_streams; i += 1) {
//
//        if(formatContext -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_VIDEO) {
//            videoStream = i;
//            break;
//        }
//
//    }
//
//    if(videoStream == -1) {
//        return -1;
//    }
//
//    let codecContextOriginal = formatContext -> streams[videoStream] -> codec;
//
//    defer { avcodec_close(codecContextOriginal); };
//
//    let codec = avcodec_find_decoder(codecContextOriginal -> codec_id);
//
//    if(codec == nil) {
//        fprintf(stderr, "Unsupported codec\n");
//        return -1;
//    }
//
//    let codecContext = avcodec_alloc_context3(codec);
//
//    defer { avcodec_close(codecContext); };
//
//    guard (avcodec_copy_context(codecContext, codecContextOriginal) == noErr) else {
//        fprintf(stderr, "Couldn't copy codec context\n");
//        return -1;
//    }
//
//    guard (avcodec_open2(codecContext, codec, nil) == noErr) else {
//        return -1;
//    }
//
//    let videoWidth = codecContext -> width;
//
//    let videoHeight = codecContext -> height;
//
//    let screen = SDL_SetVideoMode(videoWidth, videoHeight, 24, 0);
//
//    if(screen == nil) {
//        return -1;
//    }
//
//    let targetFormat = AV_PIX_FMT_RGB24;
//
//    let frame = av_frame_alloc();
//
//    if(frame == nil) {
//        return -1;
//    }
//
//    defer { av_free(frame); };
//
//    let frameRGB = av_frame_alloc();
//
//    if(frameRGB == nil) {
//        return -1;
//    }
//    defer { av_free(frameRGB); };
//
//    let numBytes = avpicture_get_size(targetFormat, videoWidth, videoHeight);
//
//    let buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
//
//    defer { av_free(buffer); };
//
//    guard (avpicture_fill((AVPicture* )frameRGB, buffer, targetFormat, videoWidth, videoHeight) > 0) else {
//        return -1;
//    }
//
//    let swsContext = sws_getContext(videoWidth, videoHeight, codecContext -> pix_fmt, videoWidth, videoHeight, targetFormat, SWS_BILINEAR, nil, nil, nil);
//
//    let bmp = SDL_CreateYUVOverlay(videoWidth, videoHeight, SDL_YV12_OVERLAY, screen);
//
//    int hasFrameFinished = 0;
//
//    AVPacket packet = { 0 };
//
//    while(av_read_frame(formatContext, &packet) == noErr) {
//
//        defer {
//
//            SDL_Event event = { 0 };
//
//            av_free_packet((AVPacket*)&packet);
//
//            SDL_PollEvent(&event);
//
//            switch(event.type) {
//                case SDL_QUIT: {
//                    exit(0);
//                    break;
//                }
//                default: {
//                    break;
//                }
//            }
//
//
//        };
//
//        guard (packet.stream_index == videoStream) else {
//            continue;
//        }
//
//        guard (avcodec_decode_video2(codecContext, frame, &hasFrameFinished, &packet) >= 0) else {
//            return -1;
//        }
//
//        guard (hasFrameFinished) else {
//            continue;
//        }
//
//        SDL_LockYUVOverlay(bmp);
//
//        const AVPicture picture = {
//            .data = {
//                [0] = bmp -> pixels[0],
//                [1] = bmp -> pixels[0],
//                [2] = bmp -> pixels[0],
//            },
//            .linesize = {
//                [0] = bmp -> pitches[0],
//                [1] = bmp -> pitches[2],
//                [2] = bmp -> pitches[1],
//            }
//        };
//
//        sws_scale(swsContext, (const uint8_t * const * )frame -> data,
//                  frame -> linesize, 0, videoHeight,
//                  picture.data, picture.linesize);
//
//        SDL_UnlockYUVOverlay(bmp);
//
//        SDL_Rect rect = { .x = 0, .y = 0, .w = videoWidth, .h = videoHeight };
//
//        SDL_DisplayYUVOverlay(bmp, &rect);
//
//    }
//
//
//
//    return 0;
//}
//
//
//void saveFrame(const AVFrame* const frame, const int width, const int height, const int frameIndex) {
//
//    char fileName[64] = {0, };
//
//    sprintf(fileName, "frame%d.ppm", frameIndex);
//
//    let file = fopen(fileName, "wb");
//
//    if (file == nil) {
//        exit(-1);
//    }
//
//    defer {
//        fclose(file);
//    };
//
//    fprintf(file, "P6\n%d %d\n255\n", width, height);
//
//    let bytesPerRow = frame -> linesize[0];
//
//    let baseAddress = frame -> data[0];
//
//    if(bytesPerRow == width * sizeof(uint8_t)) {
//
//        fwrite(baseAddress, sizeof(uint8_t), bytesPerRow * height, file);
//
//    } else {
//
//        for(var i = 0 ; i < height ; i ++) {
//            fwrite(baseAddress + i * bytesPerRow, sizeof(uint8_t), width * sizeof(uint8_t), file);
//        }
//
//    }
//
//
//}
//
//#pragma clang diagnostic pop
