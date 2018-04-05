#ifndef _RTSP_SHOW_H__
#define _RTSP_SHOW_H__
extern "C"
{
	#include "libavutil/opt.h"
	#include "libavutil/channel_layout.h"
	#include "libavutil/common.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/time.h"
	#include "libavutil/fifo.h"
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavformat/avio.h"
	#include "libavfilter/avfiltergraph.h"
	#include "libavfilter/avfilter.h"
	#include "libavfilter/buffersink.h"
	#include "libavfilter/buffersrc.h"
	#include "libswscale/swscale.h"
	#include "libswresample/swresample.h"
}

/*opencv¿â*/
#include <opencv2\opencv.hpp>

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#endif