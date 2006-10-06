/* This file is part of the KDE project
 * Copyright (C) 2006 Jan Hambrecht <jaham@gmx.net>
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

#include "vlayerdocker.h"

#include <QGridLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QAbstractItemModel>

#include <klocale.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kinputdialog.h>

#include <KoDocumentSectionView.h>

#include "vdocument.h"
#include "vlayer.h"

enum ButtonIds
{
    Button_New,
    Button_Raise,
    Button_Lower,
    Button_Delete
};

VLayerDocker::VLayerDocker( QWidget *parent, VDocument *doc )
: QWidget( parent )
, m_document( doc )
, m_model( 0 )
{
    setObjectName("Layers");

    QGridLayout* layout = new QGridLayout;
    layout->addWidget( m_layerView = new KoDocumentSectionView( this ), 0, 0, 1, 5 );

    QButtonGroup *buttonGroup = new QButtonGroup( this );
    buttonGroup->setExclusive( false );

    QToolButton *button = new QToolButton( this );
    button->setIcon( SmallIcon( "14_layer_newlayer" ) );
    button->setText( i18n( "New" ) );
    buttonGroup->addButton( button, Button_New );
    layout->addWidget( button, 1, 0 );

    button = new QToolButton( this );
    button->setIcon( SmallIcon( "14_layer_raiselayer" ) );
    button->setText( i18n( "Raise" ) );
    buttonGroup->addButton( button, Button_Raise );
    layout->addWidget( button, 1, 1 );

    button = new QToolButton( this );
    button->setIcon( SmallIcon( "14_layer_lowerlayer" ) );
    button->setText( i18n( "Lower" ) );
    buttonGroup->addButton( button, Button_Lower );
    layout->addWidget( button, 1, 2 );

    button = new QToolButton( this );
    button->setIcon( SmallIcon( "14_layer_deletelayer" ) );
    button->setText( i18n( "Delete" ) );
    buttonGroup->addButton( button, Button_Delete );
    layout->addWidget( button, 1, 3 );

    layout->setSpacing( 0 );
    layout->setMargin( 3 );

    connect( buttonGroup, SIGNAL( buttonClicked( int ) ), this, SLOT( slotButtonClicked( int ) ) );

    layout->activate();
    setLayout(layout);

    m_model = new VDocumentModel( m_document );
    //m_layerView->setRootIsDecorated( false );
    m_layerView->setItemsExpandable( true );
    m_layerView->setModel( m_model );
    m_layerView->setDisplayMode( KoDocumentSectionView::MinimalMode );
}

VLayerDocker::~VLayerDocker()
{
}

void VLayerDocker::updateView()
{
    m_model->update();
}

void VLayerDocker::slotButtonClicked( int buttonId )
{
    switch( buttonId )
    {
        case Button_New:
            addLayer();
            break;
        case Button_Raise:
            break;
        case Button_Lower:
            break;
        case Button_Delete:
            break;
    }
}

void VLayerDocker::addLayer()
{
    bool ok = true;
    QString name = KInputDialog::getText( i18n( "New Layer" ), i18n( "Enter the name of the new layer:" ),
                                          i18n( "New layer" ), &ok, this );
    if( ok )
    {
        KoLayerShape* layer = new KoLayerShape();
        /*TODO: porting to flake
        layer->setName( name );
        VLayerCmd* cmd = new VLayerCmd( m_document, i18n( "Add Layer" ),
                layer, VLayerCmd::addLayer );
        m_view->part()->addCommand( cmd, true );
        updateLayers();
        */
        m_document->insertLayer( layer );
        m_document->setObjectName( layer, name );
        m_model->update();
    }
}


VDocumentModel::VDocumentModel( VDocument *doc )
: m_document( doc )
{
}

void VDocumentModel::update()
{
    emit layoutChanged();
}

int VDocumentModel::rowCount( const QModelIndex &parent ) const
{
    // check if parent is root node
    if( ! parent.isValid() )
        return m_document->layers().count();

    Q_ASSERT(parent.model() == this);
    Q_ASSERT(parent.internalPointer());

    KoShapeContainer *parentShape = dynamic_cast<KoShapeContainer*>( (KoShape*)parent.internalPointer() );
    if( parentShape )
        return parentShape->childCount();
    else
        return 0;
}

int VDocumentModel::columnCount( const QModelIndex & ) const
{
    return 1;
}

QModelIndex VDocumentModel::index( int row, int column, const QModelIndex &parent ) const
{
    // check if parent is root node
    if( ! parent.isValid() )
    {
        if( row < m_document->layers().count() )
            return createIndex( row, column, m_document->layers().at(row) );
        else
            return QModelIndex();
    }

    Q_ASSERT(parent.model() == this);
    Q_ASSERT(parent.internalPointer());

    KoShapeContainer *parentShape = dynamic_cast<KoShapeContainer*>( (KoShape*)parent.internalPointer() );
    if( parentShape && row < parentShape->childCount() )
        return createIndex( row, column, parentShape->iterator().at(row) );
    else
        return QModelIndex();
}

