#pragma once
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}
#include "log.h"
class Decoder
{
public:
	Decoder(AVSampleFormat fmt, int channels, unsigned int rate);
	~Decoder();
	int Init();
	int decode(const uint8_t * buffer, size_t size);
	char* get_pcmData();
	int get_pcmFrames();
private:
	AVSampleFormat out_fmt;
	int out_channels;
	unsigned int out_rate;
	AVCodec * codec;
	AVCodecContext * pCodecCtx;
	AVCodecParserContext * pCodecParserCtx;
	AVFrame * pFrame;
	AVPacket packet;
	SwrContext * swr_ctx;

	uint8_t* out_buffer;
	char *pcm_buffer;
	int pcm_frames;
private:
	WW_LOG_XX::Logger logger;
};