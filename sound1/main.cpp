#include"log.h"
#include"alsa.h"
#include "decoder.h"
#include<sys/time.h>
#include<thread>


int main()
{

	WW_LOG_XX::Logger logger;
	logger.log_init(WW_LOG_XX::LL_NOTICE, "sound.log", "./log/");

	int err;
	alsaPlayer player(2, SND_PCM_FORMAT_S16, 48000);
	alsaTimer timer;
	Decoder decoder(AV_SAMPLE_FMT_S16, 2, 48000);

	if ((err = player.Init(SND_PCM_ACCESS_MMAP_INTERLEAVED, SND_PCM_FORMAT_S16, 2, 48000, 40000, 10000)) < 0)
	{
		WW_LOG_ERROR(logger, "Setting of params(play) failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	if ((err = player.ControlBufferInit(1000, 120, 100, 80, 40)) < 0)
	{
		WW_LOG_ERROR(logger, "no enough memory");
		exit(EXIT_FAILURE);
	}
	if ((err = timer.Init(10)) < 0)
	{
		WW_LOG_ERROR(logger, "Setting of params(timer) failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = decoder.Init()) < 0)
	{
		WW_LOG_ERROR(logger, "decoder init() failed");
		exit(EXIT_FAILURE);
	}

	FILE* file = fopen("EFZ(S16,48000,2).aac", "r");
	char*buffer = (char*)malloc(220);
	memset(buffer, 0, 220);
	bool firstDate = true;
	FILE* pfile = fopen("EFZ(S16,48000,2).pcm", "w");

	std::thread thread([&timer, &player, &buffer, &file, &firstDate, &decoder, &pfile]()
	{
		WW_LOG_XX::Logger logger;
		logger.log_init(WW_LOG_XX::LL_NOTICE, "decode_and_play_thread.log", "./");
		timer.timer_loop(40, [&player, &buffer, &file, &firstDate, &decoder, &pfile, &logger]()
		{
			auto len = fread(buffer, 1, 220, file);
			/*if (len > 0)
			fwrite(buffer,len,1,pfile);*/
			if (len > 0)
			{
				auto ret = decoder.decode((uint8_t*)buffer, len);
				if (ret < 0)
				{
					WW_LOG_ERROR(logger, "decode error");
					exit(EXIT_FAILURE);
				}
				if (firstDate)
				{
					player.pwrite_init(100);
					firstDate = false;
				}
				int flag = player.latency_status();
				if (flag == 0)
				{
					auto pcm_buffer = decoder.get_pcmData();
					auto pcm_frame = decoder.get_pcmFrames();
					if (pcm_frame)
					{
						fwrite(pcm_buffer, pcm_frame * 2 * 2, 1, pfile);
						player.writeData(pcm_buffer, pcm_frame);
					}
				}
				if (flag == 3)
				{
					player.reset();
					firstDate = true;
				}
				auto frames = player.get_freeFrames();
				if (frames > 0)
					player.readControlData_and_play(frames);
			}
			else
			{
				int flag = player.latency_status();
				if (flag == 3)
				{
					player.reset();
					firstDate = true;
					WW_LOG_ERROR(logger, "%d\n", player.getLatencyTime());
				}
				auto frames = player.get_freeFrames();
				WW_LOG_ERROR(logger, "XX%d %d", player.getLatencyTime(), frames);
				if (frames > 0)
					player.readControlData_and_play(frames);
			}
		});
	});


	thread.join();
	free(buffer);
	player.play_remainder();
	return 0;
}

//int main()
//{
//	std::thread thr([]() {main1(); });
//	thr.join();
//	return 0;
//}

//int main()
//{
//	_WW_Log_XX::log_init(_WW_Log_XX::LL_ERROR, "sound", ".");
//
//	alsaTimer timer;
//	Decoder decoder(AV_SAMPLE_FMT_S16, 2, 48000);
//	int err;
//	if ((err = decoder.Init()) < 0)
//	{
//		WW_LOG_ERROR("decoder init() failed");
//		exit(EXIT_FAILURE);
//	}
//	if ((err = timer.Init(10)) < 0)
//	{
//		WW_LOG_ERROR("Setting of params(timer) failed: %s\n", snd_strerror(err));
//		exit(EXIT_FAILURE);
//	}
//	FILE* file = fopen("EFZ.aac", "r");
//	char*buffer = (char*)malloc(210);
//	memset(buffer, 0, 210);
//
//	FILE* pfile = fopen("EFZ(S16,48000,2).pcm", "w");
//	int len = 0;
//	int count = 0;
//	timer.timer_loop(40, [&len, &decoder, buffer, file, pfile, &count]()
//	{
//		len = fread(buffer, 1, 210, file);
//		if (len > 0)
//		{
//			auto ret = decoder.decode((uint8_t*)buffer, len);
//			if (ret < 0)
//			{
//				WW_LOG_ERROR("decode error");
//				exit(EXIT_FAILURE);
//			}
//			auto pcm_buffer = decoder.get_pcmData();
//			auto frame = decoder.get_pcmFrames();
//			if (frame)
//			{
//				fwrite(pcm_buffer, frame * 2 * 2, 1, pfile);
//				//printf("%d\n", ++count);
//			}
//		}
//		else
//		{
//			printf("end");
//		}
//	});
//}