#include"log.h"
#include"alsa.h"
#include <thread>

#include<sys/time.h>
int main()
{
	_WW_Log_XX::log_init(_WW_Log_XX::LL_ERROR, "sound", ".");
	int err;
	alsaPlayer player(2, SND_PCM_FORMAT_S16, 48000);
	alsaTimer timer;

	if ((err = player.Init(SND_PCM_ACCESS_MMAP_INTERLEAVED, SND_PCM_FORMAT_S16, 2, 48000, 40000, 10000)) < 0)
	{
		printf("Setting of params(play) failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	if ((err = player.ControlBufferInit(1000, 120, 100, 80, 40)) < 0)
	{
		WW_LOG_ERROR("no enough memory");
		exit(EXIT_FAILURE);
	}

	if ((err = timer.Init(10)) < 0)
	{
		printf("Setting of params(timer) failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	FILE* file = fopen("Shayne Ward-No Promises_48000.wav", "r");
	char*buffer = (char*)malloc(2 * 2 * 1920);
	memset(buffer, 0, 2 * 2 * 1920);
	bool firstDate = true;

	std::thread thread([&timer,&firstDate, &player, buffer,file]() 
	{
		timer.timer_loop(40, [&player, buffer, file, &firstDate]()
		{
			auto len = fread(buffer, 1, 480 * 2 * 2, file);
			if (len > 0)
			{
				if (firstDate)
				{
					player.pwrite_init(100);
					firstDate = false;
				}
				int flag = player.latency_status();
				if (flag == 0)
					player.writeData(buffer, 480);
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
					WW_LOG_ERROR("%d\n", player.getLatencyTime());
				}

				auto frames = player.get_freeFrames();
				WW_LOG_ERROR("XX%d %d", player.getLatencyTime(), frames);
				if (frames > 0)
					player.readControlData_and_play(frames);
			}
			/*struct timeval val;
			gettimeofday(&val,NULL);
			printf("%lf\n", val.tv_sec + val.tv_usec / 1000000.0);*/
		});
	});
	
	thread.join();
	free(buffer);
	//player.play_remainder();
	return 0;
}