/***************************************************************************
 *   Copyright (C) 2008-2018 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QMessageBox>
#include <QTranslator>
#include <QtPlugin>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}

#include "ffmpegmetadatamodel.h"
#include "settingsdialog.h"
#include "decoder_ffmpeg.h"
#include "decoderffmpegfactory.h"


// DecoderFFmpegFactory

DecoderFFmpegFactory::DecoderFFmpegFactory()
{
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)) //ffmpeg-3.5
    avcodec_register_all();
    avformat_network_init();
    av_register_all();
#endif
}

bool DecoderFFmpegFactory::canDecode(QIODevice *i) const
{
    QStringList filters = properties().filters;

    AVProbeData pd;
    memset(&pd, 0, sizeof(pd));
    uint8_t buf[PROBE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];
    pd.buf_size = i->peek((char*)buf, sizeof(buf) - AVPROBE_PADDING_SIZE);
    pd.buf = buf;
    if(pd.buf_size < PROBE_BUFFER_SIZE)
        return false;
    AVInputFormat *fmt = av_probe_input_format(&pd, 1);
    if(!fmt)
        return false;

    QStringList formats = QString::fromLatin1(fmt->name).split(",");

    if(filters.contains("*.wma") && formats.contains("asf"))
        return true;
    else if(filters.contains("*.mp3") && formats.contains("mp3"))
        return true;
    else if(filters.contains("*.aac") && formats.contains("aac"))
        return true;
    else if(filters.contains("*.ac3") && formats.contains("eac3"))
        return true;
    else if(filters.contains("*.dts") && formats.contains("dts"))
        return true;
    else if(filters.contains("*.mka") && formats.contains("mka"))
        return true;
    else if(filters.contains("*.vqf") && formats.contains("vqf"))
        return true;
    else if(filters.contains("*.ape") && formats.contains("ape"))
        return true;
    else if(filters.contains("*.tta") && formats.contains("tta"))
        return true;
    else if(filters.contains("*.m4a") && (formats.contains("m4a") || formats.contains("mp4")))
        return true;
    return false;
}

const DecoderProperties DecoderFFmpegFactory::properties() const
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    QStringList filters;
    filters << "*.wma" << "*.ape" << "*.tta" << "*.m4a" << "*.aac" << "*.ra" << "*.shn" << "*.vqf" << "*.ac3";
    filters = settings.value("FFMPEG/filters", filters).toStringList();

    //remove unsupported filters
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54,51,100)) //libav 10
    if(!avcodec_find_decoder(AV_CODEC_ID_WMAV1))
        filters.removeAll("*.wma");
    if(!avcodec_find_decoder(AV_CODEC_ID_APE))
        filters.removeAll("*.ape");
    if(!avcodec_find_decoder(AV_CODEC_ID_TTA))
        filters.removeAll("*.tta");
    if(!avcodec_find_decoder(AV_CODEC_ID_AAC))
        filters.removeAll("*.aac");
    if(!avcodec_find_decoder(AV_CODEC_ID_MP3))
        filters.removeAll("*.mp3");
    if(!avcodec_find_decoder(AV_CODEC_ID_AAC) && !avcodec_find_decoder(AV_CODEC_ID_ALAC))
        filters.removeAll("*.m4a");
    if(!avcodec_find_decoder(AV_CODEC_ID_RA_288))
        filters.removeAll("*.ra");
    if(!avcodec_find_decoder(AV_CODEC_ID_SHORTEN))
        filters.removeAll("*.shn");
    if(!avcodec_find_decoder(AV_CODEC_ID_EAC3))
        filters.removeAll("*.ac3");
    if(!avcodec_find_decoder(AV_CODEC_ID_DTS))
        filters.removeAll("*.dts");
    if(!avcodec_find_decoder(AV_CODEC_ID_TRUEHD))
        filters.removeAll("*.mka");
    if(!avcodec_find_decoder(AV_CODEC_ID_TWINVQ))
        filters.removeAll("*.vqf");
#else
    if(!avcodec_find_decoder(CODEC_ID_WMAV1))
        filters.removeAll("*.wma");
    if(!avcodec_find_decoder(CODEC_ID_APE))
        filters.removeAll("*.ape");
    if(!avcodec_find_decoder(CODEC_ID_TTA))
        filters.removeAll("*.tta");
    if(!avcodec_find_decoder(CODEC_ID_AAC))
        filters.removeAll("*.aac");
    if(!avcodec_find_decoder(CODEC_ID_MP3))
        filters.removeAll("*.mp3");
    if(!avcodec_find_decoder(CODEC_ID_AAC) && !avcodec_find_decoder(CODEC_ID_ALAC))
        filters.removeAll("*.m4a");
    if(!avcodec_find_decoder(CODEC_ID_RA_288))
        filters.removeAll("*.ra");
    if(!avcodec_find_decoder(CODEC_ID_SHORTEN))
        filters.removeAll("*.shn");
    if(!avcodec_find_decoder(CODEC_ID_EAC3))
        filters.removeAll("*.ac3");
    if(!avcodec_find_decoder(CODEC_ID_DTS))
        filters.removeAll("*.dts");
    if(!avcodec_find_decoder(CODEC_ID_TRUEHD))
        filters.removeAll("*.mka");
    if(!avcodec_find_decoder(CODEC_ID_TWINVQ))
        filters.removeAll("*.vqf");
#endif


    DecoderProperties properties;
    properties.name = tr("FFmpeg Plugin");
    properties.filters = filters;
    properties.description = tr("FFmpeg Formats");
    if(filters.contains("*.wma"))
        properties.contentTypes << "audio/x-ms-wma";
    if(filters.contains("*.mp3"))
        properties.contentTypes << "audio/mpeg";
    if(filters.contains("*.aac"))
        properties.contentTypes << "audio/aac" << "audio/aacp";
    if(filters.contains("*.shn"))
        properties.contentTypes << "audio/x-ffmpeg-shorten";
    if(filters.contains("*.m4a"))
    {
        properties.contentTypes << "audio/3gpp" << "audio/3gpp2" << "audio/mp4";
        properties.contentTypes << "audio/MP4A-LATM" << "audio/mpeg4-generic";
        properties.contentTypes << "audio/m4a";
    }
    if(filters.contains("*.ac3"))
        properties.contentTypes << "audio/ac3" << "audio/eac3";
    if(filters.contains("*.dts"))
        properties.contentTypes << "audio/dts";
    if(filters.contains("*.mka"))
        properties.contentTypes << "audio/true-hd" << "audio/x-matroska";
    properties.shortName = "ffmpeg";
    properties.hasAbout = true;
    properties.hasSettings = true;
    properties.noInput = false;
    properties.priority = 10;
    return properties;
}

Decoder *DecoderFFmpegFactory::create(const QString &path, QIODevice *input)
{
    return new DecoderFFmpeg(path, input);
}

QList<FileInfo *> DecoderFFmpegFactory::createPlayList(const QString &fileName, bool useMetaData, QStringList *)
{
    QList <FileInfo*> list;
    AVFormatContext *in = 0;

#ifdef Q_OS_WIN
    if (avformat_open_input(&in,fileName.toUtf8().constData(), 0, 0) < 0)
#else
    if (avformat_open_input(&in,fileName.toLocal8Bit().constData(), 0, 0) < 0)
#endif
    {
        qDebug("DecoderFFmpegFactory: unable to open file");
        return list;
    }
    FileInfo *info = new FileInfo(fileName);
    avformat_find_stream_info(in, 0);

    if (useMetaData)
    {
        AVDictionaryEntry *album = av_dict_get(in->metadata,"album",0,0);
        if(!album)
            album = av_dict_get(in->metadata,"WM/AlbumTitle",0,0);
        AVDictionaryEntry *artist = av_dict_get(in->metadata,"artist",0,0);
        if(!artist)
            artist = av_dict_get(in->metadata,"author",0,0);
        AVDictionaryEntry *comment = av_dict_get(in->metadata,"comment",0,0);
        AVDictionaryEntry *genre = av_dict_get(in->metadata,"genre",0,0);
        AVDictionaryEntry *title = av_dict_get(in->metadata,"title",0,0);
        AVDictionaryEntry *year = av_dict_get(in->metadata,"WM/Year",0,0);
        if(!year)
            year = av_dict_get(in->metadata,"year",0,0);
        if(!year)
            year = av_dict_get(in->metadata,"date",0,0);
        AVDictionaryEntry *track = av_dict_get(in->metadata,"track",0,0);
        if(!track)
            track = av_dict_get(in->metadata,"WM/Track",0,0);
        if(!track)
            track = av_dict_get(in->metadata,"WM/TrackNumber",0,0);

        if(album)
            info->setMetaData(Qmmp::ALBUM, QString::fromUtf8(album->value).trimmed());
        if(artist)
            info->setMetaData(Qmmp::ARTIST, QString::fromUtf8(artist->value).trimmed());
        if(comment)
            info->setMetaData(Qmmp::COMMENT, QString::fromUtf8(comment->value).trimmed());
        if(genre)
            info->setMetaData(Qmmp::GENRE, QString::fromUtf8(genre->value).trimmed());
        if(title)
            info->setMetaData(Qmmp::TITLE, QString::fromUtf8(title->value).trimmed());
        if(year)
            info->setMetaData(Qmmp::YEAR, year->value);
        if(track)
            info->setMetaData(Qmmp::TRACK, track->value);
    }
    info->setLength(in->duration/AV_TIME_BASE);
    avformat_close_input(&in);
    list << info;
    return list;
}

MetaDataModel* DecoderFFmpegFactory::createMetaDataModel(const QString &path, QObject *parent)
{
    return new FFmpegMetaDataModel(path,parent);
}

void DecoderFFmpegFactory::showSettings(QWidget *parent)
{
    SettingsDialog *s = new SettingsDialog(parent);
    s->show();
}

void DecoderFFmpegFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About FFmpeg Audio Plugin"),
                        tr("Qmmp FFmpeg Audio Plugin")+"\n"+
                        tr("Compiled against:") + "\n"+
                        QString("libavformat-%1.%2.%3\n"
                                "libavcodec-%4.%5.%6\n"
                                "libavutil-%7.%8.%9")
                        .arg(LIBAVFORMAT_VERSION_MAJOR)
                        .arg(LIBAVFORMAT_VERSION_MINOR)
                        .arg(LIBAVFORMAT_VERSION_MICRO)
                        .arg(LIBAVCODEC_VERSION_MAJOR)
                        .arg(LIBAVCODEC_VERSION_MINOR)
                        .arg(LIBAVCODEC_VERSION_MICRO)
                        .arg(LIBAVUTIL_VERSION_MAJOR)
                        .arg(LIBAVUTIL_VERSION_MINOR)
                        .arg(LIBAVUTIL_VERSION_MICRO) +"\n"+
                        tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QTranslator *DecoderFFmpegFactory::createTranslator(QObject *parent)
{
    QTranslator *translator = new QTranslator(parent);
    QString locale = Qmmp::systemLanguageID();
    translator->load(QString(":/ffmpeg_plugin_") + locale);
    return translator;
}

Q_EXPORT_PLUGIN2(ffmpeg,DecoderFFmpegFactory)