QModelIndex VDocumentModel::parent( const QModelIndex &child ) const
{
    // check if child is root node
    if( ! child.isValid() )
        return QModelIndex();

    Q_ASSERT(child.model() == this);
    Q_ASSERT(child.internalPointer());

    KoShape *childShape = static_cast<KoShape*>( child.internalPointer() );
    // check if child shape is a layer, and return invalid model index if it is
    KoLayerShape *childlayer = dynamic_cast<KoLayerShape*>( childShape );
    if( childlayer )
        return QModelIndex();

    // get the childs parent shape
    KoShapeContainer *parentShape = childShape->parent();
    if( ! parentShape )
        return QModelIndex();

    // check if the parent is a layer
    KoLayerShape *parentLayer = dynamic_cast<KoLayerShape*>( parentShape );
    if( parentLayer )
        return createIndex( m_document->layers().indexOf( parentLayer ), 0, parentShape );

    KoShapeContainer *grandParentShape = parentShape->parent();
    if( ! parentShape->parent() )
        return QModelIndex();

    return createIndex( grandParentShape->iterator().indexOf( parentShape ), 0, parentShape );
}

QVariant VDocumentModel::data( const QModelIndex &index, int role ) const
{
    if( ! index.isValid() )
        return QVariant();

    Q_ASSERT(index.model() == this);
    Q_ASSERT(index.internalPointer());

    KoShape *shape = static_cast<KoShape*>( index.internalPointer() );

    switch (role)
    {
        case Qt::DisplayRole:
        {
            QString name = m_document->objectName( shape );
            if( name.isEmpty() )
            {
                if( dynamic_cast<KoLayerShape*>( shape ) )
                    name = i18n("Layer");
                else if( dynamic_cast<KoShapeContainer*>( shape ) )
                    name = i18n("Group");
                else
                    name = i18n("Shape");
            }
            return name;
        }
        case Qt::DecorationRole: return QVariant();//return shape->icon();
        case Qt::EditRole: return m_document->objectName( shape );
        case Qt::SizeHintRole: return shape->size();
        case ActiveRole:
        {
            KoLayerShape *layer = dynamic_cast<KoLayerShape*>( shape );
            if( layer )
                return (layer == m_document->activeLayer() );
            else
                return false;
        }
        case PropertiesRole: return QVariant::fromValue( properties( shape ) );
        case AspectRatioRole:
        {
            QRectF bbox = shape->boundingRect();
            return double(bbox.width()) / bbox.height();
        }
        default:
/*            if (role >= int(BeginThumbnailRole))
                return QImage( role - int(BeginThumbnailRole), role - int(BeginThumbnailRole), QImage::Format_ARGB32 );
            else*/
                return QVariant();
    }
}

Qt::ItemFlags VDocumentModel::flags(const QModelIndex &index) const
{
    if( ! index.isValid() )
        return Qt::ItemIsEnabled;

    Q_ASSERT(index.model() == this);
    Q_ASSERT(index.internalPointer());

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    if( dynamic_cast<KoShapeContainer*>( (KoShape*)index.internalPointer() ) )
        flags |= Qt::ItemIsDropEnabled;
    return flags;
}

bool VDocumentModel::setData(const QModelIndex &index, const QVariant &value, int role )
{
    if( ! index.isValid() )
        return false;

    Q_ASSERT(index.model() == this);
    Q_ASSERT(index.internalPointer());

    KoShape *shape = static_cast<KoShape*>( index.internalPointer() );
    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
            m_document->setObjectName( shape, value.toString() );
            break;
        case PropertiesRole:
            setProperties( shape, value.value<PropertyList>());
            break;
        case ActiveRole:
            if (value.toBool())
            {
                KoLayerShape *layer = dynamic_cast<KoLayerShape*>( shape );
                if( layer )
                    m_document->setActiveLayer( layer );
            }
            break;
        default:
            return false;
    }

    emit dataChanged( index, index );
    return true;
}

bool VDocumentModel::hasChildren( const QModelIndex & parent ) const
{
    if( ! parent.isValid() )
        return true;

    KoShapeContainer *container = dynamic_cast<KoShapeContainer*>( (KoShape*)parent.internalPointer());
    if( container && container->childCount() )
        return true;

    return false;
}

KoDocumentSectionModel::PropertyList VDocumentModel::properties( KoShape* shape ) const
{
    PropertyList l;
    l << Property(i18n("Visible"), SmallIcon("14_layer_visible"), SmallIcon("14_layer_novisible"), shape->isVisible());
    l << Property(i18n("Locked"), SmallIcon("locked"), SmallIcon("unlocked"), shape->isLocked());
    return l;
}

void VDocumentModel::setProperties( KoShape* shape, const PropertyList &properties )
{
    bool oldVisibleState = shape->isVisible();
    bool oldLockedState = shape->isLocked();

    shape->setVisible( properties.at( 0 ).state.toBool() );
    shape->setLocked( properties.at( 1 ).state.toBool() );

    if( ( oldVisibleState != shape->isVisible() ) || ( oldLockedState != shape->isLocked() ) )
        shape->repaint();
}

#include "vlayerdocker.moc"

// kate: replace-tabs on; space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
