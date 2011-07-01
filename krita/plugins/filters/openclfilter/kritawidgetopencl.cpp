/*
 * This file is part of Krita
 *
 * Copyright (c) 2011 Matus Talcik <matus.talcik@gmail.com>
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

#include "kritawidgetopencl.h"
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>

KritaWidgetOpenCL::KritaWidgetOpenCL(QWidget * parent) : KisConfigWidget(parent)
{
    textEdit = new QTextEdit;
    
    layout = new QHBoxLayout;
    layout->addWidget(textEdit);
    
    this->setLayout(layout);
}

KisPropertiesConfiguration* KritaWidgetOpenCL::configuration() const
{
    KisFilterConfiguration* config = new KisFilterConfiguration("OpenCL", 1);
    config->setProperty("kernel", textEdit->toPlainText());
    return config;
}

void KritaWidgetOpenCL::setConfiguration(const KisPropertiesConfiguration* config)
{
}

#include "kritawidgetopencl.moc"