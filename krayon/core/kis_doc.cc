/*
 *  kis_doc.cc - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2000 John Califf  <jcaliff@compuzone.net>
 *  Copyright (c) 2001 Toshitaka Fujioka  <fujioka@kde.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qpainter.h>
#include <qwidget.h>
#include <qregexp.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qbuffer.h>
#include <qimage.h>

#include <kcommand.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <kimageio.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <koFilterManager.h>
#include <koTemplateChooseDia.h>
#include <koStore.h>
#include <koStoreDevice.h>
#include <koMainWindow.h>
#include <koQueryTrader.h>

#include "kis_doc.h"
#include "kis_view.h"
#include "kis_shell.h"
#include "kis_factory.h"
#include "kis_dlg_new.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_channel.h"
#include "kis_selection.h"
#include "kis_framebuffer.h"
#include "kis_timer.h"

#define KIS_DEBUG(AREA, CMD)

/*
    KisDoc - constructor ko virtual method implemented
*/

KisDoc::KisDoc( QWidget *parentWidget, const char *widgetName,
    QObject* parent, const char* name, bool singleViewMode )
    : KoDocument( parentWidget, widgetName, parent, name, singleViewMode )
    , m_commands()
{
    bool loadPlugins = true;
    setInstance( KisFactory::global(), loadPlugins );

    m_pCurrent = 0L;
    m_pNewDialog = 0L;
    m_pClipImage = 0L;

    m_pSelection = new KisSelection(this);
    m_pFrameBuffer = new KisFrameBuffer(this);

    m_Images.setAutoDelete(false);

kdDebug(0) << "QPixmap::defaultDepth(): " << QPixmap::defaultDepth() << endl;

}

/*
    initDoc - ko virtual method implemented
*/

bool KisDoc::initDoc()
{

KisTimer::start();

    bool ok = false;
    QString name = i18n( "image %1" ).arg( m_Images.count() + 1 );

    // choose dialog for open mode
    QString templ;
    KoTemplateChooseDia::ReturnType ret;

    ret = KoTemplateChooseDia::choose (KisFactory::global(),
              templ,
              "application/x-krayon", "*.kra",
              i18n("Krayon"),
              KoTemplateChooseDia::NoTemplates,
              "krayon_template");

    // create document from template - use default
    // 512x512 RGBA image util we have real templates
    // however, this will never happen because KoTemplateChossDia
    // returns false if there is no templae selected

    if (ret == KoTemplateChooseDia::Template)
    {
        KisImage *img = newImage(name, 512, 512, cm_RGBA, 8);
        if (!img) return false;

        // add background layer
        img->addLayer(QRect(0, 0, 512, 512), KisColor::white(),
            false, i18n("background"));
        img->markDirty(QRect(0, 0, 512, 512));

        // list of images - mdi document
        setCurrentImage(img);

        // signal to tabbar for iages
        emit imageListUpdated();

        setModified (true);
        ok = true;
    }
    // open an existing document
    else if ( ret == KoTemplateChooseDia::File )
    {
        KURL url;
        url.setPath (templ);
        ok = openURL (url);
    }
    // create a new document from scratch
    else if ( ret == KoTemplateChooseDia::Empty )
    {
        // NewDialog for entering parameters
        ok = slotNewImage();
        // signal to tabbar for images
        if(ok) emit imageListUpdated();
    }

KisTimer::stop("initDoc()");

    return ok;
}


/*
    Save document (image) to xml format using QDomDocument
    ko virtual method implemented
*/

QDomDocument KisDoc::saveXML( )
{
    kdDebug(0) << "KisDoc::saveXML" << endl;

    // FIXME: implement saving of non-RGB modes.

    QDomDocument doc( "image" );
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
    doc.appendChild( saveImages( doc ) );

    setModified( false );
    return doc;
}

// save images
QDomElement KisDoc::saveImages( QDomDocument &doc )
{
    QStringList imageNames = images();
    QString tmp_currentImageName = currentImage();

    QDomElement images = doc.createElement( "images" );
    images.setAttribute( "editor", "Krayon" );
    images.setAttribute( "mime", "application/x-krayon" );
    images.setAttribute( "version", "1.2" );

    kdDebug(0) << "editor: " <<  "Krayon" << endl;
    kdDebug(0) << "mime: " << "application/x-krayon"  << endl;

    for ( QStringList::Iterator it = imageNames.begin(); it != imageNames.end(); ++it )
    {
        setImage( *it );
        KisImage *img = m_pCurrent;

        // image element
        QDomElement image = doc.createElement( "image" );
        image.setAttribute( "name", img->name() );
        image.setAttribute( "author", img->author() );
        image.setAttribute( "email", img->email() );
        image.setAttribute( "width", img->width() );
        image.setAttribute( "height", img->height() );
        image.setAttribute( "bitDepth", static_cast<int>(img->bitDepth()) );
        image.setAttribute( "cMode", static_cast<int>(img->colorMode()) );

        kdDebug(0) << "name: " <<  img->name() << endl;
        kdDebug(0) << "author: " <<  img->author() << endl;
        kdDebug(0) << "email: " <<  img->email() << endl;
        kdDebug(0) << "img->width(): " <<  img->width() << endl;
        kdDebug(0) << "img->height(): " <<  img->height() << endl;
        kdDebug(0) << "bitDepth " <<  static_cast<int>(img->bitDepth()) << endl;
        kdDebug(0) << "cMode: " <<  static_cast<int>(img->colorMode()) << endl;

        images.appendChild( image );

        // save layers
        image.appendChild( saveLayers( doc, img ) );
    } // end of images loop

    setImage( tmp_currentImageName );

    // save tool settings
    images.appendChild( saveToolSettings( doc ) );

    return images;
}

// save layers
QDomElement KisDoc::saveLayers( QDomDocument &doc, KisImage *img )
{
    // layers element - variable
    QDomElement layers = doc.createElement( "layers" );

    // layer elements
    kdDebug(0) << "layer elements" << endl;

    QPtrList<KisLayer> l_lst = img->layerList();
    for ( KisLayer *lay = l_lst.first(); lay != 0; lay = l_lst.next() )
    {
        QDomElement layer = doc.createElement( "layer" );

        layer.setAttribute( "name", lay->name() );
        layer.setAttribute( "x", lay->imageExtents().x() );
        layer.setAttribute( "y", lay->imageExtents().y() );
        layer.setAttribute( "width", lay->imageExtents().width() );
        layer.setAttribute( "height", lay->imageExtents().height() );
        layer.setAttribute( "opacity", static_cast<int>(lay->opacity()) );

        kdDebug(0) << "name: " <<  lay->name() << endl;
        kdDebug(0) << "x: " << lay->imageExtents().x()   << endl;
        kdDebug(0) << "y: " << lay->imageExtents().y()   << endl;
        kdDebug(0) << "width: " << lay->imageExtents().width()   << endl;
        kdDebug(0) << "height: " << lay->imageExtents().height()   << endl;
        kdDebug(0) << "opacity: " <<  static_cast<int>(lay->opacity())  << endl;

        if ( lay->visible() )
            layer.setAttribute( "visible", "true" );
        else
            layer.setAttribute( "visible", "false" );

        if ( lay->linked() )
            layer.setAttribute( "linked", "true" );
        else
            layer.setAttribute( "linked", "false" );

        layer.setAttribute( "bitDepth", static_cast<int>(lay->bitDepth()) );
        layer.setAttribute( "cMode", static_cast<int>(lay->colorMode()) );

        kdDebug(0) << "bitDepth: " <<  static_cast<int>(lay->bitDepth())  << endl;
        kdDebug(0) << "colorMode: " <<  static_cast<int>(lay->colorMode())  << endl;

        layers.appendChild( layer );

        // save channels
        layer.appendChild( saveChannels( doc, lay ) );
    } // end of layers loop

    return layers;
}

