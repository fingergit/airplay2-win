#include "FgAirplayChannel.h"
#include "CAutoLock.h"

FgAirplayChannel::FgAirplayChannel(IAirServerCallback* pCallback)
: m_nRef(1)
, m_pCallback(pCallback)
, m_pCodec(NULL)
, m_pCodecCtx(NULL)
, m_pSwsCtx(NULL)
, m_bCodecOpened(false)
, m_fScaleRatio(1.0f)
{
	memset(&m_sVideoFrameOri, 0, sizeof(SFgVideoFrame));
	memset(&m_sVideoFrameScale, 0, sizeof(SFgVideoFrame));

	m_mutexAudio = CreateMutex(NULL, FALSE, NULL);
	m_mutexVideo = CreateMutex(NULL, FALSE, NULL);
}

FgAirplayChannel::~FgAirplayChannel()
{
	m_pCallback = NULL;
	if (m_sVideoFrameOri.data)
	{
		delete[] m_sVideoFrameOri.data;
		m_sVideoFrameOri.data = NULL;
	}
	if (m_sVideoFrameScale.data)
	{
		delete[] m_sVideoFrameScale.data;
		m_sVideoFrameScale.data = NULL;
	}

	unInitFFmpeg();

	CloseHandle(m_mutexAudio);
	CloseHandle(m_mutexVideo);
}

long FgAirplayChannel::addRef()
{
	long lRef = InterlockedIncrement(&m_nRef);
	return (m_nRef > 1 ? m_nRef : 1);
}

long FgAirplayChannel::release()
{
	LONG lRef = InterlockedDecrement(&m_nRef);
	if (0 == lRef)
	{
		delete this;
		return 0;
	}
	return (m_nRef > 1 ? m_nRef : 1);
}

int FgAirplayChannel::initFFmpeg(const void* privatedata, int privatedatalen) {
	if (m_pCodec == NULL) {
		m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
	}
	if (m_pCodec == NULL) {
		return -1;
	}

	m_pCodecCtx->extradata = (uint8_t*)av_malloc(privatedatalen);
	m_pCodecCtx->extradata_size = privatedatalen;
	memcpy(m_pCodecCtx->extradata, privatedata, privatedatalen);
	m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	int res = avcodec_open2(m_pCodecCtx, m_pCodec, NULL);
	if (res < 0)
	{
		printf("Failed to initialize decoder\n");
		return -1;
	}

	m_bCodecOpened = true;

	return 0;
}

