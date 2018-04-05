// CGaveFile.cpp : 定义控制台应用程序的入口点。
//
/* Copyright [c] 2017-2027 By Gang.Wang Allrights Reserved 
   This file give a simple example of how to save net stream to 
   local file. Any questions, you can join QQ group for help, QQ
   Group number:127903734.
*/
#include "stdafx.h"
#include "RtspShow.h"
#include <string>
#include <memory>
#include <thread>
#include <iostream>
using namespace std;

AVFormatContext *inputContext = nullptr;
AVFormatContext * outputContext;
int64_t lastReadPacktTime ;
SwsContext      *img_convert_ctx = NULL;
AVFrame         *pFrame = NULL;
AVFrame         *pFrameRGB = NULL;

AVCodec *pVideoCodec = NULL;
AVCodec *pAudioCodec = NULL;
AVCodecContext *pVideoCodecCtx = NULL;
AVCodecContext *pAudioCodecCtx = NULL;
AVIOContext * pb = NULL;
AVInputFormat *piFmt = NULL;

//using namespace cv;

static int interrupt_cb(void *ctx)
{
	int  timeout  = 3;
	if(av_gettime() - lastReadPacktTime > timeout *1000 *1000)
	{
		return -1;
	}
	return 0;
}
int OpenInput(string inputUrl)
{
	inputContext = avformat_alloc_context();	
	lastReadPacktTime = av_gettime();
	inputContext->interrupt_callback.callback = interrupt_cb;
	int ret = avformat_open_input(&inputContext, inputUrl.c_str(), nullptr,nullptr);
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
		return  ret;
	}
	ret = avformat_find_stream_info(inputContext,nullptr);
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
	}
	else
	{
		av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n",inputUrl.c_str());
	}
	return ret;
}

shared_ptr<AVPacket> ReadPacketFromSource()
{
	shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
	av_init_packet(packet.get());
	lastReadPacktTime = av_gettime();
	int ret = av_read_frame(inputContext, packet.get());
	if(ret >= 0)
	{
		return packet;
	}
	else
	{
		return nullptr;
	}
}
void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb)
{
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
	if (pkt->duration > 0)
		pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}
int WritePacket(shared_ptr<AVPacket> packet)
{
	auto inputStream = inputContext->streams[packet->stream_index];
	auto outputStream = outputContext->streams[packet->stream_index];				
	av_packet_rescale_ts(packet.get(),inputStream->time_base,outputStream->time_base);
	return av_interleaved_write_frame(outputContext, packet.get());
}

int OpenOutput(string outUrl)
{


	int ret  = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", outUrl.c_str());
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
		goto Error;
	}

	ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);	
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open avio failed");
		goto Error;
	}

	for(int i = 0; i < inputContext->nb_streams; i++)
	{
		AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);				
		ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);	
		if(ret < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
			goto Error;
		}
	}

	ret = avformat_write_header(outputContext, nullptr);
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "format write header failed");
		goto Error;
	}

	av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n",outUrl.c_str());			
	return ret ;
Error:
	if(outputContext)
	{
		for(int i = 0; i < outputContext->nb_streams; i++)
		{
			avcodec_close(outputContext->streams[i]->codec);
		}
		avformat_close_input(&outputContext);
	}
	return ret ;
}

void CloseInput()
{
	if(inputContext != nullptr)
	{
		avformat_close_input(&inputContext);
	}
}

void CloseOutput()
{
	if(outputContext != nullptr)
	{
		for(int i = 0 ; i < outputContext->nb_streams; i++)
		{
			AVCodecContext *codecContext = outputContext->streams[i]->codec;
			avcodec_close(codecContext);
		}
		avformat_close_input(&outputContext);
	}
}
void Init()
{
	av_register_all();
	avcodec_register_all();
	avfilter_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);
}

