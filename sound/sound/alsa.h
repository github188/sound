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
	int latency_status();//-1 ���䣬0д�룬1������2 p_writeΪnullptr, 3 ����
	int ControlBufferInit(int controlBuffer_time, int max_latencyTime, 
		int max_resetTime, int min_reduceTime, int max_addTime);
	void writeData(char* buf, int frames);
	void readControlData_and_play(int frames);
	void pwrite_init(int latencyTime);

	int getLatencyTime();
private:
	int underrun_recovery(int err);
private:
	snd_pcm_t* handle;//pci�豸���
	snd_pcm_hw_params_t *playback_params;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	int channels_;
	int sample_rate;//������
	snd_pcm_format_t format_;

//control_buffer	
private:
	char* p_write;
	char* p_read;
	char* p_controlBuffer;
	int controlBuffer_time_;//controlBuffer ��С ms
	int controlBuffer_size;//controlBuffer ��С �ֽ�
	bool add_flag;//����
	bool re_flag;//����
	int max_latencyTime_;//�����ʱ���� ms
	int max_resetTime_;//������� ���� ms
	int min_reduceTime_;    //��������������ֵ ms
	int max_addTime_;//���������䵽��ֵ ms
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
