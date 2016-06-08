#include "alsa.h"
#include "log.h"



//alsa player
alsaPlayer::alsaPlayer(int channels, snd_pcm_format_t format, int rate)
{
	channels_ = channels;
	format_ = format;
	sample_rate = rate;
	logger.log_init(WW_LOG_XX::LL_NOTICE, "alsa.log", "./log/");
}

alsaPlayer::~alsaPlayer()
{
	free(p_controlBuffer);
	p_read = nullptr;
	p_write = nullptr;
	snd_pcm_close(handle);
}

int alsaPlayer::Init(snd_pcm_access_t access, snd_pcm_format_t fmt, unsigned int channels,
	unsigned int frequency, unsigned int buffer_time, unsigned int period_time)
{
	int err, dir;
	snd_pcm_hw_params_alloca(&playback_params);

	if ((err = snd_pcm_open(&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		WW_LOG_ERROR(logger,"Playback open error: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Playback open error: %s\n", snd_strerror(err));
#endif 
		return err;
	}
	/* choose all parameters */
	err = snd_pcm_hw_params_any(handle, playback_params);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Broken configuration for PLAYBACK: no configurations available: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Broken configuration for PLAYBACK: no configurations available: %s\n", snd_strerror(err));
#endif 
		return err;
	}
	/* set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, playback_params, access);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Access type not available for  PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Access type not available for  PLAYBACK: %s\n", snd_strerror(err));
#endif 
		return err;
	}
	/* set the sample format */
	err = snd_pcm_hw_params_set_format(handle, playback_params, fmt);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Sample format not available for  PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Sample format not available for  PLAYBACK: %s\n", snd_strerror(err));
#endif
		return err;
	}
	snd_pcm_format_t ft;
	err = snd_pcm_hw_params_get_format(playback_params, &ft);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to get format for  PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Unable to get format for  PLAYBACK: %s\n", snd_strerror(err));
#endif
		return err;
	}
	if (ft != fmt)
	{
		WW_LOG_ERROR(logger,"format doesn't match for  PLAYBACK(get %d):%s\n", ft, snd_strerror(err));
#ifdef DEBUG
		printf("format doesn't match for  PLAYBACK(get %d):%s\n", ft, snd_strerror(err));
#endif
		return -EINVAL;
	}
	/* set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, playback_params, channels);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Channels count (%i) not available for  PLAYBACK: %s\n", channels, snd_strerror(err));
#ifdef DEBUG
		printf("Channels count (%i) not available for  PLAYBACK: %s\n", channels, snd_strerror(err));
#endif 
		return err;
	}
	/* set the stream rate */
	unsigned int rrate = frequency;
	err = snd_pcm_hw_params_set_rate_near(handle, playback_params, &rrate, 0);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Rate %iHz not available for  PLAYBACK: %s\n", frequency, snd_strerror(err));
#ifdef DEBUG
		printf("Rate %iHz not available for  PLAYBACK: %s\n", frequency, snd_strerror(err));
#endif 
		return err;
	}
	if (rrate != frequency)
	{
		WW_LOG_ERROR(logger,"Rate doesn't match (requested %iHz, get %iHz)\n", frequency, err);
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", frequency, err);
		return -EINVAL;
	}
	/* set the buffer time */
	unsigned int time_temp = buffer_time;
	err = snd_pcm_hw_params_set_buffer_time_near(handle, playback_params, &time_temp, &dir);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to set buffer time %i for  PLAYBACK: %s\n", buffer_time, snd_strerror(err));
#ifdef DEBUG
		printf("Unable to set buffer time %i for  PLAYBACK: %s\n", buffer_time, snd_strerror(err));
#endif 
		return err;
	}
	if (time_temp != buffer_time)
	{
		WW_LOG_ERROR(logger,"buffer_time doesn't match (requested %d, get %i)\n", buffer_time, time_temp);
#ifdef DEBUG
		printf("buffer_time doesn't match (requested %d, get %i)\n", buffer_time, time_temp);
#endif 
		return -EINVAL;
	}
	/* set the period time */
	time_temp = period_time;
	err = snd_pcm_hw_params_set_period_time_near(handle, playback_params, &time_temp, &dir);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to set period time %i for  PLAYBACK: %s\n", period_time, snd_strerror(err));
#ifdef DEBUG
		printf("Unable to set period time %i for  PLAYBACK: %s\n", period_time, snd_strerror(err));
#endif 
		return err;
	}
	if (time_temp != period_time)
	{
		WW_LOG_ERROR(logger,"period_time doesn't match (requested %d, get %i)\n", period_time, time_temp);
#ifdef DEBUG
		printf("period_time doesn't match (requested %d, get %i)\n", period_time, time_temp);
#endif
		return -EINVAL;
	}
	/* write the parameters to device */
	err = snd_pcm_hw_params(handle, playback_params);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to set params(Init) for PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Unable to set params(Init) for PLAYBACK: %s\n", snd_strerror(err));