int _tmain(int argc, _TCHAR* argv[])
{
	Init();
	//int ret = OpenInput("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov");
	int ret = OpenInput("D:\\3_201804021820.dat");

	if(ret >= 0)
	{
	//int	ret = OpenOutput("D:\\3_20180403203.dat"); 
	}
	if(ret <0) goto Error;

	int nRestart = false;
	fprintf(stdout, "version %d\n", avcodec_version());
	int got_picture;
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();
	int videoindex = -1;
	int audioindex = -1;
	AVStream *pVst, *pAst;
	uint8_t* buffer_rgb = NULL;

	FILE *fpcm;
	for (int i = 0; i < inputContext->nb_streams; i++)
	{
		if ((inputContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) && (videoindex < 0))
		{
			videoindex = i;
		}
		if ((inputContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) && (audioindex < 0))
		{
			audioindex = i;
		}
	}

	if (!nRestart)
	{
		pVst = inputContext->streams[videoindex];
		pAst = inputContext->streams[audioindex];

		pVideoCodecCtx = pVst->codec;
		pAudioCodecCtx = pAst->codec;
		pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
		if (pVideoCodec == NULL)
			return -1;
		pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);

		if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
			return -1;

		/*	pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
		if (pAudioCodec == NULL)
		return -1;
		pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);

		if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0)
		return -1;

		nRestart = true;
		fpcm = fopen("D:\\audio.mp3", "wb");*/
	}
	while (true)
	{
		auto packet = ReadPacketFromSource();
		if (nullptr == packet) {
			continue;
		}
		if (packet->stream_index == videoindex)
		{
			fprintf(stdout, "pkt.size=%d,pkt.pts=%lld, pkt.data=0x%x.", packet->size, packet->pts, (unsigned int)packet->data);
			int av_result = avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, packet.get());

			if (got_picture)
			{
				fprintf(stdout, "decode one video frame!\n");
			}

			if (av_result < 0)
			{
				fprintf(stderr, "decode failed: inputbuf = 0x%x , input_framesize = %d\n", packet->data, packet->size);
				continue;
			}

			if (got_picture)
			{

				int bytes = avpicture_get_size(AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);
				buffer_rgb = (uint8_t *)av_malloc(bytes);
				avpicture_fill((AVPicture *)pFrameRGB, buffer_rgb, AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height);

				img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
					pVideoCodecCtx->width,	pVideoCodecCtx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
				if (img_convert_ctx == NULL)
				{

					printf("can't init convert context!\n");
					return -1;
				}
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				IplImage *pRgbImg = cvCreateImage(cvSize(pVideoCodecCtx->width, pVideoCodecCtx->height), 8, 3);
				memcpy(pRgbImg->imageData, buffer_rgb, pVideoCodecCtx->width * 3 * pVideoCodecCtx->height);
				cvShowImage("GB28181Show", pRgbImg);
				cvWaitKey(10);
				cvReleaseImage(&pRgbImg);
				sws_freeContext(img_convert_ctx);
				av_free(buffer_rgb);
 
			}
		}
		//else if (packet->stream_index == audioindex)
		//{
		//	int frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 3 / 2;
		//	if (avcodec_decode_audio4(pAudioCodecCtx, pFrame, &frame_size, packet.get()) >= 0)
		//	{
		//		fprintf(stdout, "===========================>>>decode one audio frame!\n");
		//	}
		//	if (frame_size)
		//	{
		//		uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO; //定义目标音频参数 
		//														   //nb_samples: AAC-1024 MP3-1152  
		//		int out_nb_samples = pAudioCodecCtx->frame_size;
		//		AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
		//		int out_sample_rate = 44100;
		//		int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

		//		//Out Buffer Size  
		//		int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);//计算转换后数据大小  
		//		uint8_t * audio_out_buffer = (uint8_t *)av_malloc(out_buffer_size);//申请输出缓冲区  
		//																		   //FIX:Some Codec's Context Information is missing  
		//		uint64_t in_channel_layout = av_get_default_channel_layout(pAudioCodecCtx->channels);
		//		//Swr  
		//		SwrContext *audio_convert_ctx = swr_alloc();
		//		audio_convert_ctx = swr_alloc_set_opts(audio_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		//			pAudioCodecCtx->channel_layout, pAudioCodecCtx->sample_fmt, pAudioCodecCtx->sample_rate, 0, NULL);//配置源音频参数和目标音频参数  
		//		swr_init(audio_convert_ctx);

		//		int len2 = swr_convert(audio_convert_ctx, &audio_out_buffer, out_buffer_size, (const uint8_t **)pFrame->data, pFrame->nb_samples);
		//		int resampled_data_size = len2 * out_channels  * av_get_bytes_per_sample(out_sample_fmt);//每声道采样数 x 声道数 x 每个采样字节数   
		//		//x 声道数 x 每个采样字节数
		//		fwrite(audio_out_buffer, 1, resampled_data_size, fpcm);//pcm记录  
		//		fflush(fpcm);
		//	}
		//}

		if(packet)
		{
			/*ret = WritePacket(packet);
			if(ret >= 0)
			{
				cout<<"WritePacket Success!"<<endl;
			}
			else
			{
				cout<<"WritePacket failed!"<<endl;
			}*/
		}
		else
		{
			break;
		}
	}
	

Error:
	CloseInput();
	CloseOutput();
	fclose(fpcm);
	while(true)
	{
		this_thread::sleep_for(chrono::seconds(100));
	}
	return 0;
}