void FgAirplayChannel::unInitFFmpeg()
{
	CAutoLock oLock(m_mutexVideo, "unInitFFmpeg");
	if (m_pCodecCtx)
	{
		if (m_pCodecCtx->extradata) {
			av_freep(&m_pCodecCtx->extradata);
		}
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
	if (m_pSwsCtx)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
}

float FgAirplayChannel::setScale(float fRatio)
{
	m_fScaleRatio = fRatio;
	return m_fScaleRatio;
}

int FgAirplayChannel::decodeH264Data(SFgH264Data* data, const char* remoteName, const char* remoteDeviceId) {
	int ret = 0;
	if (!m_bCodecOpened && !data->is_key) {
		return 0;
	}
	if (data->is_key) {
		ret = initFFmpeg(data->data, data->size);
		if (ret < 0) {
			return ret;
		}
	}
	if (!m_bCodecOpened) {
		return 0;
	}

	AVPacket pkt1, * packet = &pkt1;
	int frameFinished;
	AVFrame* pFrame;

	pFrame = av_frame_alloc();

	av_new_packet(packet, data->size);
	memcpy(packet->data, data->data, data->size);

	ret = avcodec_send_packet(this->m_pCodecCtx, packet);
	frameFinished = avcodec_receive_frame(this->m_pCodecCtx, pFrame);

	av_packet_unref(packet);

	// Did we get a video frame?
	if (frameFinished == 0)
	{
		if (m_sVideoFrameOri.width != pFrame->width ||
			m_sVideoFrameOri.height != pFrame->height) {
			if (m_sVideoFrameOri.data)
			{
				delete[] m_sVideoFrameOri.data;
				m_sVideoFrameOri.data = NULL;
			}
		}

		m_sVideoFrameOri.width = pFrame->width;
		m_sVideoFrameOri.height = pFrame->height;
		m_sVideoFrameOri.pts = pFrame->pts;
		m_sVideoFrameOri.isKey = pFrame->key_frame;
		int ySize = pFrame->linesize[0] * pFrame->height;
		int uSize = pFrame->linesize[1] * pFrame->height >> 1;
		int vSize = pFrame->linesize[2] * pFrame->height >> 1;
		m_sVideoFrameOri.dataTotalLen = ySize + uSize + vSize;
		m_sVideoFrameOri.dataLen[0] = ySize;
		m_sVideoFrameOri.dataLen[1] = uSize;
		m_sVideoFrameOri.dataLen[2] = vSize;
		if (!m_sVideoFrameOri.data)
		{
			m_sVideoFrameOri.data = new uint8_t[m_sVideoFrameOri.dataTotalLen];
		}
		memcpy(m_sVideoFrameOri.data, pFrame->data[0], ySize);
		memcpy(m_sVideoFrameOri.data + ySize, pFrame->data[1], uSize);
		memcpy(m_sVideoFrameOri.data + ySize + uSize, pFrame->data[2], vSize);
		m_sVideoFrameOri.pitch[0] = pFrame->linesize[0];
		m_sVideoFrameOri.pitch[1] = pFrame->linesize[1];
		m_sVideoFrameOri.pitch[2] = pFrame->linesize[2];

		if (m_pCallback != NULL)
		{
			if (m_fScaleRatio < 0.9999f || m_fScaleRatio > 1.0001f) {
				scaleH264Data(&m_sVideoFrameOri);
				m_pCallback->outputVideo(&m_sVideoFrameScale, remoteName, remoteDeviceId);
			}
			else {
				m_pCallback->outputVideo(&m_sVideoFrameOri, remoteName, remoteDeviceId);
			}
		}
	}
	av_frame_free(&pFrame);

	return 0;
}

int FgAirplayChannel::scaleH264Data(SFgVideoFrame* pSrcFrame)
{
	int nScreenWidth = pSrcFrame->width * m_fScaleRatio;
	int nScreenHeight = pSrcFrame->height * m_fScaleRatio;
	if (m_pSwsCtx && m_sVideoFrameScale.width != nScreenWidth || m_sVideoFrameScale.height != nScreenHeight)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
	if (!m_pSwsCtx)
	{
		m_pSwsCtx = sws_getContext(pSrcFrame->width, pSrcFrame->height, AV_PIX_FMT_YUV420P,
			nScreenWidth, nScreenHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);
	}
	if (m_sVideoFrameScale.width != nScreenWidth || m_sVideoFrameScale.height != nScreenHeight)
	{
		delete[] m_sVideoFrameScale.data;
		m_sVideoFrameScale.data = NULL;
	}

	m_sVideoFrameScale.width = nScreenWidth;
	m_sVideoFrameScale.height = nScreenHeight;
	m_sVideoFrameScale.pts = pSrcFrame->pts;
	m_sVideoFrameScale.isKey = pSrcFrame->isKey;
	m_sVideoFrameScale.pitch[0] = ((nScreenWidth + 15) >> 4) << 4;
	m_sVideoFrameScale.pitch[1] = (((nScreenWidth >> 1) + 15) >> 4) << 4;
	m_sVideoFrameScale.pitch[2] = m_sVideoFrameScale.pitch[1];

	int ySize = m_sVideoFrameScale.pitch[0] * nScreenHeight;
	int uSize = m_sVideoFrameScale.pitch[1] * nScreenHeight >> 1;
	int vSize = m_sVideoFrameScale.pitch[2] * nScreenHeight >> 1;
	m_sVideoFrameScale.dataTotalLen = ySize + uSize + vSize;
	m_sVideoFrameScale.dataLen[0] = ySize;
	m_sVideoFrameScale.dataLen[1] = uSize;
	m_sVideoFrameScale.dataLen[2] = vSize;
	if (!m_sVideoFrameScale.data) {
		m_sVideoFrameScale.data = new uint8_t[m_sVideoFrameScale.dataTotalLen];
	}

	const uint8_t* srcSlice[3] = { pSrcFrame->data, pSrcFrame->data + pSrcFrame->dataLen[0], pSrcFrame->data + pSrcFrame->dataLen[0] + pSrcFrame->dataLen[1] };
	const uint8_t* dst[3] = { m_sVideoFrameScale.data,
		m_sVideoFrameScale.data + m_sVideoFrameScale.dataLen[0],
		m_sVideoFrameScale.data + m_sVideoFrameScale.dataLen[0] + m_sVideoFrameScale.dataLen[1] };
	sws_scale(m_pSwsCtx, (const uint8_t* const*)srcSlice, (const int*)pSrcFrame->pitch, 0,
		pSrcFrame->height, (uint8_t* const*)dst, (const int*)m_sVideoFrameScale.pitch);

	return 0;
}