#endif 
		return err;
	}

	snd_pcm_uframes_t size;
	err = snd_pcm_hw_params_get_buffer_size(playback_params, &size);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to get buffer size for  PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Unable to get buffer size for  PLAYBACK: %s\n", snd_strerror(err));
#endif 
		return err;
	}
	buffer_size = size;
#ifdef DEBUG
	printf("buffer_size:%d\n", buffer_size);
#endif // DEBUG

	err = snd_pcm_hw_params_get_period_size(playback_params, &size, &dir);
	if (err < 0)
	{
		WW_LOG_ERROR(logger,"Unable to get period size for  PLAYBACK: %s\n", snd_strerror(err));
#ifdef DEBUG
		printf("Unable to get period size for  PLAYBACK: %s\n", snd_strerror(err));
#endif 
		return err;
	}
	period_size = size;
#ifdef DEBUG
	printf("period_size :%d\n", period_size);
#endif
	return 0;
}

int alsaPlayer::play(char * buffer, snd_pcm_uframes_t size)
{
	while (size > 0)
	{
		int err = snd_pcm_mmap_writei(handle, buffer, size);
		/*if (err = -EAGAIN)
			continue;*/
		if (err < 0)
		{
			if (underrun_recovery(err) < 0)
			{
				WW_LOG_ERROR(logger,"Write error: %s\n", snd_strerror(err));
#ifdef DEBUG
				printf("Write error: %s\n", snd_strerror(err));
#endif
				return err;
			}
			break;
		}
		if (size == err)
			break;
		buffer += err * channels_ * (snd_pcm_format_physical_width(format_) / 8);
		size -= err;
	}
	return 0;
}

void alsaPlayer::play_remainder()
{
	snd_pcm_drain(handle);
}

int alsaPlayer::get_freeFrames()
{
	auto avail = snd_pcm_avail_update(handle);
	return avail;
}

void alsaPlayer::reset()
{
	p_write = nullptr;
}

int alsaPlayer::latency_status()
{
	if (p_write == nullptr)
		return 2;
	int  latency;
	auto latency_temp = p_write - p_read;//单位是字节
	if (latency_temp > controlBuffer_size / 2)
	{
		latency = latency_temp - controlBuffer_size;
	}
	else if (latency_temp<controlBuffer_size / 2 && latency_temp>-(controlBuffer_size / 2))
	{
		latency = latency_temp;
	}
	else
	{
		latency = latency_temp + controlBuffer_size;
	}
	int max_latencySize = max_latencyTime_*sample_rate / 1000 * channels_*(snd_pcm_format_physical_width(format_) / 8);
	int max_resetSize = max_resetTime_*sample_rate / 1000 * channels_*(snd_pcm_format_physical_width(format_) / 8);
	int min_reduceSize = min_reduceTime_*sample_rate / 1000 * channels_*(snd_pcm_format_physical_width(format_) / 8);
	int max_addSize = max_addTime_*sample_rate / 1000 * channels_*(snd_pcm_format_physical_width(format_) / 8);
	if (latency > max_latencySize)
	{
		re_flag = true;
		return 1;
	}
	else if (latency > min_reduceSize)
	{
		if (re_flag)
			return 1;
		else
			return 0;
	}
	else if (latency > max_addSize)
	{
		add_flag = false;
		re_flag = false;
		return 0;
	}
	else if (latency > 0)
	{
		if (add_flag)
			return -1;
		else
			return 0;
	}
	else if (latency >= -max_resetSize)
	{
		add_flag = true;
		return -1;
	}
	else
	{
		//WW_LOG_ERROR("latency:%d %d", latency, max_resetSize);
		return 3;
	}
}

int alsaPlayer::ControlBufferInit(int controlBuffer_time, int max_latencyTime, int max_resetTime, int min_reduceTime, int max_addTime)
{

	auto frames = controlBuffer_time*sample_rate / 1000;
	controlBuffer_size = frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
	p_controlBuffer = (char*)malloc(controlBuffer_size);
	if (p_controlBuffer == NULL)
		return -1;
	memset(p_controlBuffer, 0, controlBuffer_size);
	p_write = nullptr;
	p_read = p_controlBuffer;
	controlBuffer_time_ = controlBuffer_time;
	max_latencyTime_ = max_latencyTime;
	max_resetTime_ = max_resetTime;
	min_reduceTime_ = min_reduceTime;
	max_addTime_ = max_addTime;
	add_flag = false;
	re_flag = false;
	return 0;
}

