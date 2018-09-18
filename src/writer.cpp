// Adapted from https://stackoverflow.com/questions/34511312/how-to-encode-a-video-from-several-images-generated-in-a-c-program-without-wri

#include "movie.h"


#include <vector>



// One-time initialization.
class FFmpegInitialize
{
public :

	FFmpegInitialize()
	{
		// Loads the whole database of available codecs and formats.
		av_register_all();
	}
};

static FFmpegInitialize ffmpegInitialize;
//boost::lockfree::spsc_queue<image, boost::lockfree::capacity<100> > spsc_queue;


MovieWriter::MovieWriter()
{
	spsc_queue = new boost::lockfree::spsc_queue<image, boost::lockfree::capacity<50>>;
    videonum = 1;
}

void MovieWriter::init(string filepath, int height, int width, int channel)
{
    this->height = height;
    this->width = width;
    this->channel = channel;
    this->filepath = filepath;
    int ret;
    if(this->channel == 3) {
        // Preparing to convert my generated RGB images to YUV frames.
        swsCtx = sws_getContext(width, height,
                                AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        // Preparing the containers of the frame data:
        // Allocating memory for each RGB frame, which will be lately converted to YUV.
        rgbpic = av_frame_alloc();
        rgbpic->format = AV_PIX_FMT_RGB24;
        rgbpic->width = width;
        rgbpic->height = height;
        ret = av_frame_get_buffer(rgbpic, 1);
    }
    else
    {
        // Preparing to convert my generated IR images to YUV frames.
        swsCtx = sws_getContext(width, height,
                                AV_PIX_FMT_GRAY8, width, height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        // Preparing the containers of the frame data:
        // Allocating memory for each IR frame, which will be lately converted to YUV.
        irpic = av_frame_alloc();
        irpic->format = AV_PIX_FMT_GRAY8;
        irpic->width = width;
        irpic->height = height;
        ret = av_frame_get_buffer(irpic, 1);
    }

    // Allocating memory for each conversion output YUV frame.
    yuvpic = av_frame_alloc();
    yuvpic->format = AV_PIX_FMT_YUV420P;
    yuvpic->width = width;
    yuvpic->height = height;
    ret = av_frame_get_buffer(yuvpic, 1);

    // After the format, code and general frame data is set,
    // we can write the video in the frame generation loop:
    // std::vector<uint8_t> B(width*height*3);

}


void MovieWriter::start() {
    iframe = 0;
    done = false;

    char* fmtext = "mp4";
    const string filename = filepath + "/" + to_string(videonum) + "." + fmtext;

    // Preparing the data concerning the format and codec,
    // in order to write properly the header, frame data and end of file.
    fmt = av_guess_format(fmtext, NULL, NULL);
    avformat_alloc_output_context2(&fc, NULL, NULL, filename.c_str());

    // Setting up the codec.
    AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    AVDictionary* opt = NULL;
    av_dict_set(&opt, "preset", "veryfast", 0);     //编码速度 ultrafast superfast veryfast faster fast medium slow slower veryslow
    av_dict_set(&opt, "crf", "25", 0);               //视频帧率
    // av_dict_set(&opt, "tune", "zerolatency", 0);  //无延迟参数, 实时处理
    stream = avformat_new_stream(fc, codec);
    c = stream->codec;
    c->width = width;
    c->height = height;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->time_base = (AVRational){ 1, 25 };

    // Setting up the format, its stream(s),
    // linking with the codec(s) and write the header.
    if (fc->oformat->flags & AVFMT_GLOBALHEADER)
    {
        // Some formats require a global header.
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);

    // Once the codec is set up, we need to let the container know
    // which codec are the streams using, in this case the only (video) stream.
    stream->time_base = (AVRational){ 1, 25 };
    av_dump_format(fc, 0, filename.c_str(), 1);
    avio_open(&fc->pb, filename.c_str(), AVIO_FLAG_WRITE);
    int ret = avformat_write_header(fc, &opt);
    av_dict_free(&opt);
    boost::function0< void> f = boost::bind(&MovieWriter::addFrame,this);
    thd = new boost::thread(f);
}

void MovieWriter::join()
{
    thd->join();
}

void MovieWriter::addFrame()
{
	image img;
    while(!this->done)
	{
		while(this->spsc_queue->pop(img))
		{
			if (this->channel == 3) {
				// The AVFrame data will be stored as RGBRGBRGB... row-wise,
				// from left to right and from top to bottom.
				for (unsigned int y = 0; y < this->height; y++) {
					for (unsigned int x = 0; x < this->width; x++) {
						// rgbpic->linesize[0] is equal to width * 3.
						this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 0] = img.data[
								y * this->width * 3 + 3 * x + 2];
						this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 1] = img.data[
								y * this->width * 3 + 3 * x + 1];
						this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 2] = img.data[
								y * this->width * 3 + 3 * x + 0];
					}
				}
                // Not actually scaling anything, but just converting
                // the RGB data to YUV and store it in yuvpic.
                sws_scale(this->swsCtx, this->rgbpic->data, this->rgbpic->linesize, 0,
                          this->height, this->yuvpic->data, this->yuvpic->linesize);
			}
			else {
				for (unsigned int y = 0; y < this->height; y++) {
					for (unsigned int x = 0; x < this->width; x++) {
						// irpic->linesize[0] is equal to width.
						this->irpic->data[0][y * this->irpic->linesize[0] + x] = img.data[
								y * this->width + x];
						// this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 1] = img.data[
						// 		y * this->width + x];
						// this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 2] = img.data[
						// 		y * this->width + x];
					}
				}
                // Not actually scaling anything, but just converting
                // the IR data to YUV and store it in yuvpic.
                sws_scale(this->swsCtx, this->irpic->data, this->irpic->linesize, 0,
                          this->height, this->yuvpic->data, this->yuvpic->linesize);
			}
//            cv::Mat im(height,width,CV_8UC1);
//            memcpy(im.data,yuvpic->data[0],height*width);
//            imwrite("testir.jpg",im);
			av_init_packet(&this->pkt);
			this->pkt.data = NULL;
			this->pkt.size = 0;

			// The PTS of the frame are just in a reference unit,
			// unrelated to the format we are using. We set them,
			// for instance, as the corresponding frame number.
			this->yuvpic->pts = this->iframe++;

			int got_output;
			int ret = avcodec_encode_video2(this->c, &this->pkt, this->yuvpic, &got_output);
			//cout<<"iframe: "<<iframe<<endl;
			if (got_output) {
				fflush(stdout);

				// We set the packet PTS and DTS taking in the account our FPS (second argument),
				// and the time base that our selected format uses (third argument).
				av_packet_rescale_ts(&this->pkt, (AVRational) {1, 25}, this->stream->time_base);

				this->pkt.stream_index = this->stream->index;
				printf("Writing frame %d (size = %d)\n", this->iframe, this->pkt.size);

				// Write the encoded frame to the mp4 file.
				av_interleaved_write_frame(this->fc, &this->pkt);
				av_packet_unref(&this->pkt);
			}
		}
	}
	printf("DONE!\n");
    while(this->spsc_queue->pop(img))
    {
        if (channel == 3) {
            // The AVFrame data will be stored as RGBRGBRGB... row-wise,
            // from left to right and from top to bottom.
            for (unsigned int y = 0; y < this->height; y++) {
                for (unsigned int x = 0; x < this->width; x++) {
                    // rgbpic->linesize[0] is equal to width * 3.
                    this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 0] = img.data[
                            y * this->width * 3 + 3 * x + 2];
                    this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 1] = img.data[
                            y * this->width * 3 + 3 * x + 1];
                    this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 2] = img.data[
                            y * this->width * 3 + 3 * x + 0];
                }
            }
            // Not actually scaling anything, but just converting
            // the RGB data to YUV and store it in yuvpic.
            sws_scale(this->swsCtx, this->rgbpic->data, this->rgbpic->linesize, 0,
                      this->height, this->yuvpic->data, this->yuvpic->linesize);
        }
        else {
            for (unsigned int y = 0; y < this->height; y++) {
                for (unsigned int x = 0; x < this->width; x++) {
                    // irpic->linesize[0] is equal to width.
                    this->irpic->data[0][y * this->irpic->linesize[0] + x] = img.data[
                            y * this->width + x];
                    // this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 1] = img.data[
                    // 		y * this->width + x];
                    // this->rgbpic->data[0][y * this->rgbpic->linesize[0] + 3 * x + 2] = img.data[
                    // 		y * this->width + x];
                }
            }
            // Not actually scaling anything, but just converting
            // the IR data to YUV and store it in yuvpic.
            sws_scale(this->swsCtx, this->irpic->data, this->irpic->linesize, 0,
                      this->height, this->yuvpic->data, this->yuvpic->linesize);
        }

        av_init_packet(&this->pkt);
        this->pkt.data = NULL;
        this->pkt.size = 0;

        // The PTS of the frame are just in a reference unit,
        // unrelated to the format we are using. We set them,
        // for instance, as the corresponding frame number.
        this->yuvpic->pts = this->iframe++;

        int got_output;
        int ret = avcodec_encode_video2(this->c, &this->pkt, this->yuvpic, &got_output);
        //cout<<"iframe: "<<iframe<<endl;
        if (got_output) {
            fflush(stdout);

            // We set the packet PTS and DTS taking in the account our FPS (second argument),
            // and the time base that our selected format uses (third argument).
            av_packet_rescale_ts(&this->pkt, (AVRational) {1, 25}, this->stream->time_base);

            this->pkt.stream_index = this->stream->index;
            printf("Writing frame %d (size = %d)\n", this->iframe, this->pkt.size);

            // Write the encoded frame to the mp4 file.
            av_interleaved_write_frame(this->fc, &this->pkt);
            av_packet_unref(&this->pkt);
        }
    }
}

bool MovieWriter::addImage(image i)
{
	int count=0;
	while(!spsc_queue->push(i))
	{
		if(count>300)
		{
			return false;
		}
		usleep(100);
		count++;
		//printf("pushing \n");
	}
	return true;
}

void MovieWriter::stop()
{
    // Writing the delayed frames:
	for (int got_output = 1; got_output; )
	{
		int ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
		if (got_output)
		{
			fflush(stdout);
			av_packet_rescale_ts(&pkt, (AVRational){ 1, 25 }, stream->time_base);
			pkt.stream_index = stream->index;
			printf("Writing frame %d (size = %d)\n", iframe, pkt.size);
			av_interleaved_write_frame(fc, &pkt);
			av_packet_unref(&pkt);
		}
	}
	
	// Writing the end of the file.
	av_write_trailer(fc);

	// Closing the file.
	if (!(fmt->flags & AVFMT_NOFILE))
	    avio_closep(&fc->pb);
	avcodec_close(stream->codec);
	//prepare to write next video
	videonum++;
}

MovieWriter::~MovieWriter()
{	
	// Freeing all the allocated memory:
	sws_freeContext(swsCtx);
	av_frame_free(&rgbpic);
	av_frame_free(&yuvpic);
	avformat_free_context(fc);
}