// save channels
QDomElement KisDoc::saveChannels( QDomDocument &doc, KisLayer *lay )
{
    // channels element - variable, normally maximum of 4 channels
    QDomElement channels = doc.createElement( "channels" );

    kdDebug(0) << "channel elements" << endl;

    // channel elements
    for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel() )
    {
        QDomElement channel = doc.createElement( "channel" );

        channel.setAttribute( "cId", static_cast<int>(ch->channelId()) );
        channel.setAttribute( "bitDepth", static_cast<int>(ch->bitDepth()) );

        kdDebug(0) << "cId: " <<  static_cast<int>(ch->channelId())  << endl;
        kdDebug(0) << "bitDepth: " <<  static_cast<int>(ch->bitDepth())  << endl;

        channels.appendChild( channel );
    } // end of channels loop

    return channels;
}

// save tool settings
QDomElement KisDoc::saveToolSettings( QDomDocument &doc )
{
    // tool element
    QDomElement tool = doc.createElement( "tool" );

    //save pen tool settings
    tool.appendChild( savePenToolSettings( doc ) );

    // save brush tool settings
    tool.appendChild( saveBrushToolSettings( doc ) );

    // save airbrush tool settings
    tool.appendChild( saveAirbrushToolSettings( doc ) );

    // save eraser tool settings
    tool.appendChild( saveEraserToolSettings( doc ) );

    // save line tool settings
    tool.appendChild( saveLineToolSettings( doc ) );

    // save polyline tool settings
    tool.appendChild( savePolylineToolSettings( doc ) );

    // save polygon tool settings
    tool.appendChild( savePolygonToolSettings( doc ) );

    // save rectangle tool settings
    tool.appendChild( saveRectangleToolSettings( doc ) );

    // save ellipse tool settings
    tool.appendChild( saveEllipseToolSettings( doc ) );

    // save filler tool settings
    tool.appendChild( saveFillerToolSettings( doc ) );

    // save Color changer settings
    tool.appendChild( saveColorChangerSettings( doc ) );

    // save Stamp (Pattern) tool settings
    tool.appendChild( saveStampToolSettings( doc ) );

    // save Gradients settings
    tool.appendChild( saveGradientsSettings( doc ) );


    return tool;
}

// save pen tool settings
QDomElement KisDoc::savePenToolSettings( QDomDocument &doc )
{
    // pen tool element
    QDomElement penTool = doc.createElement( "penTool" );

    penTool.setAttribute( "opacity", penToolSettings.opacity );
    penTool.setAttribute( "paintThreshold", penToolSettings.paintThreshold );
    penTool.setAttribute( "paintWithPattern", static_cast<int>( penToolSettings.paintWithPattern ) );
    penTool.setAttribute( "paintWithGradient", static_cast<int>( penToolSettings.paintWithGradient ) );

    return penTool;
}

// save brush tool settings
QDomElement KisDoc::saveBrushToolSettings( QDomDocument &doc )
{
    // brush tool element
    QDomElement brushTool = doc.createElement( "brushTool" );

    brushTool.setAttribute( "opacity", brushToolSettings.opacity );
    brushTool.setAttribute( "blendWithCurrentGradient", static_cast<int>( brushToolSettings.blendWithCurrentGradient ) );
    brushTool.setAttribute( "blendWithCurrentPattern", static_cast<int>( brushToolSettings.blendWithCurrentPattern ) );

    return brushTool;
}

// save airbrush tool settings
QDomElement KisDoc::saveAirbrushToolSettings( QDomDocument &doc )
{
    // airbrush tool element
    QDomElement airbrushTool = doc.createElement( "airbrushTool" );

    airbrushTool.setAttribute( "opacity", airbrushToolSettings.opacity );
    airbrushTool.setAttribute( "useCurrentGradient", static_cast<int>( airbrushToolSettings.useCurrentGradient ) );
    airbrushTool.setAttribute( "useCurrentPattern", static_cast<int>( airbrushToolSettings.useCurrentPattern ) );

    return airbrushTool;
}

// save eraser tool settings
QDomElement KisDoc::saveEraserToolSettings( QDomDocument &doc )
{
    // eraser tool element
    QDomElement eraserTool = doc.createElement( "eraserTool" );

    eraserTool.setAttribute( "opacity", eraserToolSettings.opacity );
    eraserTool.setAttribute( "blendWithCurrentGradient", static_cast<int>( eraserToolSettings.blendWithCurrentGradient ) );
    eraserTool.setAttribute( "blendWithCurrentPattern", static_cast<int>( eraserToolSettings.blendWithCurrentPattern ) );

    return eraserTool;
}

// save line tool settings
QDomElement KisDoc::saveLineToolSettings( QDomDocument &doc )
{
    // line tool element
    QDomElement lineTool = doc.createElement( "lineTool" );

    lineTool.setAttribute( "thickness", lineToolSettings.thickness );
    lineTool.setAttribute( "opacity", lineToolSettings.opacity );
    lineTool.setAttribute( "fillInteriorRegions", static_cast<int>( lineToolSettings.fillInteriorRegions ) );
    lineTool.setAttribute( "useCurrentPattern", static_cast<int>( lineToolSettings.useCurrentPattern ) );
    lineTool.setAttribute( "fillWithGradient", static_cast<int>( lineToolSettings.fillWithGradient ) );

    return lineTool;
}

// save polyline tool settings
QDomElement KisDoc::savePolylineToolSettings( QDomDocument &doc )
{
    // polyline tool element
    QDomElement polylineTool = doc.createElement( "polylineTool" );

    polylineTool.setAttribute( "thickness", polylineToolSettings.thickness );
    polylineTool.setAttribute( "opacity", polylineToolSettings.opacity );
    polylineTool.setAttribute( "fillInteriorRegions", static_cast<int>( polylineToolSettings.fillInteriorRegions ) );
    polylineTool.setAttribute( "useCurrentPattern", static_cast<int>( polylineToolSettings.useCurrentPattern ) );
    polylineTool.setAttribute( "fillWithGradient", static_cast<int>( polylineToolSettings.fillWithGradient ) );

    return polylineTool;
}

// save polygon tool settings
QDomElement KisDoc::savePolygonToolSettings( QDomDocument &doc )
{
    // polygon tool element
    QDomElement polygonTool = doc.createElement( "polygonTool" );

    polygonTool.setAttribute( "thickness", polygonToolSettings.thickness );
    polygonTool.setAttribute( "opacity", polygonToolSettings.opacity );
    polygonTool.setAttribute( "corners", polygonToolSettings.corners );
    polygonTool.setAttribute( "sharpness", polygonToolSettings.sharpness );
    polygonTool.setAttribute( "fillInteriorRegions", static_cast<int>( polygonToolSettings.fillInteriorRegions ) );
    polygonTool.setAttribute( "useCurrentPattern", static_cast<int>( polygonToolSettings.useCurrentPattern ) );
    polygonTool.setAttribute( "fillWithGradient", static_cast<int>( polygonToolSettings.fillWithGradient ) );
    polygonTool.setAttribute( "polygon", static_cast<int>( polygonToolSettings.polygon ) );
    polygonTool.setAttribute( "concavePolygon", static_cast<int>( polygonToolSettings.concavePolygon ) );

    return polygonTool;
}