void alsaPlayer::writeData(char * buf, int frames)
{
	auto p_bufTemp = buf;
	auto p_temp = p_write;
	p_temp = p_temp + frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
	if ((p_temp - p_controlBuffer) <= controlBuffer_size) //未绕回
	{
		/*	if ((p_write - p_read<0 && p_write - p_read>-(controlBuffer_size / 2)) || (p_write - p_read > controlBuffer_size / 2))
				p_write = p_write + frames*channels_*(snd_pcm_format_physical_width(format_) / 8);*/
		memcpy(p_write, p_bufTemp, frames*channels_*(snd_pcm_format_physical_width(format_) / 8));
		p_write = p_write + frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
	}
	else//绕回
	{
		auto extra_buffer_size = p_temp - p_controlBuffer - controlBuffer_size;
		auto extra_frames = extra_buffer_size / (channels_*(snd_pcm_format_physical_width(format_) / 8));
		auto no_extra_frames = frames - extra_frames;
		//处理未绕回部分
		if (no_extra_frames)
		{
			memcpy(p_write, p_bufTemp, no_extra_frames*channels_*(snd_pcm_format_physical_width(format_) / 8));
			p_bufTemp = p_bufTemp + no_extra_frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
		}
		p_write = p_controlBuffer;
		//处理绕回部分
		memcpy(p_write, p_bufTemp, extra_frames*channels_*(snd_pcm_format_physical_width(format_) / 8));
		p_write = p_write + extra_frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
	}
}

void alsaPlayer::readControlData_and_play(int frames)
{
	auto p_temp = p_read;
	p_temp = p_temp + frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
	if ((p_temp - p_controlBuffer) <= controlBuffer_size) //未绕回
	{
		play(p_read, frames);
		p_read = p_read + frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
		//采用向前置0,大小为controlBuffer_size / 2
		auto front_size = p_read - p_controlBuffer;
		if (front_size > controlBuffer_size / 2)
		{
			memset(p_read - controlBuffer_size / 2, 0, controlBuffer_size / 2);
		}
		else
		{
			memset(p_controlBuffer, 0, front_size);
			memset(p_read + controlBuffer_size / 2, 0, controlBuffer_size / 2 - front_size);
		}
	}
	else //绕回
	{
		auto extra_buffer_size = p_temp - p_controlBuffer - controlBuffer_size;
		auto extra_frames = extra_buffer_size / (channels_*(snd_pcm_format_physical_width(format_) / 8));
		auto no_extra_frames = frames - extra_frames;
		//处理未绕回部分
		if (no_extra_frames)
		{
			play(p_read, no_extra_frames);
		}
		p_read = p_controlBuffer;

		//处理绕回部分
		play(p_read, extra_frames);
		p_read = p_read + extra_frames*channels_*(snd_pcm_format_physical_width(format_) / 8);
		auto front_size = p_read - p_controlBuffer;
		if (front_size > controlBuffer_size / 2)
		{
			memset(p_read - controlBuffer_size / 2, 0, controlBuffer_size / 2);
		}
		else
		{
			memset(p_controlBuffer, 0, front_size);
			memset(p_read + controlBuffer_size / 2, 0, controlBuffer_size / 2 - front_size);
		}
	}
}

void alsaPlayer::pwrite_init(int latencyTime)
{
	int latencySize = latencyTime*sample_rate / 1000 * channels_*(snd_pcm_format_physical_width(format_) / 8);
	auto p_temp = p_read;
	p_temp = p_temp + latencySize;
	if ((p_temp - p_controlBuffer) <= controlBuffer_size) //未绕回
	{
		p_write = p_read + latencySize;
	}
	else//绕回
	{
		auto extra_buffer_size = p_temp - p_controlBuffer - controlBuffer_size;
		p_write = p_controlBuffer + extra_buffer_size;
	}
}

int alsaPlayer::getLatencyTime()
{
	if (p_write == nullptr)
		return 0;
	int  latency;
	auto latency_temp = p_write - p_read;//单位是字节
	if (latency_temp > controlBuffer_size / 2)
	{
		latency = latency_temp - controlBuffer_size;
	}
	else if (latency_temp<controlBuffer_size / 2 && latency_temp>-(controlBuffer_size / 2))
	{
		latency = latency_temp;
	}
	else
	{
		latency = latency_temp + controlBuffer_size;
	}
	auto latencyTime = latency * 1000 / (channels_ * 48000 * (snd_pcm_format_physical_width(format_) / 8));
	return latencyTime;
}

