/***************************************************************************
 *   Copyright (C) 2012-2017 by Ilya Kotov                                 *
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

#include <string.h>
#include "statehandler.h"
#include "visual.h"
#include "output.h"
#include "audioconverter.h"
#include "channelconverter_p.h"
#include "volumecontrol_p.h"
#include "outputwriter_p.h"

extern "C" {
#include "equ/iir.h"
}

OutputWriter::OutputWriter (QObject* parent) : QThread (parent)
{
    m_handler = StateHandler::instance();
    m_frequency = 0;
    m_channels = 0;
    m_output = 0;
    m_format = Qmmp::PCM_UNKNOWM;
    m_totalWritten = 0;
    m_currentMilliseconds = -1;
    m_bytesPerMillisecond = 0;
    m_user_stop = false;
    m_finish = false;
    m_kbps = 0;
    m_skip = false;
    m_pause = false;
    m_prev_pause = false;
    m_useEq = false;
    m_muted = false;
    m_settings = QmmpSettings::instance();
    m_format_converter = 0;
    m_channel_converter = 0;
    m_output_buf = 0;
    m_output_size = 0;
}

OutputWriter::~OutputWriter()
{
    if(m_output)
        delete m_output;
    if(m_format_converter)
        delete m_format_converter;
    if(m_channel_converter)
        delete m_channel_converter;
    if(m_output_buf)
        delete[] m_output_buf;
}

bool OutputWriter::initialize(quint32 freq, ChannelMap map)
{
    m_in_params = AudioParameters(freq, map, Qmmp::PCM_FLOAT);
    m_output = Output::create();
    if(!m_output)
    {
        qWarning("OutputWriter: unable to create output");
        return false;
    }

    if (!m_output->initialize(freq, map, m_settings->outputFormat()))
    {
        qWarning("OutputWriter: unable to initialize output");
        delete m_output;
        m_output = 0;
        return false;
    }
    m_frequency = m_output->sampleRate();
    m_chan_map = m_output->channelMap();
    m_channels = m_chan_map.count();
    m_format = m_output->format();

    qDebug("OutputWriter: [%s] %s ==> %s",
           qPrintable(Output::currentFactory()->properties().shortName),
           qPrintable(m_in_params.toString()), qPrintable(m_output->audioParameters().toString()));

    if(!prepareConverters())
    {
        qWarning("OutputWriter: unable to convert audio");
        delete m_output;
        m_output = 0;
        return false;
    }

    if(m_output_buf)
        delete[] m_output_buf;
    m_output_size = QMMP_BLOCK_FRAMES * m_channels * 4;
    m_output_buf = new unsigned char[m_output_size * m_output->sampleSize()];

    m_bytesPerMillisecond = m_frequency * m_channels * AudioParameters::sampleSize(m_format) / 1000;
    m_recycler.configure(m_in_params.sampleRate(), m_in_params.channels()); //calculate output buffer size
    updateEqSettings();
    clean_history();
    return true;
}

void OutputWriter::pause()
{
    mutex()->lock();
    m_pause = !m_pause;
    mutex()->unlock();
    Qmmp::State state = m_pause ? Qmmp::Paused: Qmmp::Playing;
    dispatch(state);
}

void OutputWriter::stop()
{
    m_user_stop = true;
}

void OutputWriter::setMuted(bool muted)
{
    m_muted = muted;
}

void OutputWriter::finish()
{
    m_finish = true;
}

void OutputWriter::seek(qint64 pos, bool reset)
{
    m_totalWritten = pos * m_bytesPerMillisecond;
    m_currentMilliseconds = -1;
    m_skip = isRunning() && reset;
}

Recycler *OutputWriter::recycler()
{
    return &m_recycler;
}

QMutex *OutputWriter::mutex()
{
    return &m_mutex;
}

AudioParameters OutputWriter::audioParameters() const
{
    return AudioParameters(m_frequency, m_chan_map, Qmmp::PCM_FLOAT);
}

quint32 OutputWriter::sampleRate()
{
    return m_frequency;
}

int OutputWriter::channels()
{
    return m_channels;
}

Qmmp::AudioFormat OutputWriter::format() const
{
    return m_format;
}

const ChannelMap OutputWriter::channelMap() const
{
    return m_chan_map;
}

int OutputWriter::sampleSize() const
{
    return AudioParameters::sampleSize(m_format);
}

void OutputWriter::dispatchVisual (Buffer *buffer)
{
    if(!buffer)
        return;

    Visual::addAudio(buffer->data, buffer->samples, m_channels,
                     m_totalWritten / m_bytesPerMillisecond, m_output->latency());
}

bool OutputWriter::prepareConverters()
{
    if(m_format_converter)
    {
        delete m_format_converter;
        m_format_converter = 0;
    }
    if(m_channel_converter)
    {
        delete m_channel_converter;
        m_channel_converter = 0;
    }

    if(m_channels != m_output->channels())
    {
        qWarning("OutputWriter: unsupported channel number");
        return false;
    }

    if(m_in_params.format() != m_format)
    {
        m_format_converter = new AudioConverter();
        m_format_converter->configure(m_format);
    }

    if(m_in_params.channelMap() != m_chan_map)
    {
        m_channel_converter = new ChannelConverter(m_chan_map);
        m_channel_converter->configure(m_in_params.sampleRate(), m_in_params.channelMap());
    }
    return true;
}

void OutputWriter::startVisualization()
{
    foreach (Visual *visual, *Visual::visuals())
    {
        QMetaObject::invokeMethod(visual, "start", Qt::QueuedConnection);
    }
}

void OutputWriter::stopVisualization()
{
    Visual::clearBuffer();
    foreach (Visual *visual, *Visual::visuals())
    {
        QMetaObject::invokeMethod(visual, "stop", Qt::QueuedConnection);
    }
}

void OutputWriter::dispatch(qint64 elapsed, int bitrate)
{
    if (m_handler)
        m_handler->dispatch(elapsed, bitrate);
}

void OutputWriter::dispatch(const Qmmp::State &state)
{
    if (m_handler)
        m_handler->dispatch(state);
}

void OutputWriter::dispatch(const AudioParameters &p)
{
    if (m_handler)
        m_handler->dispatch(p);
}

void OutputWriter::run()
{
    mutex()->lock ();
    if (!m_bytesPerMillisecond)
    {
        qWarning("OutputWriter: invalid audio parameters");
        mutex()->unlock ();
        return;
    }
    mutex()->unlock ();

    bool done = false;
    Buffer *b = 0;
    quint64 l;
    qint64 m = 0;
    size_t output_at = 0;
    unsigned char *tmp = 0;

    dispatch(Qmmp::Playing);
    dispatch(m_output->audioParameters());
    startVisualization();

    while (!done)
    {
        mutex()->lock ();
        if(m_pause != m_prev_pause)
        {
            if(m_pause)
            {
                Visual::clearBuffer();
                m_output->suspend();
                mutex()->unlock();
                m_prev_pause = m_pause;
                continue;
            }
            else
                m_output->resume();
            m_prev_pause = m_pause;
        }
        recycler()->mutex()->lock ();
        done = m_user_stop || (m_finish && recycler()->empty());

        while (!done && (recycler()->empty() || m_pause))
        {
            recycler()->cond()->wakeOne();
            mutex()->unlock();
            recycler()->cond()->wait(recycler()->mutex());
            mutex()->lock ();
            done = m_user_stop || m_finish;
        }

        status();
        if (!b)
        {
            if((b = recycler()->next()))
            {
                if(b->rate)
                    m_kbps = b->rate;
                if(b->metaData)
                    m_output->setMetaData(*b->metaData);
            }
        }

        recycler()->cond()->wakeOne();
        recycler()->mutex()->unlock();
        mutex()->unlock();
        if (b)
        {
            mutex()->lock();
            if (m_useEq)
            {
                iir(b->data, b->samples, m_channels);
            }
            mutex()->unlock();
            dispatchVisual(b);
            if (SoftwareVolume::instance())
                SoftwareVolume::instance()->changeVolume(b, m_channels);
            if (m_muted)
                memset(b->data, 0, b->size * sizeof(float));
            if(m_channel_converter)
                m_channel_converter->applyEffect(b);
            l = 0;
            m = 0;

            //increase buffer size if needed
            if(b->samples > m_output_size)
            {
                delete [] m_output_buf;
                m_output_size = b->samples;
                m_output_buf = new unsigned char[m_output_size * sampleSize()];
            }

            if(m_format_converter)
            {
                m_format_converter->fromFloat(b->data, m_output_buf, b->samples);
                tmp = m_output_buf;
            }
            else
            {
                tmp = (unsigned char*)b->data;
            }
            output_at = b->samples * m_output->sampleSize();

            while (l < output_at && !m_pause && !m_prev_pause)
            {
                mutex()->lock();
                if(m_skip)
                {
                    m_skip = false;
                    Visual::clearBuffer();
                    m_output->reset();
                    mutex()->unlock();
                    break;
                }
                mutex()->unlock();
                m = m_output->writeAudio(tmp + l, output_at - l);
                if(m >= 0)
                {
                    m_totalWritten += m;
                    l+= m;
                }
                else
                    break;
            }
            if(m < 0)
                break;
        }
        mutex()->lock();
        //force buffer change
        recycler()->mutex()->lock ();
        recycler()->done();
        recycler()->mutex()->unlock();
        b = 0;
        mutex()->unlock();
    }
    mutex()->lock ();
    //write remaining data
    if(m_finish)
    {
        m_output->drain();
#ifdef Q_OS_WIN
        qDebug("OutputWriter: total written %I64d", m_totalWritten);
#else
        qDebug("OutputWriter: total written %lld", m_totalWritten);
#endif
    }
    dispatch(Qmmp::Stopped);
    stopVisualization();
    mutex()->unlock();
}

void OutputWriter::status()
{
    qint64 ct = m_totalWritten / m_bytesPerMillisecond - m_output->latency();

    if (ct < 0)
        ct = 0;

    if (ct > m_currentMilliseconds)
    {
        m_currentMilliseconds = ct;
        dispatch(m_currentMilliseconds, m_kbps);
    }
}

void OutputWriter::updateEqSettings()
{
    mutex()->lock();
    if(m_settings->eqSettings().isEnabled())
    {
        double preamp = m_settings->eqSettings().preamp();
        int bands =  m_settings->eqSettings().bands();

        init_iir(m_frequency, bands);

        set_preamp(0, 1.0 + 0.0932471 *preamp + 0.00279033 * preamp * preamp);
        set_preamp(1, 1.0 + 0.0932471 *preamp + 0.00279033 * preamp * preamp);
        for(int i = 0; i < bands; ++i)
        {
            double value =  m_settings->eqSettings().gain(i);
            set_gain(i,0, 0.03*value+0.000999999*value*value);
            set_gain(i,1, 0.03*value+0.000999999*value*value);
        }
    }
    m_useEq = m_settings->eqSettings().isEnabled();
    mutex()->unlock();
}