// save rectangle tool settings
QDomElement KisDoc::saveRectangleToolSettings( QDomDocument &doc )
{
    // rectangle tool element
    QDomElement rectangleTool = doc.createElement( "rectangleTool" );

    rectangleTool.setAttribute( "thickness", rectangleToolSettings.thickness );
    rectangleTool.setAttribute( "opacity", rectangleToolSettings.opacity );
    rectangleTool.setAttribute( "fillInteriorRegions", static_cast<int>( rectangleToolSettings.fillInteriorRegions ) );
    rectangleTool.setAttribute( "useCurrentPattern", static_cast<int>( rectangleToolSettings.useCurrentPattern ) );
    rectangleTool.setAttribute( "fillWithGradient", static_cast<int>( rectangleToolSettings.fillWithGradient ) );

    return rectangleTool;
}

// save ellipse tool settings
QDomElement KisDoc::saveEllipseToolSettings( QDomDocument &doc )
{
    // ellipse tool element
    QDomElement ellipseTool = doc.createElement( "ellipseTool" );

    ellipseTool.setAttribute( "thickness", ellipseToolSettings.thickness );
    ellipseTool.setAttribute( "opacity", ellipseToolSettings.opacity );
    ellipseTool.setAttribute( "fillInteriorRegions", static_cast<int>( ellipseToolSettings.fillInteriorRegions ) );
    ellipseTool.setAttribute( "useCurrentPattern", static_cast<int>( ellipseToolSettings.useCurrentPattern ) );
    ellipseTool.setAttribute( "fillWithGradient", static_cast<int>( ellipseToolSettings.fillWithGradient ) );

    return ellipseTool;
}

// save filler tool settings
QDomElement KisDoc::saveFillerToolSettings( QDomDocument &doc )
{
    // filler tool element
    QDomElement fillerTool = doc.createElement( "fillerTool" );

    fillerTool.setAttribute( "opacity", fillerToolSettings.opacity );
    fillerTool.setAttribute( "fillWithPattern", static_cast<int>( fillerToolSettings.fillWithPattern ) );
    fillerTool.setAttribute( "fillWithGradient", static_cast<int>( fillerToolSettings.fillWithGradient ) );

    return fillerTool;
}

// save Color changer settings
QDomElement KisDoc::saveColorChangerSettings( QDomDocument &doc )
{
    // Color changer element
    QDomElement colorChanger = doc.createElement( "colorChanger" );

    colorChanger.setAttribute( "opacity", colorChangerSettings.opacity );
    colorChanger.setAttribute( "fillWithPattern", static_cast<int>( colorChangerSettings.fillWithPattern ) );
    colorChanger.setAttribute( "fillWithGradient", static_cast<int>( colorChangerSettings.fillWithGradient ) );

    return colorChanger;
}

// save Stamp (Pattern) tool settings
QDomElement KisDoc::saveStampToolSettings( QDomDocument &doc )
{
    // Stamp (Pattern) tool element
    QDomElement stampTool = doc.createElement( "stampTool" );

    stampTool.setAttribute( "opacity", stampToolSettings.opacity );
    stampTool.setAttribute( "blendWithCurrentGradient", static_cast<int>( stampToolSettings.blendWithCurrentGradient ) );

    return stampTool;
}

// save Gradients settings
QDomElement KisDoc::saveGradientsSettings( QDomDocument &doc )
{
    // gradients element
    QDomElement gradients = doc.createElement( "gradients" );

    gradients.setAttribute( "opacity", gradientsSettings.opacity );
    gradients.setAttribute( "offset", gradientsSettings.offset );
    gradients.setAttribute( "mode", gradientsSettings.mode );
    gradients.setAttribute( "blend", gradientsSettings.blend );
    gradients.setAttribute( "gradient", gradientsSettings.gradient );
    gradients.setAttribute( "repeat", gradientsSettings.repeat );

    return gradients;
}

/*
    Save extra, document-specific data outside xml format as defined by
    DTD for this document type.  It is appended to the saved
    xml document in gzipped format using the store methods of koffice
    common code koffice/lib/store/ as an internal file, not a real,
    separate file in the filesystem.

    In this case it's the binary image data
    Krayon can only handle rgb and rgba formats for now,
    by channel for each layer of the image saved in binary format

    ko virtual method implemented
*/

bool KisDoc::completeSaving( KoStore* store )
{
    kdDebug(0) << "KisDoc::completeSaving() entering" << endl;

    if (!store)         return false;
    if (!m_pCurrent)    return false;

    QStringList imageNames = images();
    QString tmp_currentImageName = currentImage();
    uint imageNumbers = 1;

    for ( QStringList::Iterator it = imageNames.begin(); it != imageNames.end(); ++it )
    {
        setImage( *it );
        QPtrList<KisLayer> layers = m_pCurrent->layerList();
        uint layerNumbers = 0;

        for ( KisLayer *lay = layers.first(); lay != 0; lay = layers.next())
        {
            for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel() )
            {
                QString image = QString( "image%1" ).arg( imageNumbers );
                QString layer;
                if ( layerNumbers == 0 )
                    layer = QString::fromLatin1( "background" );
                else
                    layer = QString( "layer%1" ).arg( layerNumbers );

                QString url = QString( "images/%1/layers/%2/channels/ch%3.bin" )
                              .arg( image )
                              .arg( layer )
                              .arg( static_cast<int>(ch->channelId()) );

                if ( store->open( url ) )
                {
                    ch->writeToStore(store);
                    store->close();
                }
            }
            ++layerNumbers;
        }
        ++imageNumbers;
    }
    kdDebug(0) << "KisDoc::completeSaving() leaving" << endl;
    setImage( tmp_currentImageName );

    return true;
}


/*
    loadXML - reimplements ko method
*/

bool KisDoc::loadXML( QIODevice *, const QDomDocument& doc )
{
    kdDebug(0) << "KisDoc::loadXML() entering" << endl;

    if ( doc.doctype().name() != "image" )
    {
        kdDebug(0) << "KisDoc::loadXML() no doctype name error" << endl;
        return false;
    }

    QDomElement images = doc.documentElement();

    if ( images.attribute( "mime" ) != "application/x-krayon" && images.attribute( "mime" ) != "application/vnd.kde.krayon" )
    {
        kdDebug(0) << "KisDoc::loadXML() no mime name error" << endl;
        return false;
    }

    if ( images.attribute( "version" ) != "1.2" )
    {
        kdDebug(0) << "KisDoc::loadXML() old file format" << endl;
        oldFileFormat = true;
        if ( !loadXMLOldFileFormat( images ) )
            return false;
    }
    else
    {
        oldFileFormat = false;
        // load images
        if ( !loadImages( images ) )
            return false;
    }

    kdDebug(0) << "KisDoc::loadXML() leaving succesfully" << endl;

    return true;
}

// load images
bool KisDoc::loadImages( QDomElement &element )
{
    QDomElement elem = element.firstChild().toElement();
    while ( !elem.isNull() )
    {
        if ( elem.tagName() == "image" )
        {
            // this assumes that we are loading an existing image
            // with certain attributes set

            QString name = elem.attribute( "name" );
            int w = elem.attribute( "width" ).toInt();
            int h = elem.attribute( "height" ).toInt();
            int cm = elem.attribute( "cMode" ).toInt();
            int bd = elem.attribute( "bitDepth" ).toInt();

            kdDebug(0) << "name: " << name << endl;
            kdDebug(0) << "width: " << w << endl;
            kdDebug(0) << "height: " << w << endl;
            kdDebug(0) << "cMode: " << cm << endl;
            kdDebug(0) << "bitDepth: " << bd << endl;

            cMode colorMode;

            switch ( cm )
            {
                case 0:
                    colorMode = cm_Indexed;
                    break;
                case 1:
                    colorMode = cm_Greyscale;
                    break;
                case 2:
                    colorMode = cm_RGB;
                    break;
                case 3:
                    colorMode = cm_RGBA;
                    break;
                case 4:
                    colorMode = cm_CMYK;
                    break;
                case 5:
                    colorMode = cm_CMYKA;
                    break;
                case 6:
                    colorMode = cm_Lab;
                    break;
                case 7:
                    colorMode = cm_LabA;
                    break;
                default:
                    return false;
            }

            KisImage *img = newImage( name, w, h, colorMode, bd );
            if ( !img ) return false;

            img->setAuthor( elem.attribute( "author" ) );
            img->setEmail( elem.attribute( "email" ) );

            // load layers
            if ( !loadLayers( elem, img ) )
                return false;

            setCurrentImage( img );
        }
        else if ( elem.tagName() == "tool" ) {
            // load tool settings
            loadToolSettings( elem );
        }

        elem = elem.nextSibling().toElement();
    }

    return true;
}

