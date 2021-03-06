/***************************************************************************
 *   Copyright (C) 2009-2018 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <stdint.h>
#include "ffmpegmetadatamodel.h"

FFmpegMetaDataModel::FFmpegMetaDataModel(const QString &path, QObject *parent) : MetaDataModel(parent)
{
    m_in = 0;
#ifdef Q_OS_WIN
    if (avformat_open_input(&m_in, path.toUtf8().constData(), 0, 0) < 0)
#else
    if (avformat_open_input(&m_in, path.toLocal8Bit().constData(), 0, 0) < 0)
#endif
        return;
    avformat_find_stream_info(m_in, 0);
    av_read_play(m_in);
}

FFmpegMetaDataModel::~FFmpegMetaDataModel()
{
    if(m_in)
        avformat_close_input(&m_in);
}

QHash<QString, QString> FFmpegMetaDataModel::audioProperties()
{
    QHash<QString, QString> ap;
    if(!m_in)
        return ap;
    QString text = QString("%1").arg(int(m_in->duration/AV_TIME_BASE)/60);
    text +=":"+QString("%1").arg(int(m_in->duration/AV_TIME_BASE)%60,2,10,QChar('0'));
    ap.insert(tr("Length"), text);
    ap.insert(tr("File size"), tr("%1 KB").arg(avio_size(m_in->pb) / 1000));
    ap.insert(tr("Bitrate"), tr("%1 kbps").arg(m_in->bit_rate/1000));

    int idx = av_find_best_stream(m_in, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);

    if(idx >= 0)
    {
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
        AVCodecParameters *c = m_in->streams[idx]->codecpar;
#else
        AVCodecContext *c = m_in->streams[idx]->codec;
#endif
        ap.insert(tr("Sample rate"), tr("%1 Hz").arg(c->sample_rate));
        ap.insert(tr("Channels"), QString("%1").arg(c->channels));
    }
    return ap;
}

QPixmap FFmpegMetaDataModel::cover()
{
    if(!m_in)
        return QPixmap();
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
    AVCodecParameters *c = 0;
#else
    AVCodecContext *c = 0;
#endif

    for (uint idx = 0; idx < m_in->nb_streams; idx++)
    {
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
        c = m_in->streams[idx]->codecpar;
#else
        c = m_in->streams[idx]->codec;
#endif

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav 10
        if (c->codec_type == AVMEDIA_TYPE_VIDEO && c->codec_id == AV_CODEC_ID_MJPEG)
#else
        if (c->codec_type == AVMEDIA_TYPE_VIDEO && c->codec_id == CODEC_ID_MJPEG)
#endif
            break;
    }
    if (c)
    {
        AVPacket pkt;
        av_read_frame(m_in, &pkt);
        QPixmap pix;
        pix.loadFromData(QByteArray((const char*)pkt.data, pkt.size));
        return pix;
    }
    return QPixmap();
}