int alsaPlayer::underrun_recovery(int err)
{
	if (err == -EPIPE)
	{    /* under-run */
		err = snd_pcm_prepare(handle);
		if (err < 0)
			WW_LOG_ERROR(logger,"Can't recovery from overrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	}
	else if (err == -ESTRPIPE)
	{
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);       /* wait until the suspend flag is released */
		if (err < 0)
		{
			err = snd_pcm_prepare(handle);
			if (err < 0)
				WW_LOG_ERROR(logger,"Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}



//alsa timer
alsaTimer::~alsaTimer()
{
	snd_timer_close(handle);
}

int alsaTimer::Init(int intervals)//ms
{
	int  err;
	int classS = SND_TIMER_CLASS_GLOBAL;
	int sclass = SND_TIMER_CLASS_NONE;
	int card = 0;
	int device = SND_TIMER_GLOBAL_SYSTEM;
	int subdevice = 0;

	snd_timer_id_t *id;
	snd_timer_info_t *info;
	snd_timer_params_t *params;
	char timername[64];
	snd_timer_id_alloca(&id);
	snd_timer_info_alloca(&info);
	snd_timer_params_alloca(&params);

	if (classS == SND_TIMER_CLASS_SLAVE && sclass == SND_TIMER_SCLASS_NONE)
	{
		WW_LOG_ERROR(logger,"slave class is not set\n");
#ifdef DEBUG
		printf("slave class is not set\n");
#endif 
		return -1;
	}

	sprintf(timername, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i", classS, sclass, card, device, subdevice);
	if ((err = snd_timer_open(&handle, timername, SND_TIMER_OPEN_NONBLOCK)) < 0)
	{
		WW_LOG_ERROR(logger,"timer open %i (%s)\n", err, snd_strerror(err));
#ifdef DEBUG
		fprintf(stderr, "timer open %i (%s)\n", err, snd_strerror(err));
#endif 
		return err;
	}

	if ((err = snd_timer_info(handle, info)) < 0)
	{
		WW_LOG_ERROR(logger,"timer info %i (%s)\n", err, snd_strerror(err));
#ifdef DEBUG
		fprintf(stderr, "timer info %i (%s)\n", err, snd_strerror(err));
#endif 
		return err;
	}

	int HZ = 1000 / intervals;
	snd_timer_params_set_auto_start(params, 1);//timer自动启动 第二个参数为bool ---1 自启动  
	if (!snd_timer_info_is_slave(info))
	{
		snd_timer_params_set_ticks(params, (1000000000 / snd_timer_info_get_resolution(info)) / HZ);
		auto t = snd_timer_params_get_ticks(params);
		if (snd_timer_params_get_ticks(params) != intervals)
		{
			WW_LOG_ERROR(logger,"intervals does not match,get(%d)\n", t);
#ifdef DEBUG
			printf("intervals does not match,get(%d)\n", t);
#endif
			return -EINVAL;
		}
	}
	else
	{
		WW_LOG_ERROR(logger,"timer is slave");
#ifdef DEBUG
		printf("timer is slave");
#endif
		return -2;
	}
	if ((err = snd_timer_params(handle, params)) < 0)
	{
		WW_LOG_ERROR(logger,"timer params %i (%s)\n", err, snd_strerror(err));
#ifdef DEBUG
		fprintf(stderr, "timer params %i (%s)\n", err, snd_strerror(err));
#endif
		return err;
	}

	if ((err = snd_timer_start(handle)) < 0)
	{
#ifdef DEBUG
		fprintf(stderr, "timer start %i (%s)\n", err, snd_strerror(err));
#endif
		WW_LOG_ERROR(logger,"timer start %i (%s)\n", err, snd_strerror(err));
		return err;
	}
	return 0;
}

int alsaTimer::timer_loop(int timeout, const TimerCallback & cb_)
{
	int count, err;
	struct pollfd *fds;
	snd_timer_read_t tr;

	count = snd_timer_poll_descriptors_count(handle);
	fds = (pollfd *)calloc(count, sizeof(struct pollfd));
	if (fds == NULL)

	{
#ifdef DEBUG
		fprintf(stderr, "malloc error\n");
#endif 
		WW_LOG_ERROR(logger,"pollfd malloc error\n");
		return -1;
	}
	while (1)
	{
		if ((err = snd_timer_poll_descriptors(handle, fds, count)) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "snd_timer_poll_descriptors error: %s\n", snd_strerror(err));
#endif 
			WW_LOG_ERROR(logger,"snd_timer_poll_descriptors error: %s\n", snd_strerror(err));
			return err;
		}
		if ((err = poll(fds, count, timeout)) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "poll error %i (%s)\n", err, strerror(err));
#endif
			WW_LOG_ERROR(logger,"poll error %i (%s)\n", err, strerror(err));
			return err;
		}
		if (err == 0)
		{
#ifdef DEBUG
			fprintf(stderr, "timer time out!!\n");
#endif 
			WW_LOG_ERROR(logger,"timer time out!!");
			return -2;
		}
		timeval val;

		while (snd_timer_read(handle, &tr, sizeof(tr)) == sizeof(tr))
		{
			//gettimeofday(&val, NULL);
			//printf("%lf\n",val.tv_sec+val.tv_usec/1000000.0);
			cb_();
		}
	}
	free(fds);
	return 0;
}