// load layers
bool KisDoc::loadLayers( QDomElement &element, KisImage *img )
{
    // layers element
    QDomElement layers = element.namedItem( "layers" ).toElement();
    if ( layers.isNull() )
    {
         kdDebug(0) << "KisDoc::loadXML(): layers.isNull() error!" << endl;
         return false;
    }

    // layer elements
    QDomNode l = layers.firstChild();

    while ( !l.isNull() )
    {
        QDomElement layer = l.toElement();
        if ( layer.tagName() == "layer" )
        {
            kdDebug(0) << "layer" << endl;

            QString layerName = layer.attribute( "name" );
            int w = layer.attribute( "width" ).toInt();
            int h = layer.attribute( "height" ).toInt();
            int x = layer.attribute( "x" ).toInt();
            int y = layer.attribute( "y" ).toInt();

            img->addLayer( QRect( x, y, w, h ), KisColor::white(), true, layerName );
            img->markDirty( QRect( x, y, w, h ) );

            KisLayer *lay = img->getCurrentLayer();
            lay->setOpacity( layer.attribute( "opacity" ).toInt() );

            if ( layer.attribute( "visible" ) == "true" )
                lay->setVisible( true );
            else
                lay->setVisible( false );

            if ( layer.attribute( "linked" ) == "true" )
                lay->setLinked( true );
            else
                lay->setLinked( false );

            // load channels
            loadChannels( layer, lay );
        }
        l = l.nextSibling();
    }

    return true;
}

// load channels
void KisDoc::loadChannels( QDomElement &element, KisLayer *lay )
{
    // channels element
    QDomElement channels = element.namedItem( "channels" ).toElement();
    if ( !channels.isNull() )
    {
        // channel elements
        QDomNode c = channels.firstChild();
        while ( !c.isNull() )
        {
            QDomElement channel = c.toElement();
            if ( channel.tagName() == "channel" )
            {
                kdDebug(0) << "channel" << endl;
                // TODO
            }
            c = c.nextSibling();
        }
    }
}

// load tool settings
void KisDoc::loadToolSettings( QDomElement &elem )
{
    QDomElement tool = elem.firstChild().toElement();

    while ( !tool.isNull() ) {
        if ( tool.tagName() == "penTool" )
            loadPenToolSettings( tool );
        else if ( tool.tagName() == "brushTool" )
            loadBrushToolSettings( tool );
        else if ( tool.tagName() == "airbrushTool" )
            loadAirbrushToolSettings( tool );
        else if ( tool.tagName() == "eraserTool" )
            loadEraserToolSettings( tool );
        else if ( tool.tagName() == "lineTool" )
            loadLineToolSettings( tool );
        else if ( tool.tagName() == "polylineTool" )
            loadPolylineToolSettings( tool );
        else if ( tool.tagName() == "polygonTool" )
            loadPolygonToolSettings( tool );
        else if ( tool.tagName() == "rectangleTool" )
            loadRectangleToolSettings( tool );
        else if ( tool.tagName() == "ellipseTool" )
            loadEllipseToolSettings( tool );
        else if ( tool.tagName() == "fillerTool" )
            loadFillerToolSettings( tool );
        else if ( tool.tagName() == "colorChanger" )
            loadColorChangerSettings( tool );
        else if ( tool.tagName() == "stampTool" )
            loadStampToolSettings( tool );
        else if ( tool.tagName() == "gradients" )
            loadGradientsSettings( tool );

        tool = tool.nextSibling().toElement();
    }
}

// load pen tool settings
void KisDoc::loadPenToolSettings( QDomElement &elem )
{
    penToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    penToolSettings.paintThreshold = elem.attribute( "paintThreshold" ).toInt();
    penToolSettings.paintWithPattern = static_cast<bool>( elem.attribute( "paintWithPattern" ).toInt() );
    penToolSettings.paintWithGradient = static_cast<bool>( elem.attribute( "paintWithGradient" ).toInt() );
}

// load brush tool settings
void KisDoc::loadBrushToolSettings( QDomElement &elem )
{
    brushToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    brushToolSettings.blendWithCurrentGradient = static_cast<bool>( elem.attribute( "blendWithCurrentGradient" ).toInt() );
    brushToolSettings.blendWithCurrentPattern = static_cast<bool>( elem.attribute( "blendWithCurrentPattern" ).toInt() );
}

// load airbrush tool settings
void KisDoc::loadAirbrushToolSettings( QDomElement &elem )
{
    airbrushToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    airbrushToolSettings.useCurrentGradient = static_cast<bool>( elem.attribute( "useCurrentGradient" ).toInt() );
    airbrushToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
}

// load eraser tool settings
void KisDoc::loadEraserToolSettings( QDomElement &elem )
{
    eraserToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    eraserToolSettings.blendWithCurrentGradient = static_cast<bool>( elem.attribute( "blendWithCurrentGradient" ).toInt() );
    eraserToolSettings.blendWithCurrentPattern = static_cast<bool>( elem.attribute( "blendWithCurrentPattern" ).toInt() );
}

// load line tool settings
void KisDoc::loadLineToolSettings( QDomElement &elem )
{
    lineToolSettings.thickness = elem.attribute( "thickness" ).toInt();
    lineToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    lineToolSettings.fillInteriorRegions = static_cast<bool>( elem.attribute( "fillInteriorRegions" ).toInt() );
    lineToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
    lineToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
}

// load polyline tool settings
void KisDoc::loadPolylineToolSettings( QDomElement &elem )
{
    polylineToolSettings.thickness = elem.attribute( "thickness" ).toInt();
    polylineToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    polylineToolSettings.fillInteriorRegions = static_cast<bool>( elem.attribute( "fillInteriorRegions" ).toInt() );
    polylineToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
    polylineToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );

}

// load polygon tool settings
void KisDoc::loadPolygonToolSettings( QDomElement &elem )
{
    polygonToolSettings.thickness = elem.attribute( "thickness" ).toInt();
    polygonToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    polygonToolSettings.corners = elem.attribute( "corners" ).toInt();
    polygonToolSettings.sharpness = elem.attribute( "sharpness" ).toInt();
    polygonToolSettings.fillInteriorRegions = static_cast<bool>( elem.attribute( "fillInteriorRegions" ).toInt() );
    polygonToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
    polygonToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
    polygonToolSettings.polygon = static_cast<bool>( elem.attribute( "polygon" ).toInt() );
    polygonToolSettings.concavePolygon = static_cast<bool>( elem.attribute( "concavePolygon" ).toInt() );
}

// load rectangle tool settings
void KisDoc::loadRectangleToolSettings( QDomElement &elem )
{
    rectangleToolSettings.thickness = elem.attribute( "thickness" ).toInt();
    rectangleToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    rectangleToolSettings.fillInteriorRegions = static_cast<bool>( elem.attribute( "fillInteriorRegions" ).toInt() );
    rectangleToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
    rectangleToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
}

