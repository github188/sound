#include "decoder.h"
#include "log.h"

#if __WORDSIZE == 64
#define UINT64_C(c)	c ## UL
#else
#define UINT64_C(c)	c ## ULL
#endif

Decoder::Decoder(AVSampleFormat fmt, int channels, unsigned int rate)
{
	out_fmt = fmt;
	out_channels = channels;
	out_rate = rate;
	pcm_frames = 0;
	logger.log_init(WW_LOG_XX::LL_NOTICE, "decoder.log", "./log/");
}

Decoder::~Decoder()
{
	free(pcm_buffer);
	av_frame_free(&pFrame);
	swr_free(&swr_ctx);
	av_parser_close(pCodecParserCtx);
	avcodec_free_context(&pCodecCtx);
}

int Decoder::Init()
{
	avcodec_register_all();
	codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		WW_LOG_ERROR(logger,"avcodec_find_decoder(AV_CODEC_ID_AAC) failed");
#ifdef DEBUG
		printf("avcodec_find_decoder(AV_CODEC_ID_AAC) failed");
#endif 
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(codec);
	if (!pCodecCtx)
	{
		WW_LOG_ERROR(logger,"avcodec_alloc_context3(%p) failed", codec);
#ifdef DEBUG
		printf("avcodec_alloc_context3(%p) failed", codec);
#endif
		return -1;
	}

	pCodecParserCtx = av_parser_init(AV_CODEC_ID_AAC);
	if (!pCodecParserCtx)
	{
		WW_LOG_ERROR(logger,"av_parser_init(AV_CODEC_ID_AAC) failed");
#ifdef DEBUG
		printf("av_parser_init(AV_CODEC_ID_AAC) failed");
#endif
		avcodec_free_context(&pCodecCtx);
		return -1;
	}

	if (avcodec_open2(pCodecCtx, codec, NULL) < 0)
	{
		WW_LOG_ERROR(logger,"avcodec_open2(%p, %p, NULL) failed", pCodecCtx, codec);
#ifdef DEBUG
		printf("avcodec_open2(%p, %p, NULL) failed", pCodecCtx, codec);
#endif
		av_parser_close(pCodecParserCtx);
		avcodec_free_context(&pCodecCtx);
		return -1;
	}

	swr_ctx = swr_alloc();
	if (!swr_ctx)
	{
		WW_LOG_ERROR(logger,"swr_alloc() failed");
#ifdef DEBUG
		printf("swr_alloc() failed");
#endif
		return -1;
	}

	pFrame = av_frame_alloc();
	if (!pFrame)
	{
		WW_LOG_ERROR(logger,"av_frame_alloc() failed");
#ifdef DEBUG
		printf("av_frame_alloc() failed");
#endif
		return -1;
	}

	av_init_packet(&packet);
	int size = 200 * out_rate / 1000 * out_channels * 4; //200ms buffer  位深度最大为4字节
	pcm_buffer = (char*)malloc(size);
	if (!pcm_buffer)
	{
		WW_LOG_ERROR(logger,"pcm_buffer no enough memory");
		return -1;
	}
	memset(pcm_buffer, 0, size);
	return 0;
}

int Decoder::decode(const uint8_t * buffer, size_t size)
{
	auto p_temp = pcm_buffer;
	int AllFrames = 0;
	while (size > 0)
	{
		auto len = av_parser_parse2(pCodecParserCtx, pCodecCtx,
			&packet.data, &packet.size, buffer, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
		buffer += len;
		size -= len;
		if (packet.size == 0)
			continue;

		int got_pframe;
		auto ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_pframe, &packet);
		if (ret < 0)
		{
			WW_LOG_ERROR(logger,"avcodec_decode_audio4 error(%d)", ret);
#ifdef DEBUG
			printf("avcodec_decode_audio4 error(%d)", ret);
#endif 
			return -1;
		}
		if (ret != packet.size)
		{
			WW_LOG_ERROR(logger,"The packet has more than one frame!");
#ifdef DEBUG
			printf("The packet has more than one frame!");
#endif 
			return -1;
		}
		if (got_pframe)
		{
			swr_ctx = swr_alloc_set_opts(swr_ctx,
				av_get_default_channel_layout(out_channels),
				out_fmt,
				out_rate,
				av_get_default_channel_layout(pFrame->channels),
				(AVSampleFormat)pFrame->format,
				pFrame->sample_rate,
				0,
				NULL);
			if (!swr_ctx) 
			{
				WW_LOG_ERROR(logger,"swr_alloc_set_opts() error");
				return -1;
			}
				
			swr_init(swr_ctx);
			int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, pFrame->sample_rate) + pFrame->nb_samples,
				out_rate, pFrame->sample_rate, AV_ROUND_UP);
			auto tmp = av_samples_alloc(&out_buffer, NULL, out_channels, out_samples, out_fmt, 0);
			if (tmp < 0)
			{
				WW_LOG_ERROR(logger,"av_samples_alloc() failed");
				return -1;
			}
				
			auto out_frameSize = swr_convert(swr_ctx, &out_buffer, out_samples, (const uint8_t**)pFrame->data, pFrame->nb_samples);
			if (out_frameSize < 0)
			{
				WW_LOG_ERROR(logger,"swr_convert() error");
				return -1;
			}
			int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_frameSize, out_fmt, 1);
			if (out_buffer_size < 0)
			{
				WW_LOG_ERROR(logger,"av_samples_get_buffer_size() failed");
				return -1;
			}
			AllFrames = AllFrames + out_frameSize;
			memcpy(p_temp, out_buffer,out_buffer_size);
			p_temp = p_temp + out_buffer_size;
			av_freep(&out_buffer);
		}
	}
	pcm_frames = AllFrames;
	return 0;
}

char * Decoder::get_pcmData()
{
	return pcm_buffer;
}

int Decoder::get_pcmFrames()
{

	return pcm_frames;
}
