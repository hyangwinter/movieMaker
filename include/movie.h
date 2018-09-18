// Adapted from https://stackoverflow.com/questions/34511312/how-to-encode-a-video-from-several-images-generated-in-a-c-program-without-wri
#pragma once
#ifndef MOVIE_H
#define MOVIE_H

#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/highgui/highgui.hpp>
/* Header files for boost 1.60.1*/
#include <boost/date_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/lock_factories.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

extern "C"
{
	#include <x264.h>
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libavutil/opt.h>
	#include <librsvg-2.0/librsvg/rsvg.h>
}

#define HEIGHT 960
#define WIDTH 1280
#define CHANNEL 3
using namespace std;

struct image{
	uchar data[HEIGHT*WIDTH*CHANNEL];
};

//extern boost::lockfree::spsc_queue<image, boost::lockfree::capacity<100> > spsc_queue; //, boost::lockfree::capacity<5>

class MovieWriter
{
private:
    unsigned int height;
    unsigned int width;
    unsigned int channel;
	unsigned int iframe;
	unsigned int videonum;
    string filepath;

	SwsContext* swsCtx;
	AVOutputFormat* fmt;
	AVStream* stream;
	AVFormatContext* fc;
	AVCodecContext* c;
	AVPacket pkt;
	AVFrame *rgbpic, *yuvpic, *irpic;
	boost::thread *thd;
    boost::lockfree::spsc_queue<image, boost::lockfree::capacity<50>> *spsc_queue;

public :
    bool done;
	MovieWriter();
	void init(string filepath, int height, int width, int channel);
	void start();
	void join();
	void addFrame();
	bool addImage(image i);
	void stop();
	~MovieWriter();
};

class MovieReader
{
	const unsigned int width, height;

	SwsContext* swsCtx;
	AVOutputFormat* fmt;
	AVStream* stream;
	AVFormatContext* fc;
	AVCodecContext* c;
	AVFrame* pFrame;
	AVFrame* pFrameRGB;
	AVFrame* pFrameIR;


	// The index of video stream.
	int ivstream;
	int framecount;

public :

	MovieReader(const std::string& filename, const unsigned int width, const unsigned int height);

	bool getFrame(std::string &savepath, const int channels, int imgnum);

	~MovieReader();	
};

#endif // MOVIE_H