// load ellipse tool settings
void KisDoc::loadEllipseToolSettings( QDomElement &elem )
{
    ellipseToolSettings.thickness = elem.attribute( "thickness" ).toInt();
    ellipseToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    ellipseToolSettings.fillInteriorRegions = static_cast<bool>( elem.attribute( "fillInteriorRegions" ).toInt() );
    ellipseToolSettings.useCurrentPattern = static_cast<bool>( elem.attribute( "useCurrentPattern" ).toInt() );
    ellipseToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
}

// load filler tool settings
void KisDoc::loadFillerToolSettings( QDomElement &elem )
{
    fillerToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    fillerToolSettings.fillWithPattern = static_cast<bool>( elem.attribute( "fillWithPattern" ).toInt() );
    fillerToolSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
}

// load Color changer settings
void KisDoc::loadColorChangerSettings( QDomElement &elem )
{
    colorChangerSettings.opacity = elem.attribute( "opacity" ).toInt();
    colorChangerSettings.fillWithPattern = static_cast<bool>( elem.attribute( "fillWithPattern" ).toInt() );
    colorChangerSettings.fillWithGradient = static_cast<bool>( elem.attribute( "fillWithGradient" ).toInt() );
}

// load Stamp (Pattern) tool settings
void KisDoc::loadStampToolSettings( QDomElement &elem )
{
    stampToolSettings.opacity = elem.attribute( "opacity" ).toInt();
    stampToolSettings.blendWithCurrentGradient = static_cast<bool>( elem.attribute( "blendWithCurrentGradient" ).toInt() );
}

// load Gradients settings
void KisDoc::loadGradientsSettings( QDomElement &elem )
{
    gradientsSettings.opacity = elem.attribute( "opacity" ).toInt();
    gradientsSettings.offset = elem.attribute( "offset" ).toInt();
    gradientsSettings.mode = elem.attribute( "mode" );
    gradientsSettings.blend = elem.attribute( "blend" );
    gradientsSettings.gradient = elem.attribute( "gradient" );
    gradientsSettings.repeat = elem.attribute( "repeat" );
}

bool KisDoc::completeLoading( KoStore* store )
{
    kdDebug(0) << "KisDoc::completeLoading() entering" << endl;

    if ( !store )  return false;
    if ( !m_pCurrent) return false;

    if ( !oldFileFormat )
    {
        QStringList imageNames = images();
        QString tmp_currentImageName = currentImage();
        uint imageNumbers = 1;

        for ( QStringList::Iterator it = imageNames.begin(); it != imageNames.end(); ++it )
        {
            setImage( *it );
            QPtrList<KisLayer> layers = m_pCurrent->layerList();
            uint layerNumbers = 0;

            for ( KisLayer *lay = layers.first(); lay != 0; lay = layers.next() )
            {
                for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel() )
                {
                    QString image = QString( "image%1" ).arg( imageNumbers );
                    QString layer;
                    if ( layerNumbers == 0 )
                        layer = QString::fromLatin1( "background" );
                    else
                        layer = QString( "layer%1" ).arg( layerNumbers );

                    QString url = QString( "images/%1/layers/%2/channels/ch%3.bin" )
                                  .arg( image )
                                  .arg( layer )
                                  .arg( static_cast<int>(ch->channelId()) );

                    if ( store->open( url ) )
                    {
                        kdDebug(0) << "KisDoc::completeLoading() ch->loadFromStore()" << endl;
                        ch->loadFromStore( store );
                        store->close();
                    }
                }
                ++layerNumbers;
            }
            ++imageNumbers;

            // need this to force redraw of image data just loaded
            current()->markDirty( QRect( 0, 0, current()->width(), current()->height() ) );
            setCurrentImage( current() );
        }
    }
    else
        completeLoadingOldFileFormat( store );

    kdDebug(0) << "KisDoc::completeLoading() leaving" << endl;
    return true;
}

// load old file format
bool KisDoc::loadXMLOldFileFormat( QDomElement &image )
{
    // this assumes that we are loading an existing image
    // with certain attributes set

    QString name = image.attribute( "name" );
    int w = image.attribute( "width" ).toInt();
    int h = image.attribute( "height" ).toInt();
    int cm = image.attribute( "cMode" ).toInt();
    int bd = image.attribute( "bitDepth" ).toInt();

    kdDebug(0) << "name: " << name << endl;
    kdDebug(0) << "width: " << w << endl;
    kdDebug(0) << "height: " << w << endl;
    kdDebug(0) << "cMode: " << cm << endl;
    kdDebug(0) << "bitDepth: " << bd << endl;

    cMode colorMode;

    switch ( cm )
    {
        case 0:
            colorMode = cm_Indexed;
            break;
        case 1:
            colorMode = cm_Greyscale;
            break;
        case 2:
            colorMode = cm_RGB;
            break;
        case 3:
            colorMode = cm_RGBA;
            break;
        case 4:
            colorMode = cm_CMYK;
            break;
        case 5:
            colorMode = cm_CMYKA;
            break;
        case 6:
            colorMode = cm_Lab;
            break;
        case 7:
            colorMode = cm_LabA;
            break;
        default:
            return false;
    }

    KisImage *img = newImage( name, w, h, colorMode, bd );
    if ( !img ) return false;

    img->setAuthor( image.attribute( "author" ) );
    img->setEmail( image.attribute( "email" ) );

    // layers element
    QDomElement layers = image.namedItem( "layers" ).toElement();
    if ( layers.isNull() )
    {
        kdDebug(0) << "KisDoc::loadXMLOldFileFormat(): layers.isNull() error!" << endl;
        return false;
    }

    // layer elements
    QDomNode l = layers.firstChild();

    while ( !l.isNull() )
    {
        QDomElement layer = l.toElement();
        if ( layer.tagName() != "layer" ) continue;

        kdDebug(0) << "layer" << endl;

        QString layerName = layer.attribute( "name" );
        int w = layer.attribute( "width" ).toInt();
        int h = layer.attribute( "height" ).toInt();
        int x = layer.attribute( "x" ).toInt();
        int y = layer.attribute( "y" ).toInt();

        img->addLayer( QRect( x, y, w, h ), KisColor::white(), true, layerName );
        img->markDirty( QRect( x, y, w, h ) );

        KisLayer *lay = img->getCurrentLayer();
        lay->setOpacity( layer.attribute( "opacity" ).toInt() );

        if ( layer.attribute( "visible" ) == "true" )
            lay->setVisible( true );
        else
            lay->setVisible( false );

        if ( layer.attribute( "linked" ) == "true" )
            lay->setLinked( true );
        else
            lay->setLinked( false );

        // channels element
        QDomElement channels = layer.namedItem( "channels" ).toElement();
        if ( channels.isNull() ) continue;

        // channel elements
        QDomNode c = channels.firstChild();

        while ( !c.isNull() )
        {
            QDomElement channel = c.toElement();
            if (channel.tagName() != "channel" ) continue;

            kdDebug(0) << "channel" << endl;
            c = c.nextSibling();
        }

        l = l.nextSibling();
    }

    setCurrentImage( img );

    return true;
}

