// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavutil/error.h>

int main(void) {
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_AV1);
    if (decoder == NULL) {
        fputs("FFmpeg has no AV1 decoder; NX2 AV1 movies cannot be decoded\n", stderr);
        return 1;
    }

    AVCodecContext* context = avcodec_alloc_context3(decoder);
    if (context == NULL) {
        fputs("Could not allocate the AV1 decoder context\n", stderr);
        return 1;
    }
    const int result = avcodec_open2(context, decoder, NULL);
    avcodec_free_context(&context);
    if (result < 0) {
        char error[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(result, error, sizeof(error));
        fprintf(stderr, "Could not open AV1 decoder %s: %s\n", decoder->name, error);
        return 1;
    }
    printf("AV1 decoder %s opens successfully\n", decoder->name);
    return 0;
}
