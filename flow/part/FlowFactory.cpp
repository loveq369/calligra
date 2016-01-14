/* This file is part of the KDE project
   Copyright (C)  2006 Peter Simonsson <peter.simonsson@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "FlowFactory.h"

#include "FlowDocument.h"
#include "FlowAboutData.h"
#include "FlowPart.h"

#include <KoPluginLoader.h>

#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kiconloader.h>

KComponentData* FlowFactory::s_instance = 0;
KAboutData* FlowFactory::s_aboutData = 0;

static int factoryCount = 0;

FlowFactory::FlowFactory()
    : KPluginFactory()
{
    (void) componentData();

    if (factoryCount == 0) {

        // Load the KoPA-specific tools
        KoPluginLoader::instance()->load(QLatin1String("CalligraPageApp/Tool"));

        // Load Flow specific dockers
        KoPluginLoader::instance()->load(QLatin1String("Flow/Dock"));
    }
    factoryCount++;
}

FlowFactory::~FlowFactory()
{
  delete s_instance;
  s_instance = 0;
  delete s_aboutData;
  s_aboutData = 0;
}

QObject* FlowFactory::create( const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent,
                             const QVariantList& args, const QString& keyword )
{
    Q_UNUSED( args );
    Q_UNUSED( keyword );
    FlowPart *part = new FlowPart(parent);
    FlowDocument* doc = new FlowDocument(part);
    doc->setDefaultStylesResourcePath(QLatin1String("flow/styles/"));
    part->setDocument(doc);

    return part;
}

const KComponentData &FlowFactory::componentData()
{
  if (!s_instance) {
    s_instance = new KComponentData(aboutData());

    s_instance->dirs()->addResourceType("app_shape_collections", "data", "flow/stencils/");
    KIconLoader::global()->addAppDir("calligra");
  }

  return *s_instance;
}

KAboutData* FlowFactory::aboutData()
{
  if(!s_aboutData) {
    s_aboutData = newFlowAboutData();
  }

  return s_aboutData;
}

#include "FlowFactory.moc"