void KisDoc::completeLoadingOldFileFormat( KoStore *store )
{
    QPtrList<KisLayer> layers = m_pCurrent->layerList();

    for ( KisLayer *lay = layers.first(); lay != 0; lay = layers.next() )
    {
        for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel() )
        {
            QString url = QString( "layers/%1/channels/ch%2.bin" ).arg( lay->name() )
                          .arg( static_cast<int>(ch->channelId()) );

            if ( store->open( url ) )
            {
                kdDebug(0) << "KisDoc::completeLoadingOldFileFormat() ch->loadFromStore()" << endl;
                ch->loadFromStore( store );
                store->close();
            }
        }
    }

    // need this to force redraw of image data just loaded
    current()->markDirty( QRect( 0, 0, current()->width(), current()->height() ) );
    setCurrentImage( current() );
}

/*
    setCurrentImage - using pointer to a KisImage - this is
    normally done from the view.
*/

void KisDoc::setCurrentImage(KisImage *img)
{
    if (m_pCurrent)
    {
        // disconnect old current image
        QObject::disconnect( m_pCurrent, SIGNAL( updated() ),
	            this, SLOT( slotImageUpdated() ) );
        QObject::disconnect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
	            this, SLOT( slotImageUpdated( const QRect& ) ) );
        QObject::disconnect( m_pCurrent, SIGNAL( layersUpdated() ),
                    this, SLOT( slotLayersUpdated() ) );
    }

    m_pCurrent = img;

    if(m_pCurrent)
    {
        // connect new current image
        QObject::connect( m_pCurrent, SIGNAL( updated() ),
		    this, SLOT( slotImageUpdated() ) );
        QObject::connect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
		    this, SLOT( slotImageUpdated( const QRect& ) ) );
        QObject::connect( m_pCurrent, SIGNAL( layersUpdated() ),
		    this, SLOT( slotLayersUpdated() ) );
    }

    // signal to tabbar for images - kis_view.cc
    emit imageListUpdated();
    // signal to current image - kis_image.cc
    emit layersUpdated();
    // signal to view to update contents - kis_view.cc
    emit docUpdated();
}


/*
    setCurrentImage - by name
*/

void KisDoc::setCurrentImage(const QString& _name)
{
    KisImage *img = m_Images.first();

    while (img)
    {
        if (img->name() == _name)
	    {
	        setCurrentImage(img);
	        return;
	    }

        img = m_Images.next();
    }
}


/*
    renameImage - from menu or click on image tab
*/

void KisDoc::renameImage(QString & oldName, QString & newName)
{
    KisImage *img = m_Images.first();

    while (img)
    {
        if(img->name() == oldName)
        {
            img->setName(newName);
            break;
        }
        img = m_Images.next();
    }

    emit imageListUpdated();
}


/*
    images - build list of images by name
*/

QStringList KisDoc::images()
{
    QStringList lst;
    KisImage *img = m_Images.first();

    while (img)
    {
        lst.append(img->name());
        img = m_Images.next();
    }

    return lst;
}

/*
    isEmpty - has no image.  This is not an error condition but
    still need to be check occasionally for operations which
    require an image
*/
bool KisDoc::isEmpty() const
{
    if (m_pCurrent) return false;
    return true;
}

/*
    currentView - pointer to current view for this doc
*/
KisView *KisDoc::currentView()
{
/* ### what's this? (Simon
    KisView *v = 0L;
    for( v = (KisView*)firstView(); v != 0L; v = (KisView*)nextView() )
    {
        // how to tell which of these is current?  too much encapsualtion!
        //v->enableUndo( _b );
    }

    return v;
*/
    return dynamic_cast<KisView *>( views().getLast() );
}


/*
    currentImage - name of current image
*/
QString KisDoc::currentImage()
{
    if (m_pCurrent) return m_pCurrent->name();
    return QString("");
}


/*
    current - pointer to current image
*/
KisImage* KisDoc::current()
{
    return m_pCurrent;
}

/*
    resetShells - touch all shells for this document to
    force a fake resize and show scrollbars in the view(s)
    This is necessary when a new current image is established
    which may have a different size from the former one.
    resetShells() is invoked by the docUpdated() signal sent to the
    view, from the view's docUpdated(), as a result of the current
    image changing.  See the method setCurrentImage() in this class.
    What a roundabout way to do the simplest thing!
*/
void KisDoc::resetShells()
{
    int shellCount = 0;
    QPtrListIterator<KoMainWindow> it( shells() );

    for ( ; it.current(); ++it )
    {
        KoMainWindow *tmpKo = it.current();

        shellCount++;

        kdDebug() << "KisShell Number: " << shellCount << endl;

        int shellWidth  = tmpKo->width();
        int shellHeight = tmpKo->height();

        tmpKo->resize(shellWidth - 1, shellHeight - 1);

        //kdDebug() << "KisShell width  - 1: "  << shellWidth  << endl;
        //kdDebug() << "KisShell height - 1: "  << shellHeight << endl;

        tmpKo->resize(shellWidth, shellHeight);

        //kdDebug() << "KisShell width: "  << shellWidth  << endl;
        //kdDebug() << "KisShell height: " << shellHeight << endl;
    }
}

/*
    currentShell - pointer to current main window for doc
    Right now it just returns a ptr to the first shell for
    this doc until it can be determined which shell contains
    the active view (and which view is active!).
*/
KoMainWindow * KisDoc::currentShell()
{
    return shells().getFirst();
}

/*
    KisDoc destructor - Note that since this is MDI, each image
    in the list must be deleted to free up all memory.  While
    there is only ONE Koffice "document", there are multiple
    images you can load and work with in any session
*/

KisDoc::~KisDoc()
{
    KisImage *img = m_Images.first();

    while (img)
    {
        // disconnect old current image
        if(img == m_pCurrent)
        {
            QObject::disconnect( m_pCurrent, SIGNAL( updated() ),
	            this, SLOT( slotImageUpdated() ) );
            QObject::disconnect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
	            this, SLOT( slotImageUpdated( const QRect& ) ) );
            QObject::disconnect( m_pCurrent, SIGNAL( layersUpdated() ),
                this, SLOT( slotLayersUpdated() ) );
        }

        delete img;
        img = m_Images.next();
    }

    if(m_pClipImage != 0L)
    {
        delete m_pClipImage;
        m_pClipImage = 0L;
    }

    if(m_pSelection != 0L)
    {
        delete m_pSelection;
        m_pSelection = 0L;
    }

    if(m_pFrameBuffer != 0L)
    {
        delete m_pFrameBuffer;
        m_pFrameBuffer = 0L;
    }
}


/*
    Save current document (image) in a standard image format
    Note that only the current visible layer(s) will be saved
    usually one needs to merge all layers first, as with Gimp

    The format the image is saved in is determined solely by
    the file extension used. (Although the name of the original
    file, if any, should be the default file name and its extension
    the default extension.  An imagi information dialog and
    extended file save dialog giving the user more choice about
    the exact save format to use is needed, eventually).

    To insure 32 bit images being exported, before the converstion to
    a specific file format, we get the data directly from the layer
    channels, and put it into a QImage, without using the QPimap paint
    device.  Therefore the user's hardware has no effect on the depth
    of the save file.

    There also need to be another method just to save the current view
    as an image, mostly for debugging and documentation purposes, but it
    will only save at the display depth of the hardware -  often 16 bit,
    because it gets a QPixmap from the canvas instead of a QImage from
    the layer(s).

*/

