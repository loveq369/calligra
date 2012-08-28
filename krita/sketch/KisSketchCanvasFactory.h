/* This file is part of the KDE project
 *
 * Copyright (c) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KISSKETCHCANVASFACTORY_H
#define KISSKETCHCANVASFACTORY_H

#include <kis_abstract_canvas_widget_factory.h>

class KisSketchCanvasFactory : public KisAbstractCanvasWidgetFactory
{
public:
    virtual KisCanvasWidgetBase* create(KisCanvas2* canvas, KisCoordinatesConverter* converter, QWidget* parent);
};

#endif // KISSKETCHCANVASFACTORY_H
