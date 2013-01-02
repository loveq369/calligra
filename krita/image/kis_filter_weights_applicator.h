/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KIS_FILTER_WEIGHTS_APPLICATOR_H
#define __KIS_FILTER_WEIGHTS_APPLICATOR_H

#include "kis_fixed_point_maths.h"
#include "kis_filter_weights_buffer.h"

#include "kis_iterator_ng.h"
#include "kis_random_accessor_ng.h"
#include "kis_paint_device.h"

#include <KoColorSpace.h>


namespace tmp {
    template <class iter> iter createIterator(KisPaintDeviceSP dev, qint32 start, qint32 lineNum, qint32 len);

    template <> KisHLineIteratorSP createIterator <KisHLineIteratorSP>
    (KisPaintDeviceSP dev, qint32 start, qint32 lineNum, qint32 len)
    {
        return dev->createHLineIteratorNG(start, lineNum, len);
    }

    template <> KisVLineIteratorSP createIterator <KisVLineIteratorSP>
    (KisPaintDeviceSP dev, qint32 start, qint32 lineNum, qint32 len)
    {
        return dev->createVLineIteratorNG(lineNum, start, len);
    }
}

class KisFilterWeightsApplicator
{
public:
    KisFilterWeightsApplicator(KisPaintDeviceSP src,
                               KisPaintDeviceSP dst,
                               qreal realScale, qreal shear,
                               qreal dx, int srcStart,
                               bool clampToEdge)
        : m_src(src),
          m_dst(dst),
          m_realScale(realScale),
          m_shear(shear),
          m_dx(dx),
          m_srcStart(srcStart),
          m_clampToEdge(clampToEdge)
    {
    }

    /**
     * Notation:
     * <pixel_name>_l -- leftmost border of the pixel
     * <pixel_name>_c -- center of the pixel
     */

    struct BlendSpan {
        KisFilterWeightsBuffer::FilterWeights *weights;
        int firstBlendPixel; // in src coords
        KisFixedPoint offset;
        KisFixedPoint offsetInc;
    };

    inline BlendSpan calculateBlendSpan(int dst_l, int line, KisFilterWeightsBuffer *buffer) const {
        KisFixedPoint dst_c = l_to_c(dst_l);
        KisFixedPoint dst_c_in_src = dstToSrc(dst_c.toFloat(), line);

        KisFixedPoint next_c_in_src = (dst_c_in_src - 0.5).toIntCeil() + 0.5;

        BlendSpan span;
        span.offset = (next_c_in_src - dst_c_in_src) * buffer->weightsPositionScale();
        span.offsetInc = buffer->weightsPositionScale();

        Q_ASSERT(span.offset <= span.offsetInc);

        span.weights = buffer->weights(span.offset);
        span.firstBlendPixel = next_c_in_src.toIntFloor() - span.weights->centerIndex;

        return span;
    }