bool KisDoc::saveAsQtImage( QString file, bool wholeImage)
{
    int x, y, w, h;
    bool ok = false;

    // current layer only
    if(!wholeImage)
    {
        x = current()->getCurrentLayer()->layerExtents().left();
        y = current()->getCurrentLayer()->layerExtents().top();
        w = current()->getCurrentLayer()->layerExtents().width();
        h = current()->getCurrentLayer()->layerExtents().height();
    }
    // all layers
    else
    {
        x = current()->getCurrentLayer()->imageExtents().left();
        y = current()->getCurrentLayer()->imageExtents().top();
        w = current()->getCurrentLayer()->imageExtents().width();
        h = current()->getCurrentLayer()->imageExtents().height();
    }

    QImage *qimg = new QImage(w, h, 32);
    if(qimg)
    {
        qimg->setAlphaBuffer(current()->colorMode() == cm_RGBA ? true : false);
        QRect saveRect = QRect(x, y, w, h);
        LayerToQtImage(qimg, 0, saveRect);
        ok = qimg->save(file, KImageIO::type(file).ascii());
        delete qimg;
    }

    return ok;

// old - this should be used for saving or capturing the view instead.

#if 0

    // prepare a pixmap for drawing
    QPixmap *pix = new QPixmap (w, h);
    if (pix == 0L)
    {
        kdDebug(0) << "KisDoc::saveAsQtImage: can't create QPixmap" << endl;
        return false;
    }

    QPainter p;
    p.begin (pix);
    p.setBackgroundColor (Qt::white);
    p.eraseRect (x, y, w, h);
    QRect saveRect = QRect(x, y, w, h);
    paintContent(p, saveRect);
    p.end ();

    // now create an image
    QImage qimg  = pix->convertToImage();
    qimg.setAlphaBuffer(current()->colorMode() == cm_RGBA ? true : false);

    // clean up pixmap
    delete pix;

    // save the image in requested format. file extension
    // determines image format - best way
    return qimg.save(file, KImageIO::type(file).ascii());

#endif
}


/*
    Copy a QImage exactly into the current image's active layer,
    pixel by pixel using scanlines,  fully 32 bit even if the alpha
    channel isn't used.
*/

bool KisDoc::QtImageToLayer(QImage *qimg, KisView * /* pView */)
{
    KisImage *img = current();
    if(!img) return false;

    KisLayer *lay = img->getCurrentLayer();
    if(!lay) return false;

    if(qimg->depth() < 16)
    {
        QImage cI = qimg->smoothScale(qimg->width(), qimg->height());
        qimg = &cI;
    }

    if(qimg->depth() < 32)
    {
        kdDebug() << "qimg depth is less than 32" << endl;
        kdDebug() << "qimg depth is: " << qimg->depth() << endl;

        qimg->convertDepth(32);
    }

    qimg->setAlphaBuffer(true);

    bool layerGrayScale = false;
    bool qimageGrayScale = false;

    int startx = 0;
    int starty = 0;

    QRect clipRect(startx, starty, qimg->width(), qimg->height());

    if (!clipRect.intersects(lay->imageExtents()))
        return false;

    clipRect = clipRect.intersect(lay->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar r, a;

    bool alpha = (img->colorMode() == cm_RGBA);

    for (int y = sy; y <= ey; y++)
    {
        sl = qimg->scanLine(y);

        for (int x = sx; x <= ex; x++)
	    {
            uint *p = (uint *)qimg->scanLine(y) + x;

            //QRgb *p = (QRgb *)qimg->scanLine(y) + x;

            if(layerGrayScale)
            {
                /* only if qimage is gray scale - in which case all
                values are packed into the red channel if converted
                to 32 bit already - can test above there should be no
                8 or 16 bit QImages in Krayon */
                if(qimageGrayScale)
                {
	                r = *(sl + x);
                }
                /* rgb qimage, but we are in grayscale mode
                average rgb values to convert to 32 bit
                gray scale - actually only shows 256 shades
                of gray because all channels are same value */
                else
                {
	                r = (qRed(*p) + qGreen(*p) + qBlue(*p))/3;
                }

	            lay->setPixel(0, startx + x, starty + y, r);
	            lay->setPixel(1, startx + x, starty + y, r);
	            lay->setPixel(2, startx + x, starty + y, r);
            }
            else
            {
	            lay->setPixel(0, startx + x, starty + y, qRed(*p));
	            lay->setPixel(1, startx + x, starty + y, qGreen(*p));
	            lay->setPixel(2, startx + x, starty + y, qBlue(*p));
            }

            if (alpha)
	        {
                /* We need to get alpha value from qimg and this
                will not work with 16 bit images correctly, but we
                have already converted to 32 bit above */

                a = qAlpha(*p);
		        lay->setPixel(3, startx + x, starty + y, a);
	        }
	    }
    }

    return true;
}


/*
    Copy a rectangular area of a layer into a QImage, pixel by pixel
    using scanlines, fully 32 bit even if the alpha channel isn't used.
    This provides a basis for a clipboard buffer and a Krayon blit
    routine, with custom modifiers to blend, apply various filters and
    raster operations, and many other neat effects.
*/

bool KisDoc::LayerToQtImage(QImage *qimg, KisView */*pView*/, QRect & clipRect)
{
    KisImage *img = current();
    if (!img)  return false;

    KisLayer *lay = img->getCurrentLayer();
    if (!lay)  return false;

    // this may not always be zero, but for current
    // uses it will be, as the entire layer is copied
    // from its offset into the image
    int startx = 0;
    int starty = 0;

    // this insures getting the layer from its offset
    // into the image, which may not be zero if the
    // layer has been moved
    if (!clipRect.intersects(lay->imageExtents()))
        return false;

    clipRect = clipRect.intersect(lay->imageExtents());

    int sx = clipRect.left();
    int sy = clipRect.top();
    int ex = clipRect.right();
    int ey = clipRect.bottom();

    uchar *sl;
    uchar r, g, b;
    uchar a = 255;

    bool alpha = (img->colorMode() == cm_RGBA);

    for (int y = sy; y <= ey; y++)
    {
        sl = qimg->scanLine(y);

        for (int x = sx; x <= ex; x++)
	    {
            // layer binary values by channel
	        r = lay->pixel(0, startx + x, starty + y);
	        g = lay->pixel(1, startx + x, starty + y);
	        b = lay->pixel(2, startx + x, starty + y);
            if(alpha) a = lay->pixel(3, startx + x, starty + y);

            uint *p = (uint *)qimg->scanLine(y - sy) + (x - sx);
            *p = alpha ? qRgba(r, g, b, a) : qRgb(r, g, b);
	    }
    }

    return true;
}

/*
    setSelection - set selection for document
*/
void KisDoc::setSelection(QRect & r)
{
    m_pSelection->setBounds(r);
}

/*
    clearSelection - clear selection for document
*/
void KisDoc::clearSelection()
{
    m_pSelection->setNull();
}


/*
   hasSelection - does this document have a document-wide selection?
*/
bool KisDoc::hasSelection()
{
    return (m_pSelection->getRectangle().isNull() ? false : true);
}

/*
    removeClipImage - delete the current clip image and nullify it
*/
void KisDoc::removeClipImage()
{
    if(m_pClipImage != 0L)
    {
        delete m_pClipImage;
        m_pClipImage = 0L;
    }
}

/*
    setClipImage - set current clip image for the document
    from the selection
*/
bool KisDoc::setClipImage()
{
    KisImage *img = current();
    if(!img) return false;

    KisLayer *lay = img->getCurrentLayer();
    if(!lay) return false;

    if(m_pClipImage != 0L)
    {
        delete m_pClipImage;
        m_pClipImage = 0L;
    }

    QRect selectRect = getSelection()->getRectangle();

    // create a clip image same size, depth, etc., as selection image
    m_pClipImage = new QImage(selectRect.width(), selectRect.height(), 32);
    if(!m_pClipImage) return false;

    // we will need alpha channel for masking the unselected pixels out
    m_pClipImage->setAlphaBuffer(true);

    // make a deep copy of the selection image, if there is one
    if(getSelection()->getImage().isNull())
        return false;
    else
        *m_pClipImage = getSelection()->getImage();

    return true;
}


KisImage* KisDoc::newImage(const QString& n, int width, int height,
    cMode cm , uchar bitDepth )
{
    KisImage *img = new KisImage( n, width, height, cm, bitDepth );
    m_Images.append(img);
    return img;
}


void KisDoc::removeImage( KisImage *img )
{
    m_Images.remove(img);
    delete img;

    if(m_Images.count() > 1)
    {
        setCurrentImage(m_Images.first());
    }
    else
    {
        setCurrentImage(0L);
    }
}


void KisDoc::slotRemoveImage( const QString& _name )
{
    KisImage *img = m_Images.first();

    while (img)
    {
        if (img->name() == _name)
	    {
	        removeImage(img);
	        return;
	    }

        img = m_Images.next();
    }
}

/*
    slotnewImage - Create a new image for this document and set the
    current image to it. There can be more than one image for each doc
*/

bool KisDoc::slotNewImage()
{
    if (!m_pNewDialog) m_pNewDialog = new NewDialog();

    /* This dialog causes bad drawable or invalid window paramater.
    It seems harmless, though, just a message about an Xerror.
    Error only occurs when document is first created and has no
    content, not when adding new image to an existing document */

    m_pNewDialog->exec();

    if(!m_pNewDialog->result() == QDialog::Accepted)
        return false;

    int w = m_pNewDialog->newwidth();
    int h = m_pNewDialog->newheight();
    bgMode bg = m_pNewDialog->backgroundMode();
    cMode cm = m_pNewDialog->colorMode();

    kdDebug() << "KisDoc::slotNewImage: w: "<< w << "h: " << h << endl;

    QString name, desiredName;
    int numero = 1;
    unsigned int runs = 0;

    /* don't allow duplicate image names if some images have
    been removed leaving "holes" in name sequence */

    do {
        desiredName = i18n( "image %1" ).arg( numero );
        KisImage *currentImg = m_Images.first();

        while (currentImg)
        {
            if (currentImg->name() == desiredName)
            {
                numero++;
            }
            currentImg = m_Images.next();
        }
        runs++;

    } while(runs < m_Images.count());

    name = i18n( "image %1" ).arg( numero );

    KisImage *img = newImage(name, w, h, cm, 8);
    if (!img) return false;

    kdDebug() << "KisDoc::slotNewImage: returned from newImage()" << endl;

    // add background layer

    if (bg == bm_White)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, i18n("background"));

    else if (bg == bm_Transparent)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), true, i18n("background"));

    else if (bg == bm_ForegroundColor)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, i18n("background"));

    else if (bg == bm_BackgroundColor)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, i18n("background"));

    kdDebug() << "KisDoc::slotNewImage: returned from addLayer()" << endl;

    img->markDirty(QRect(0, 0, w, h));
    setCurrentImage(img);

    return true;
}

