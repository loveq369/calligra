/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2005 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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

#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kgenericfactory.h>

#include <kis_view2.h>
#include <kis_types.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_layer.h>
#include <kis_statusbar.h>

#include "kis_separate_channels_plugin.h"
#include "kis_channel_separator.h"
#include "dlg_separate.h"

K_EXPORT_COMPONENT_FACTORY( kritaseparatechannels, KGenericFactory<KisSeparateChannelsPlugin>( "krita" ) )

KisSeparateChannelsPlugin::KisSeparateChannelsPlugin(QObject *parent, const QStringList &)
    : KParts::Plugin(parent)
{
    if ( parent->inherits("KisView2") ) {
        setInstance(KGenericFactory<KisSeparateChannelsPlugin>::instance());

        setXMLFile(KStandardDirs::locate("data","kritaplugins/imageseparate.rc"), true);
        m_view = (KisView2*) parent;
        KAction *action = new KAction(i18n("Separate Image..."), actionCollection(), "separate");
        connect(action, SIGNAL(triggered(bool) ), SLOT(slotSeparate()));
    }
}

KisSeparateChannelsPlugin::~KisSeparateChannelsPlugin()
{
}

void KisSeparateChannelsPlugin::slotSeparate()
{
    KisImageSP image = m_view->image();
    if (!image) return;

    KisLayerSP l = image->activeLayer();
    if (!l) return;

    KisPaintDeviceSP dev = image->activeDevice();
    if (!dev) return;

    DlgSeparate * dlgSeparate = new DlgSeparate(dev->colorSpace()->name(),
                                                image->colorSpace()->name(), m_view, "Separate");
    Q_CHECK_PTR(dlgSeparate);

    dlgSeparate->setCaption(i18n("Separate Image"));

    // If we're 8-bits, disable the downscale option
    if (dev->pixelSize() == dev->channelCount()) {
	dlgSeparate->enableDownscale(false);
    }

    if (dlgSeparate->exec() == QDialog::Accepted) {

        KisChannelSeparator separator(m_view);
        separator.separate(m_view->statusBar()->progress(),
                           dlgSeparate->getAlphaOptions(),
                           dlgSeparate->getSource(),
                           dlgSeparate->getOutput(),
                           dlgSeparate->getDownscale(),
                           dlgSeparate->getToColor());

    }

    delete dlgSeparate;

}

#include "kis_separate_channels_plugin.moc"