    template <class T>
    void processLine(int srcStart, int srcLen, int line, KisFilterWeightsBuffer *buffer, qreal filterSupport) {
        int dstStart;
        int dstEnd;

        int leftSrcBorder;
        int rightSrcBorder;

        if (m_realScale >= 0) {
            dstStart = findAntialiasedDstStart(srcStart, filterSupport, line);
            dstEnd = findAntialiasedDstEnd(srcStart + srcLen, filterSupport, line);

            leftSrcBorder = getLeftSrcNeedBorder(dstStart, line, buffer);
            rightSrcBorder = getRightSrcNeedBorder(dstEnd - 1, line, buffer);
        } else {
            dstStart = findAntialiasedDstStart(srcStart + srcLen, filterSupport, line);
            dstEnd = findAntialiasedDstEnd(srcStart, filterSupport, line);

            leftSrcBorder = getLeftSrcNeedBorder(dstEnd - 1, line, buffer);
            rightSrcBorder = getRightSrcNeedBorder(dstStart, line, buffer);
        }

        Q_ASSERT(dstStart < dstEnd);
        Q_ASSERT(leftSrcBorder < rightSrcBorder);
        Q_ASSERT(leftSrcBorder < srcStart);
        Q_ASSERT(srcStart + srcLen < rightSrcBorder);

        int pixelSize = m_src->pixelSize();
        KoMixColorsOp *mixOp = m_src->colorSpace()->mixColorsOp();
        const quint8 *defaultPixel = m_src->defaultPixel();
        const quint8 *borderPixel = defaultPixel;
        quint8 *srcLineBuf = new quint8[pixelSize * (rightSrcBorder - leftSrcBorder)];

        int i = leftSrcBorder;
        quint8 *bufPtr = srcLineBuf;

        T srcIt = tmp::createIterator<T>(m_src, srcStart, line, srcLen);

        if (m_clampToEdge) {
            borderPixel = srcIt->rawData();
        }

        for (; i < srcStart; i++, bufPtr+=pixelSize) {
            memcpy(bufPtr, borderPixel, pixelSize);
        }

        for (; i < srcStart + srcLen; i++, bufPtr+=pixelSize) {
            quint8 *data = srcIt->rawData();
            memcpy(bufPtr, data, pixelSize);
            memcpy(data, defaultPixel, pixelSize);
            srcIt->nextPixel();
        }

        if (m_clampToEdge) {
            borderPixel = bufPtr - pixelSize;
        }

        for (; i < rightSrcBorder; i++, bufPtr+=pixelSize) {
            memcpy(bufPtr, borderPixel, pixelSize);
        }

        const quint8 **colors = new const quint8* [buffer->maxSpan()];

        T dstIt = tmp::createIterator<T>(m_dst, dstStart, line, dstEnd - dstStart);
        for (int i = dstStart; i < dstEnd; i++) {
            BlendSpan span = calculateBlendSpan(i, line, buffer);

            int bufIndexStart = span.firstBlendPixel - leftSrcBorder;
            int bufIndexEnd = bufIndexStart + span.weights->span;

            const quint8 **colorsPtr = colors;
            for (int j = bufIndexStart; j < bufIndexEnd; j++) {
                *(colorsPtr++) = srcLineBuf + j * pixelSize;
            }

            mixOp->mixColors(colors, span.weights->weight, span.weights->span, dstIt->rawData());
            dstIt->nextPixel();
        }

        delete[] colors;
        delete[] srcLineBuf;
    }

private:

    int findAntialiasedDstStart(int src_l, qreal support, int line) {
        qreal dst = srcToDst(src_l, line);
        return !m_clampToEdge ? floor(dst - support) : floor(dst);
    }

    int findAntialiasedDstEnd(int src_l, qreal support, int line) {
        qreal dst = srcToDst(src_l, line);
        return !m_clampToEdge ? ceil(dst + support) : ceil(dst);
    }

    int getLeftSrcNeedBorder(int dst_l, int line, KisFilterWeightsBuffer *buffer) {
        BlendSpan span = calculateBlendSpan(dst_l, line, buffer);
        return span.firstBlendPixel;
    }

    int getRightSrcNeedBorder(int dst_l, int line, KisFilterWeightsBuffer *buffer) {
        BlendSpan span = calculateBlendSpan(dst_l, line, buffer);
        return span.firstBlendPixel + span.weights->span;
    }

    inline KisFixedPoint l_to_c(KisFixedPoint pixel_l) const {
        return pixel_l + 0.5;
    }

    inline KisFixedPoint c_to_l(KisFixedPoint pixel_c) const {
        return pixel_c - 0.5;
    }

    inline qreal srcToDst(qreal src, int line) const {
        return src * m_realScale + m_dx + line * m_shear;
    }

    inline qreal dstToSrc(qreal dst, int line) const {
        return (dst - m_dx - line * m_shear) / m_realScale;
    }

private:
    KisPaintDeviceSP m_src;
    KisPaintDeviceSP m_dst;

    qreal m_realScale;
    qreal m_shear;
    qreal m_dx;
    int m_srcStart;
    bool m_clampToEdge;
};

#endif /* __KIS_FILTER_WEIGHTS_APPLICATOR_H */
