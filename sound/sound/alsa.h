#pragma once
extern "C"
{
#include<alsa/asoundlib.h>
}
#include<functional>
//alsa play
class alsaPlayer
{
public:
	alsaPlayer(int channels, snd_pcm_format_t format, int rate);
	~alsaPlayer();
	int Init(snd_pcm_access_t access, snd_pcm_format_t fmt,
		unsigned int channels, unsigned int frequency, unsigned int buffer_time, unsigned int period_time);
	int play(char* buffer, snd_pcm_uframes_t size);
	int get_freeFrames();
	void play_remainder();

	//control_buffer
	void reset();
	int latency_status();//-1 扩充，0写入，1缩减，2 p_write为nullptr, 3 重置
	int ControlBufferInit(int controlBuffer_time, int max_latencyTime, 
		int max_resetTime, int min_reduceTime, int max_addTime);
	void writeData(char* buf, int frames);
	void readControlData_and_play(int frames);
	void pwrite_init(int latencyTime);

	int getLatencyTime();
private:
	int underrun_recovery(int err);
private:
	snd_pcm_t* handle;//pci设备句柄
	snd_pcm_hw_params_t *playback_params;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	int channels_;
	int sample_rate;//采样率
	snd_pcm_format_t format_;

//control_buffer	
private:
	char* p_write;
	char* p_read;
	char* p_controlBuffer;
	int controlBuffer_time_;//controlBuffer 大小 ms
	int controlBuffer_size;//controlBuffer 大小 字节
	bool add_flag;//扩充
	bool re_flag;//缩减
	int max_latencyTime_;//最大延时上限 ms
	int max_resetTime_;//最大重置 上限 ms
	int min_reduceTime_;    //最下限缩减到该值 ms
	int max_addTime_;//最上限扩充到该值 ms
};



//alsa timer
class alsaTimer
{
public:
	typedef std::function<void()> TimerCallback;
public:
	alsaTimer() {};
	~alsaTimer();
	int Init(int intervals);//ms
	int timer_loop(int timeout, const TimerCallback& cb_);
private:
	snd_timer_t *handle;
};
