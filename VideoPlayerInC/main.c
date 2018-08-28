//
//  main.c
//  VideoPlayerInC
//
//  Created by Purple on 28/08/2018.
//  Copyright © 2018 Purple. All rights reserved.
//

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <assert.h>

#define defer_merge(a,b) a##b
#define defer_varname(a) defer_merge(defer_scopevar_, a)
#define defer __attribute__((__unused__ __cleanup__(defer_cleanup))) void (^defer_varname(__COUNTER__))(void) = ^
static inline void
defer_cleanup (void (^*b) (void)) {
    (*b) ();
}
#define destructor(destructor) __attribute__((__cleanup__(destructor)))


#define guard(condition) if(condition){}

#define let const __auto_type
#define var __auto_type

typedef _Bool Bool;

static let true = 1;

static let false = 0;

static let noErr = 0;

static let nil = NULL;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"


void saveFrame(const AVFrame* const frame, const int width, const int height, const int iFrame);


int main(int argc, const char * argv[]) {
    
    let fileName = "bunny.mp4";
    
    av_register_all();
    
    AVFormatContext* formatContext = nil;
    
    guard (avformat_open_input(&formatContext, fileName, nil, nil) == noErr) else {
        return -1;
    }
    
    assert(formatContext != nil);
    
    defer {
        avformat_close_input((AVFormatContext** )&formatContext);
    };
    
    guard (avformat_find_stream_info(formatContext, nil) == noErr) else {
        return -1;
    }
    
    av_dump_format(formatContext, 0, fileName, 0);
    
    var videoStream = -1;
    
    for(var i = 0 ; i < formatContext -> nb_streams; i += 1) {
        
        if(formatContext -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
        
    }
    
    if(videoStream == -1) {
        return -1;
    }
    
    let codecContextOriginal = formatContext -> streams[videoStream] -> codec;
    
    defer {
        avcodec_close(codecContextOriginal);
    };
    
    let codec = avcodec_find_decoder(codecContextOriginal -> codec_id);
    
    if(codec == nil) {
        fprintf(stderr, "Unsupported codec\n");
        return -1;
    }
    
    let codecContext = avcodec_alloc_context3(codec);
    
    defer {
        avcodec_close(codecContext);
    };
    
    guard (avcodec_copy_context(codecContext, codecContextOriginal) == noErr) else {
        fprintf(stderr, "Couldn't copy codec context\n");
        return -1;
    }
    
    guard (avcodec_open2(codecContext, codec, nil) == noErr) else {
        return -1;
    }
    
    let videoWidth = codecContext -> width;
    
    let videoHeight = codecContext -> height;
    
    let targetFormat = AV_PIX_FMT_RGB24;
    
    let frame = av_frame_alloc();
    
    if(frame == nil) {
        return -1;
    }
    defer {
        av_free(frame);
    };
    
    let frameRGB = av_frame_alloc();
    
    if(frameRGB == nil) {
        return -1;
    }
    defer {
        av_free(frameRGB);
    };
    
    let numBytes = avpicture_get_size(targetFormat, videoWidth, videoHeight);
    
    let buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    
    defer {
        av_free(buffer);
    };
    
    guard (avpicture_fill((AVPicture* )frameRGB, buffer, targetFormat, videoWidth, videoHeight) > 0) else {
        return -1;
    }
    
    let swsContext = sws_getContext(videoWidth, videoHeight, codecContext -> pix_fmt, videoWidth, videoHeight, targetFormat, SWS_BILINEAR, nil, nil, nil);
    
    int hasFrameFinished = 0;
    
    AVPacket packet = { 0 };
    
    var frameIndex = 0;
    
    while(av_read_frame(formatContext, &packet) == noErr) {
        
        defer {
            av_free_packet((AVPacket*)&packet);
        };
        
        guard (packet.stream_index == videoStream) else {
            continue;
        }
        
        guard (avcodec_decode_video2(codecContext, frame, &hasFrameFinished, &packet) >= 0) else {
            return -1;
        }
        
        guard (hasFrameFinished) else {
            continue;
        }
        
        guard (sws_scale(swsContext, (const uint8_t * const * )frame -> data,
                         frame -> linesize, 0,
                         videoHeight, frameRGB -> data, frameRGB -> linesize) > 0) else {
            return -1;
        }
        
        frameIndex += 1;
        
        if(frameIndex <= 5) {
            saveFrame(frameRGB, videoWidth, videoHeight, frameIndex);
        }
        
    }
    
    
    
    return 0;
}


void saveFrame(const AVFrame* const frame, const int width, const int height, const int frameIndex) {
    
    char fileName[64] = {0, };
    
    sprintf(fileName, "frame%d.ppm", frameIndex);
    
    let file = fopen(fileName, "wb");
    
    guard (file != nil) else {
        exit(-1);
    }
    
    defer {
        fclose(file);
    };
    
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    
    let bytesPerRow = frame -> linesize[0];
    
    let baseAddress = frame -> data[0];
    
    if(bytesPerRow == width * sizeof(uint8_t)) {
        
        fwrite(baseAddress, sizeof(uint8_t), bytesPerRow * height, file);
        
    } else {
        
        for(var i = 0 ; i < height ; i ++) {
            fwrite(baseAddress + i * bytesPerRow, sizeof(uint8_t), width * sizeof(uint8_t), file);
        }
        
    }
    
    
}

#pragma clang diagnostic pop
