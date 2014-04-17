/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#include "kis_painting_assistants_decoration.h"

#include <cfloat>

#include <QList>
#include <QPointF>
#include <klocale.h>

#include "kis_painting_assistant.h"
#include "kis_image_view.h"
#include <QPainter>

struct KisPaintingAssistantsDecoration::Private {
    QList<KisPaintingAssistant*> assistants;
};

KisPaintingAssistantsDecoration::KisPaintingAssistantsDecoration(KisImageView* parent) :
        KisCanvasDecoration("paintingAssistantsDecoration", i18n("Painting assistants"), parent),
        d(new Private)
{
}

KisPaintingAssistantsDecoration::~KisPaintingAssistantsDecoration()
{
    qDeleteAll(d->assistants.begin(), d->assistants.end());
    delete d;
}

void KisPaintingAssistantsDecoration::addAssistant(KisPaintingAssistant* assistant)
{
    if (d->assistants.contains(assistant)) return;
    d->assistants.push_back(assistant);
    emit assistantChanged();
}

void KisPaintingAssistantsDecoration::removeAssistant(KisPaintingAssistant* assistant)
{
    delete assistant;
    d->assistants.removeAll(assistant);
    emit assistantChanged();
}

void KisPaintingAssistantsDecoration::removeAll()
{
    foreach (KisPaintingAssistant* assistant, d->assistants) {
        delete assistant;
    }
    d->assistants.clear();
    emit assistantChanged();
}

QPointF KisPaintingAssistantsDecoration::adjustPosition(const QPointF& point, const QPointF& strokeBegin)
{
    if (d->assistants.empty()) return point;
    if (d->assistants.count() == 1) {
        QPointF newpoint = d->assistants.first()->adjustPosition(point, strokeBegin);
        // check for NaN
        if (newpoint.x() != newpoint.x()) return point;
        return newpoint;
    }
    QPointF best = point;
    double distance = DBL_MAX;
    foreach(KisPaintingAssistant* assistant, d->assistants) {
        QPointF pt = assistant->adjustPosition(point, strokeBegin);
        if (pt.x() != pt.x()) continue;
        double d = qAbs(pt.x() - point.x()) + qAbs(pt.y() - point.y());
        if (d < distance) {
            best = pt;
            distance = d;
        }
    }
    return best;
}

void KisPaintingAssistantsDecoration::endStroke()
{
    foreach(KisPaintingAssistant* assistant, d->assistants) {
        assistant->endStroke();
    }
}

void KisPaintingAssistantsDecoration::drawDecoration(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter *converter,KisCanvas2* canvas)
{
    foreach(KisPaintingAssistant* assistant, d->assistants) {
        assistant->drawAssistant(gc, updateRect, converter,canvas);
    }
}

QList<KisPaintingAssistantHandleSP> KisPaintingAssistantsDecoration::handles()
{
    QList<KisPaintingAssistantHandleSP> hs;
    foreach(KisPaintingAssistant* assistant, d->assistants) {
        foreach(const KisPaintingAssistantHandleSP handle, assistant->handles()) {
            if (!hs.contains(handle)) {
                hs.push_back(handle);
            }
        }
        foreach(const KisPaintingAssistantHandleSP handle, assistant->sideHandles()) {
            if (!hs.contains(handle)) {
                hs.push_back(handle);
            }
        }
    }
    return hs;
}

QList<KisPaintingAssistant*> KisPaintingAssistantsDecoration::assistants()
{
    return d->assistants;
}