/*
    Mime type for this app - not same as file type, but file types
    can be associated with a mime type and are opened with applications
    associated with the same mime type
*/

QCString KisDoc::mimeType() const
{
    return "application/x-krayon";
}


/*
    Create view instance for this document - there can be more than
    one view of a document open at any time - a list of the views for
    this document can be obtained somehow - perhaps from the factory
    or some kind of view manager - needed
*/

KoView* KisDoc::createViewInstance( QWidget* parent, const char* name )
{
    if(name == 0) name = "View";
    KisView* view = new KisView( this, parent, name );

    // undo-redo

    QObject::connect( &m_commands, SIGNAL( undoRedoChanged( QString, QString ) ),
                    view, SLOT( slotUndoRedoChanged( QString, QString ) ) );
    QObject::connect( &m_commands, SIGNAL( undoRedoChanged( QStringList, QStringList ) ),
                    view, SLOT( slotUndoRedoChanged( QStringList, QStringList ) ) );

    return (view);
}


/*
    creatShell - Create view shell or top level window for document
    The document can have more than one shell if there are multiple
    views (not split) each with its own shell window
*/

KoMainWindow* KisDoc::createShell()
{
    KoMainWindow* shell = new KisShell;
    shell->show();

    return shell;
}

/*
    Draw current image on canvas - indirect approach
*/

void KisDoc::paintContent( QPainter& painter,
        const QRect& rect, bool /*transparent*/, double /*zoomX*/, double /*zoomY*/ )
{
    // TODO support zooming
    if (m_pCurrent)
    {
        m_pCurrent->paintPixmap( &painter, rect );
    }
    else
    {
        kdDebug(0) <<  "KisDoc::paintContent() - no m_pCurrent" << endl;
    }
}

/*
    Draw current image on canvas - direct apprach
*/

void KisDoc::paintPixmap(QPainter *p, QRect area)
{
    if (m_pCurrent)
    {
        m_pCurrent->paintPixmap(p, area);
    }
    else
    {
        kdDebug(0) <<  "KisDoc::paintPixmap() - no m_pCurrent" << endl;
    }
}

/*
    let document update view when image is changed
*/
void KisDoc::slotImageUpdated()
{
    // signal to view to update contents - kis_view.cc
    emit docUpdated();
}

/*
    let document update specific area of view when image is changed
*/

void KisDoc::slotImageUpdated( const QRect& rect )
{
    // signal to view to update contents - kis_view.cc
    emit docUpdated(rect);
}


void KisDoc::slotLayersUpdated()
{
    // signal to image - kis_image.cc
    emit layersUpdated();
}

QRect KisDoc::getImageRect()
{
    QRect imageRect( 0, 0, m_pCurrent->width(), m_pCurrent->height() );
    return imageRect;
}

void KisDoc::setImage( QString imageName )
{
    KisImage *img;
    for ( img = m_Images.first(); img != 0; img = m_Images.next() ) {
        if ( img->name() == imageName ) {
            m_pCurrent = img;
            return;
        }
    }
}

// Set Pen tool settings
void KisDoc::setPenToolSettings( PenToolSettings s )
{
    penToolSettings = s;
}

// Set Brush tool settings
void KisDoc::setBrushToolSettings( BrushToolSettings s )
{
    brushToolSettings = s;
}

// Set Airbrush tool settings
void KisDoc::setAirbrushToolSettings( AirbrushToolSettings s )
{
    airbrushToolSettings = s;
}

// Eraser tool settings
void KisDoc::setEraserToolSettings( EraserToolSettings s )
{
    eraserToolSettings = s;
}

// Set Line tool settings
void KisDoc::setLineToolSettings( LineToolSettings s )
{
    lineToolSettings = s;
}

// Set Polyline tool settings
void KisDoc::setPolylineToolSettings( PolylineToolSettings s )
{
    polylineToolSettings = s;
}

// Set Polygon tool settings
void KisDoc::setPolygonToolSettings( PolygonToolSettings s )
{
    polygonToolSettings = s;
}

// Set Rectangle tool settings
void KisDoc::setRectangleToolSettings( RectangleToolSettings s )
{
    rectangleToolSettings = s;
}

// Set Ellipse tool settings
void KisDoc::setEllipseToolSettings( EllipseToolSettings s )
{
    ellipseToolSettings = s;
}

// Set Filler tool settings
void KisDoc::setFillerToolSettings( FillerToolSettings s )
{
    fillerToolSettings = s;
}

// Set Color changer settings
void KisDoc::setColorChangerSettings( ColorChangerSettings s )
{
    colorChangerSettings = s;
}

// Set Stamp (Pattern) tool settings
void KisDoc::setStampToolSettings( StampToolSettings s )
{
    stampToolSettings = s;
}

// Set Gradients settings
void KisDoc::setGradientsSettings( GradientsSettings s )
{
    gradientsSettings = s;
}

#include "kis_doc.moc"
