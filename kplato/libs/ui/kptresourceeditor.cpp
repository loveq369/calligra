/* This file is part of the KDE project
  Copyright (C) 2006 - 2007 Dag Andersen kplato@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation;
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#include "kptresourceeditor.h"

#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptitemviewsettup.h"

#include <KoDocument.h>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QList>
#include <QHeaderView>
#include <QObject>
#include <QTreeWidget>
#include <QStringList>
#include <QVBoxLayout>


#include <kaction.h>
#include <kicon.h>
#include <kglobal.h>
#include <klocale.h>
#include <kactioncollection.h>
#include <kxmlguifactory.h>

#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

#include <kdebug.h>

namespace KPlato
{

ResourceItemModel::ResourceItemModel( QObject *parent )
    : ItemModelBase( parent ),
    m_group( 0 ),
    m_resource( 0 )
{
}

ResourceItemModel::~ResourceItemModel()
{
}

void ResourceItemModel::slotResourceToBeInserted( const ResourceGroup *group, int row )
{
    //kDebug()<<group->name()<<","<<row;
    Q_ASSERT( m_group == 0 );
    m_group = const_cast<ResourceGroup*>(group);
    beginInsertRows( index( group ), row, row );
}

void ResourceItemModel::slotResourceInserted( const Resource *resource )
{
    //kDebug()<<resource->name();
    Q_ASSERT( resource->parentGroup() == m_group );
    endInsertRows();
    m_group = 0;
}

void ResourceItemModel::slotResourceToBeRemoved( const Resource *resource )
{
    //kDebug()<<resource->name();
    Q_ASSERT( m_resource == 0 );
    m_resource = const_cast<Resource*>(resource);
    int row = index( resource ).row();
    beginRemoveRows( index( resource->parentGroup() ), row, row );
}

void ResourceItemModel::slotResourceRemoved( const Resource *resource )
{
    //kDebug()<<resource->name();
    Q_ASSERT( resource == m_resource );
    endRemoveRows();
    m_resource = 0;
}

void ResourceItemModel::slotResourceGroupToBeInserted( const ResourceGroup *group, int row )
{
    //kDebug()<<group->name();
    Q_ASSERT( m_group == 0 );
    m_group = const_cast<ResourceGroup*>(group);
    beginInsertRows( QModelIndex(), row, row );
}

void ResourceItemModel::slotResourceGroupInserted( const ResourceGroup *group )
{
    //kDebug()<<group->name();
    Q_ASSERT( group == m_group );
    endInsertRows();
    m_group = 0;
}

void ResourceItemModel::slotResourceGroupToBeRemoved( const ResourceGroup *group )
{
    //kDebug()<<group->name();
    Q_ASSERT( m_group == 0 );
    m_group = const_cast<ResourceGroup*>(group);
    int row = index( group ).row();
    beginRemoveRows( QModelIndex(), row, row );
}

void ResourceItemModel::slotResourceGroupRemoved( const ResourceGroup *group )
{
    //kDebug()<<group->name();
    Q_ASSERT( group == m_group );
    endRemoveRows();
    m_group = 0;
}

void ResourceItemModel::setProject( Project *project )
{
    if ( m_project ) {
        disconnect( m_project, SIGNAL( resourceChanged( Resource* ) ), this, SLOT( slotResourceChanged( Resource* ) ) );
        disconnect( m_project, SIGNAL( resourceGroupChanged( ResourceGroup* ) ), this, SLOT( slotResourceGroupChanged( ResourceGroup* ) ) );
        
        disconnect( m_project, SIGNAL( resourceGroupToBeAdded( const ResourceGroup*, int ) ), this, SLOT( slotResourceGroupToBeInserted( const ResourceGroup*, int ) ) );
        
        disconnect( m_project, SIGNAL( resourceGroupToBeRemoved( const ResourceGroup* ) ), this, SLOT( slotResourceGroupToBeRemoved( const ResourceGroup* ) ) );
        
        disconnect( m_project, SIGNAL( resourceToBeAdded( const ResourceGroup*, int ) ), this, SLOT( slotResourceToBeInserted( const ResourceGroup*, int ) ) );
        
        disconnect( m_project, SIGNAL( resourceToBeRemoved( const Resource* ) ), this, SLOT( slotResourceToBeRemoved( const Resource* ) ) );
        
        disconnect( m_project, SIGNAL( resourceGroupAdded( const ResourceGroup* ) ), this, SLOT( slotResourceGroupInserted( const ResourceGroup* ) ) );
        
        disconnect( m_project, SIGNAL( resourceGroupRemoved( const ResourceGroup* ) ), this, SLOT( slotResourceGroupRemoved( const ResourceGroup* ) ) );
        
        disconnect( m_project, SIGNAL( resourceAdded( const Resource* ) ), this, SLOT( slotResourceInserted( const Resource* ) ) );
        
        disconnect( m_project, SIGNAL( resourceRemoved( const Resource* ) ), this, SLOT( slotResourceRemoved( const Resource* ) ) );
        
        disconnect( m_project, SIGNAL( defaultCalendarChanged( Calendar* ) ), this, SLOT( slotCalendarChanged( Calendar* ) ) );
    }
    m_project = project;
    if ( m_project ) {
        connect( m_project, SIGNAL( resourceChanged( Resource* ) ), this, SLOT( slotResourceChanged( Resource* ) ) );
        connect( m_project, SIGNAL( resourceGroupChanged( ResourceGroup* ) ), this, SLOT( slotResourceGroupChanged( ResourceGroup* ) ) );
        
        connect( m_project, SIGNAL( resourceGroupToBeAdded( const ResourceGroup*, int ) ), this, SLOT( slotResourceGroupToBeInserted( const ResourceGroup*, int ) ) );
        
        connect( m_project, SIGNAL( resourceGroupToBeRemoved( const ResourceGroup* ) ), this, SLOT( slotResourceGroupToBeRemoved( const ResourceGroup* ) ) );
        
        connect( m_project, SIGNAL( resourceToBeAdded( const ResourceGroup*, int ) ), this, SLOT( slotResourceToBeInserted( const ResourceGroup*, int ) ) );
        
        connect( m_project, SIGNAL( resourceToBeRemoved( const Resource* ) ), this, SLOT( slotResourceToBeRemoved( const Resource* ) ) );
        
        connect( m_project, SIGNAL( resourceGroupAdded( const ResourceGroup* ) ), this, SLOT( slotResourceGroupInserted( const ResourceGroup* ) ) );
        
        connect( m_project, SIGNAL( resourceGroupRemoved( const ResourceGroup* ) ), this, SLOT( slotResourceGroupRemoved( const ResourceGroup* ) ) );
        
        connect( m_project, SIGNAL( resourceAdded( const Resource* ) ), this, SLOT( slotResourceInserted( const Resource* ) ) );
        
        connect( m_project, SIGNAL( resourceRemoved( const Resource* ) ), this, SLOT( slotResourceRemoved( const Resource* ) ) );
    
        connect( m_project, SIGNAL( defaultCalendarChanged( Calendar* ) ), this, SLOT( slotCalendarChanged( Calendar* ) ) );
    }
}

Qt::ItemFlags ResourceItemModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags flags = ItemModelBase::flags( index );
    if ( !m_readWrite ) {
        return flags &= ~Qt::ItemIsEditable;
    }
    if ( !index.isValid() ) {
        return flags;
    }

    if ( !index.isValid() )
        return flags;
    if ( !m_readWrite ) {
        return flags &= ~Qt::ItemIsEditable;
    }
    if ( qobject_cast<Resource*>( object ( index ) ) ) {
        switch ( index.column() ) {
            default: flags |= Qt::ItemIsEditable;
        }
    } else if ( qobject_cast<ResourceGroup*>( object( index ) ) ) {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch ( index.column() ) {
            case 0: flags |= Qt::ItemIsEditable; break;
            case 1: flags |= Qt::ItemIsEditable; break;
            default: flags &= ~Qt::ItemIsEditable;
        }
    }
    return flags;
}


QModelIndex ResourceItemModel::parent( const QModelIndex &index ) const
{
    if ( !index.isValid() || m_project == 0 ) {
        return QModelIndex();
    }
    //kDebug()<<index.internalPointer()<<":"<<index.row()<<","<<index.column();

    Resource *r = qobject_cast<Resource*>( object( index ) );
    if ( r && r->parentGroup() ) {
        // only resources have parent
        int row = m_project->indexOf(  r->parentGroup() );
        return createIndex( row, 0, r->parentGroup() );
    }
    
    return QModelIndex();
}

bool ResourceItemModel::hasChildren( const QModelIndex &parent ) const
{
    //kDebug()<<parent.internalPointer()<<":"<<parent.row()<<","<<parent.column();
    if ( m_project == 0 ) {
        return false;
    }
    if ( ! parent.isValid() ) {
        return m_project->numResourceGroups() > 0;
    }
    ResourceGroup *g = qobject_cast<ResourceGroup*>( object( parent ) );
    if ( g ) {
        return g->numResources() > 0;
    }
    return false;
}

QModelIndex ResourceItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( m_project == 0 || column < 0 || column >= columnCount() || row < 0 ) {
        return QModelIndex();
    }
    if ( ! parent.isValid() ) {
        if ( row < m_project->numResourceGroups() ) {
            return createIndex( row, column, m_project->resourceGroupAt( row ) );
        }
        return QModelIndex();
    }
    QObject *p = object( parent );
    ResourceGroup *g = qobject_cast<ResourceGroup*>( p );
    if ( g ) {
        if ( row < g->numResources() ) {
            return createIndex( row, column, g->resourceAt( row ) );
        }
        return QModelIndex();
    }
    return QModelIndex();
}

QModelIndex ResourceItemModel::index( const Resource *resource ) const
{
    if ( m_project == 0 || resource == 0 ) {
        return QModelIndex();
    }
    Resource *r = const_cast<Resource*>(resource);
    int row = -1;
    ResourceGroup *par = r->parentGroup();
    if ( par ) {
        row = par->indexOf( r );
        return createIndex( row, 0, r );
    }
    return QModelIndex();
}

QModelIndex ResourceItemModel::index( const ResourceGroup *group ) const
{
    if ( m_project == 0 || group == 0 ) {
        return QModelIndex();
    }
    ResourceGroup *g = const_cast<ResourceGroup*>(group);
    int row = m_project->indexOf( g );
    return createIndex( row, 0, g );

}

int ResourceItemModel::columnCount( const QModelIndex &/*parent*/ ) const
{
    return 10;
}

int ResourceItemModel::rowCount( const QModelIndex &parent ) const
{
    if ( m_project == 0 ) {
        return 0;
    }
    if ( ! parent.isValid() ) {
        return m_project->numResourceGroups();
    }
    QObject *p = object( parent );
    ResourceGroup *g = qobject_cast<ResourceGroup*>( p );
    if ( g ) {
        return g->numResources();
    }
    return 0;
}

QVariant ResourceItemModel::name( const Resource *res, int role ) const
{
    //kDebug()<<res->name()<<","<<role;
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->name();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceItemModel::name( const  ResourceGroup *res, int role ) const
{
    //kDebug()<<res->name()<<","<<role;
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->name();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setName( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() == res->name() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceNameCmd( res, value.toString(), "Modify resource name" ) );
            return true;
    }
    return false;
}

bool ResourceItemModel::setName( ResourceGroup *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() == res->name() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceGroupNameCmd( res, value.toString(), "Modify resourcegroup name" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::type( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->typeToString( true );
        case Role::EnumList: 
            return res->typeToStringList( true );
        case Role::EnumListValue: 
            return (int)res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceItemModel::type( const ResourceGroup *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->typeToString( true );
        case Role::EnumList: 
            return res->typeToStringList( true );
        case Role::EnumListValue: 
            return (int)res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setType( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            Resource::Type v = static_cast<Resource::Type>( value.toInt() );
            if ( v == res->type() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceTypeCmd( res, v, "Modify resource type" ) );
            return true;
    }
    return false;
}

bool ResourceItemModel::setType( ResourceGroup *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            ResourceGroup::Type v = static_cast<ResourceGroup::Type>( value.toInt() );
            if ( v == res->type() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceGroupTypeCmd( res, v, "Modify resourcegroup type" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::initials( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->initials();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setInitials( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() == res->initials() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceInitialsCmd( res, value.toString(), "Modify resource initials" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::email( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->email();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setEmail( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() == res->email() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceEmailCmd( res, value.toString(), "Modify resource email" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::calendar( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole: {
            QString s = i18n( "None" );
            Calendar *cal = res->calendar( true ); // don't check for default calendar
            if ( cal == 0 ) {
                // Do we get a default calendar
                cal = res->calendar();
                if ( cal ) {
                    s = i18nc( "Default (calendar name)", "Default (%1)", cal->name() );
                }
            } else {
                s = cal->name();
            }
            return s;
        }
        case Role::EnumList: {
            Calendar *cal = m_project->defaultCalendar();
            QString s = i18n( "None" );
            if ( cal ) {
                s = i18nc( "Default (calendar name)", "Default (%1)", cal->name() );
            }
            return QStringList() << s << m_project->calendarNames();
        }
        case Role::EnumListValue: {
            Calendar *cal = res->calendar( true ); // don't check for default calendar
            return cal == 0 ? 0 : m_project->calendarNames().indexOf( cal->name() ) + 1;
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setCalendar( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
        {
            Calendar *c = 0;
            if ( value.toInt() > 0 ) {
                QStringList lst = calendar( res, Role::EnumList ).toStringList();
                if ( value.toInt() < lst.count() ) {
                    c = m_project->calendarByName( lst.at( value.toInt() ) );
                }
            }
            if ( c == res->calendar( true ) ) {
                return false;
            }
            emit executeCommand( new ModifyResourceCalendarCmd( res, c, "Modify resource calendar" ) );
            return true;
        }
    }
    return false;
}


QVariant ResourceItemModel::units( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return res->units();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setUnits( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toInt() == res->units() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceUnitsCmd( res, value.toInt(), "Modify resource available units" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::availableFrom( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( res->availableFrom().dateTime() );
        case Qt::EditRole:
            return res->availableFrom().dateTime();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDateTime( res->availableFrom().dateTime() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setAvailableFrom( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toDateTime() == res->availableFrom().dateTime() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceAvailableFromCmd( res, value.toDateTime(), "Modify resource available from" ) );
            return true;
    }
    return false;
}
    
QVariant ResourceItemModel::availableUntil( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( res->availableUntil().dateTime() );
        case Qt::EditRole:
            return res->availableUntil().dateTime();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDateTime( res->availableUntil().dateTime() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setAvailableUntil( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toDateTime() == res->availableUntil().dateTime() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceAvailableUntilCmd( res, value.toDateTime(), "Modify resource available until" ) );
            return true;
    }
    return false;
}

QVariant ResourceItemModel::normalRate( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatMoney( res->normalRate() );
        case Qt::EditRole:
            return res->normalRate();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setNormalRate( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toDouble() == res->normalRate() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceNormalRateCmd( res, value.toDouble(), "Modify resource normal rate" ) );
            return true;
    }
    return false;
}


QVariant ResourceItemModel::overtimeRate( const Resource *res, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatMoney( res->overtimeRate() );
        case Qt::EditRole:
            return res->overtimeRate();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceItemModel::setOvertimeRate( Resource *res, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toDouble() == res->overtimeRate() ) {
                return false;
            }
            emit executeCommand( new ModifyResourceOvertimeRateCmd( res, value.toDouble(), "Modify resource overtime rate" ) );
            return true;
    }
    return false;
}


QVariant ResourceItemModel::notUsed( const ResourceGroup *, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return QString(" ");
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::EditRole:
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceItemModel::data( const QModelIndex &index, int role ) const
{
    QVariant result;
    QObject *obj = object( index );
    if ( obj == 0 ) {
        return QVariant();
    }
    Resource *r = qobject_cast<Resource*>( obj );
    if ( r ) {
        switch ( index.column() ) {
            case 0: result = name( r, role ); break;
            case 1: result = type( r, role ); break;
            case 2: result = initials( r, role ); break;
            case 3: result = email( r, role ); break;
            case 4: result = calendar( r, role ); break;
            case 5: result = units( r, role ); break;
            case 6: result = availableFrom( r, role ); break;
            case 7: result = availableUntil( r, role ); break;
            case 8: result = normalRate( r, role ); break;
            case 9: result = overtimeRate( r, role ); break;
            default:
                kDebug()<<"data: invalid display value column"<<index.column();;
                return QVariant();
        }
        if ( result.isValid() ) {
            if ( role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
                // HACK to show focus in empty cells
                result = " ";
            }
            return result;
        }
    } else { 
        ResourceGroup *g = qobject_cast<ResourceGroup*>( obj );
        if ( g ) {
            switch ( index.column() ) {
                case 0: result = name( g, role ); break;
                case 1: result = type( g, role ); break;
                default:
                    if ( index.column() < columnCount() ) {
                        result = notUsed( g, role );
                    } else {
                        kDebug()<<"data: invalid display value column"<<index.column();;
                        return QVariant();
                    }
            }
            if ( result.isValid() ) {
                if ( role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
                // HACK to show focus in empty cells
                    result = " ";
                }
                return result;
            }
        }
    }
    // define default action

    return QVariant();
}

bool ResourceItemModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if ( ( flags( index ) &Qt::ItemIsEditable ) == 0 || role != Qt::EditRole ) {
        return false;
    }
    QObject *obj = object( index );
    Resource *r = qobject_cast<Resource*>( obj );
    if ( r ) {
        switch (index.column()) {
            case 0: return setName( r, value, role );
            case 1: return setType( r, value, role );
            case 2: return setInitials( r, value, role );
            case 3: return setEmail( r, value, role );
            case 4: return setCalendar( r, value, role );
            case 5: return setUnits( r, value, role );
            case 6: return setAvailableFrom( r, value, role );
            case 7: return setAvailableUntil( r, value, role );
            case 8: return setNormalRate( r, value, role );
            case 9: return setOvertimeRate( r, value, role );
            default:
                qWarning("data: invalid display value column %d", index.column());
                return false;
        }
    } else { 
        ResourceGroup *g = qobject_cast<ResourceGroup*>( obj );
        if ( g ) {
            switch (index.column()) {
                case 0: return setName( g, value, role );
                case 1: return setType( g, value, role );
                default:
                    qWarning("data: invalid display value column %d", index.column());
                    return false;
            }
        }
    }
    return false;
}

QVariant ResourceItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        if ( role == Qt::DisplayRole ) {
            switch ( section ) {
                case 0: return i18n( "Name" );
                case 1: return i18n( "Type" );
                case 2: return i18n( "Initials" );
                case 3: return i18n( "Email" );
                case 4: return i18n( "Calendar" );
                case 5: return i18n( "Limit (%)" );
                case 6: return i18n( "Available From" );
                case 7: return i18n( "Available Until" );
                case 8: return i18n( "Normal Rate" );
                case 9: return i18n( "Overtime Rate" );
                default: return QVariant();
            }
        } else if ( role == Qt::TextAlignmentRole ) {
            switch (section) {
                case 1: return Qt::AlignCenter;
                case 2: return Qt::AlignCenter;
                case 3: return Qt::AlignCenter;
                case 4: return Qt::AlignCenter;
                case 5: return Qt::AlignCenter;
                case 6: return Qt::AlignCenter;
                case 7: return Qt::AlignCenter;
                case 8: return Qt::AlignCenter;
                case 9: return Qt::AlignCenter;
                default: return QVariant();
            }
        }
    }
    if ( role == Qt::ToolTipRole ) {
        switch ( section ) {
            case 0: return ToolTip::ResourceName;
            case 1: return ToolTip::ResourceType;
            case 2: return ToolTip::ResourceInitials;
            case 3: return ToolTip::ResourceEMail;
            case 4: return ToolTip::ResourceCalendar;
            case 5: return ToolTip::ResourceUnits;
            case 6: return ToolTip::ResourceAvailableFrom;
            case 7: return ToolTip::ResourceAvailableUntil;
            case 8: return ToolTip::ResourceNormalRate;
            case 9: return ToolTip::ResourceOvertimeRate;
            default: return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QObject *ResourceItemModel::object( const QModelIndex &index ) const
{
    QObject *o = 0;
    if ( index.isValid() ) {
        o = static_cast<QObject*>( index.internalPointer() );
        Q_ASSERT( o );
    }
    return o;
}

void ResourceItemModel::slotCalendarChanged( Calendar* )
{
    foreach ( Resource *r, m_project->resourceList() ) {
        if ( r->calendar( true ) == 0 ) {
            slotResourceChanged( r );
        }
    }
}

void ResourceItemModel::slotResourceChanged( Resource *res )
{
    ResourceGroup *g = res->parentGroup();
    if ( g ) {
        int row = g->indexOf( res );
        emit dataChanged( createIndex( row, 0, res ), createIndex( row, columnCount() - 1, res ) );
        return;
    }
}

void ResourceItemModel::slotResourceGroupChanged( ResourceGroup *res )
{
    Project *p = res->project();
    if ( p ) {
        int row = p->resourceGroups().indexOf( res );
        emit dataChanged( createIndex( row, 0, res ), createIndex( row, columnCount() - 1, res ) );
    }
}

Qt::DropActions ResourceItemModel::supportedDropActions() const
{
    return Qt::CopyAction;
}


QStringList ResourceItemModel::mimeTypes() const
{
    return QStringList() << "text/x-vcard";
}

bool ResourceItemModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
    kDebug()<<row<<" p:"<<parent.row();
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0) {
        return false;
    }
    //kDebug()<<row<<" p:"<<parent.row();
    ResourceGroup *g = 0;
    if ( parent.isValid() ) {
        g = qobject_cast<ResourceGroup*>( object( parent ) );
    } else {
        g = qobject_cast<ResourceGroup*>( object( index( row, column, parent ) ) );
    }
    if ( g == 0 ) {
        return false;
    }
    //kDebug()<<data->formats();
    MacroCommand *m = 0;
    if ( data->hasFormat( "text/x-vcard" ) ) {
        QByteArray vcard = data->data( "text/x-vcard" );
        KABC::VCardConverter vc;
        KABC::Addressee::List lst = vc.parseVCards( vcard );
        foreach( KABC::Addressee a, lst ) {
            if ( m == 0 ) m = new MacroCommand( i18np( "Add resource from addressbook", "Add %1 resources from addressbook", lst.count() ) );
            Resource *r = new Resource();
            r->setName( a.formattedName() );
            r->setEmail( a.preferredEmail() );
            m->addCommand( new AddResourceCmd( g, r ) );
        }
    }
    if ( m ) {
        emit executeCommand( m );
    }
    return true;
}

QModelIndex ResourceItemModel::insertGroup( ResourceGroup *g )
{
    //kDebug();
    emit executeCommand( new AddResourceGroupCmd( m_project, g, i18n( "Add resource group" ) ) );
    int row = m_project->resourceGroups().indexOf( g );
    if ( row != -1 ) {
        return createIndex( row, 0, g );
    }
    return QModelIndex();
}

QModelIndex ResourceItemModel::insertResource( ResourceGroup *g, Resource *r, Resource */*after*/ )
{
    //kDebug();
    emit executeCommand( new AddResourceCmd( g, r, i18n( "Add resource" ) ) );
    int row = g->indexOf( r );
    if ( row != -1 ) {
        return createIndex( row, 0, r );
    }
    return QModelIndex();
}


//--------------------
ResourceTreeView::ResourceTreeView( QWidget *parent )
    : DoubleTreeViewBase( parent )
{
//    header()->setContextMenuPolicy( Qt::CustomContextMenu );
    setStretchLastSection( false );
    
    setModel( new ResourceItemModel() );
    
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setItemDelegateForColumn( 1, new EnumDelegate( this ) );
    setItemDelegateForColumn( 4, new EnumDelegate( this ) );

    setAcceptDrops( true );
    setDropIndicatorShown( true );
    
    QList<int> lst1; lst1 << 1 << -1;
    QList<int> lst2; lst2 << 0;
    hideColumns( lst1, lst2 );
}

void ResourceTreeView::slotActivated( const QModelIndex index )
{
    kDebug()<<index.column();
}

QObject *ResourceTreeView::currentObject() const
{
    return model()->object( selectionModel()->currentIndex() );
}

QList<QObject*> ResourceTreeView::selectedObjects() const
{
    QList<QObject*> lst;
    foreach (QModelIndex i, selectionModel()->selectedRows() ) {
        lst << static_cast<QObject*>( i.internalPointer() );
    }
    return lst;
}

QList<ResourceGroup*> ResourceTreeView::selectedGroups() const
{
    QList<ResourceGroup*> gl;
    foreach ( QObject *o, selectedObjects() ) {
        ResourceGroup* g = qobject_cast<ResourceGroup*>( o );
        if ( g ) {
            gl << g;
        }
    }
    return gl;
}

QList<Resource*> ResourceTreeView::selectedResources() const
{
    QList<Resource*> rl;
    foreach ( QObject *o, selectedObjects() ) {
        Resource* r = qobject_cast<Resource*>( o );
        if ( r ) {
            rl << r;
        }
    }
    return rl;
}

//-----------------------------------
ResourceEditor::ResourceEditor( KoDocument *part, QWidget *parent )
    : ViewBase( part, parent )
{
    setupGui();
    
    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new ResourceTreeView( this );
    l->addWidget( m_view );
    
    m_view->setEditTriggers( m_view->editTriggers() | QAbstractItemView::EditKeyPressed );

    connect( model(), SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );

    connect( m_view, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( slotCurrentChanged( const QModelIndex & ) ) );

    connect( m_view, SIGNAL( selectionChanged( const QModelIndexList ) ), this, SLOT( slotSelectionChanged( const QModelIndexList ) ) );

    connect( m_view, SIGNAL( contextMenuRequested( QModelIndex, const QPoint& ) ), this, SLOT( slotContextMenuRequested( QModelIndex, const QPoint& ) ) );
    
    connect( m_view, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );

}

void ResourceEditor::updateReadWrite( bool readwrite )
{
    m_view->setReadWrite( readwrite );
}

void ResourceEditor::draw( Project &project )
{
    m_view->setProject( &project );
}

void ResourceEditor::draw()
{
}

void ResourceEditor::setGuiActive( bool activate )
{
    kDebug()<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate && !m_view->selectionModel()->currentIndex().isValid() ) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
    }
}

void ResourceEditor::slotContextMenuRequested( QModelIndex index, const QPoint& pos )
{
    //kDebug()<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if ( index.isValid() ) {
        QObject *obj = m_view->model()->object( index );
        ResourceGroup *g = qobject_cast<ResourceGroup*>( obj );
        if ( g ) {
            name = "resourceeditor_group_popup";
        } else {
            Resource *r = qobject_cast<Resource*>( obj );
            if ( r ) {
                name = "resourceeditor_resource_popup";
            }
        }
    }
    emit requestPopupMenu( name, pos );
}

void ResourceEditor::slotHeaderContextMenuRequested( const QPoint &pos )
{
    kDebug();
    QList<QAction*> lst = contextActionList();
    if ( ! lst.isEmpty() ) {
        QMenu::exec( lst, pos,  lst.first() );
    }
}

Resource *ResourceEditor::currentResource() const
{
    return qobject_cast<Resource*>( m_view->currentObject() );
}

ResourceGroup *ResourceEditor::currentResourceGroup() const
{
    return qobject_cast<ResourceGroup*>( m_view->currentObject() );
}

void ResourceEditor::slotCurrentChanged(  const QModelIndex & )
{
    //kDebug()<<curr.row()<<","<<curr.column();
//    slotEnableActions();
}

void ResourceEditor::slotSelectionChanged( const QModelIndexList )
{
    //kDebug()<<list.count();
    updateActionsEnabled();
}

void ResourceEditor::slotEnableActions( bool on )
{
    updateActionsEnabled( on );
}

void ResourceEditor::updateActionsEnabled(  bool on )
{
    bool o = on && m_view->project();

    QList<ResourceGroup*> groupList = m_view->selectedGroups();
    bool nogroup = groupList.isEmpty();
    bool group = groupList.count() == 1;
    QList<Resource*> resourceList = m_view->selectedResources();
    bool noresource = resourceList.isEmpty(); 
    bool resource = resourceList.count() == 1;
    
    bool any = !nogroup || !noresource;
    
    actionAddResource->setEnabled( o && ( (group  && noresource) || (resource && nogroup) ) );
    actionAddGroup->setEnabled( o );
    actionDeleteSelection->setEnabled( o && any );
}

void ResourceEditor::setupGui()
{
    QString name = "resourceeditor_edit_list";
    actionAddResource  = new KAction(KIcon( "document-new" ), i18n("Add Resource..."), this);
    actionCollection()->addAction("add_resource", actionAddResource );
    actionAddResource->setShortcut( KShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_I ) );
    connect( actionAddResource, SIGNAL( triggered( bool ) ), SLOT( slotAddResource() ) );
    addAction( name, actionAddResource );
    
    actionAddGroup  = new KAction(KIcon( "document-new" ), i18n("Add Resource Group..."), this);
    actionCollection()->addAction("add_group", actionAddGroup );
    actionAddGroup->setShortcut( KShortcut( Qt::CTRL + Qt::Key_I ) );
    connect( actionAddGroup, SIGNAL( triggered( bool ) ), SLOT( slotAddGroup() ) );
    addAction( name, actionAddGroup );
    
    actionDeleteSelection  = new KAction(KIcon( "edit-delete" ), i18n("Delete Selected Items"), this);
    actionCollection()->addAction("delete_selection", actionDeleteSelection );
    actionDeleteSelection->setShortcut( KShortcut( Qt::Key_Delete ) );
    connect( actionDeleteSelection, SIGNAL( triggered( bool ) ), SLOT( slotDeleteSelection() ) );
    addAction( name, actionDeleteSelection );
    
    // Add the context menu actions for the view options
    actionOptions = new KAction(KIcon("configure"), i18n("Configure..."), this);
    connect(actionOptions, SIGNAL(triggered(bool) ), SLOT(slotOptions()));
    addContextAction( actionOptions );
}

void ResourceEditor::slotOptions()
{
    kDebug();
    ItemViewSettupDialog dlg( m_view->slaveView() );
    dlg.exec();
}


void ResourceEditor::slotAddResource()
{
    //kDebug();
    QList<ResourceGroup*> gl = m_view->selectedGroups();
    if ( gl.count() > 1 ) {
        return;
    }
    ResourceGroup *g = 0;
    if ( !gl.isEmpty() ) {
        g = gl.first();
    } else {
        QList<Resource*> rl = m_view->selectedResources();
        if ( rl.count() != 1 ) {
            return;
        }
        g = rl.first()->parentGroup();
    }
    if ( g == 0 ) {
        return;
    }
    Resource *r = new Resource();
    QModelIndex i = m_view->model()->insertResource( g, r );
    if ( i.isValid() ) {
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->edit( i );
    }

}

void ResourceEditor::slotAddGroup()
{
    //kDebug();
    ResourceGroup *g = new ResourceGroup();
    QModelIndex i = m_view->model()->insertGroup( g );
    if ( i.isValid() ) {
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->edit( i );
    }
}

void ResourceEditor::slotDeleteSelection()
{
    QObjectList lst = m_view->selectedObjects();
    //kDebug()<<lst.count()<<" objects";
    if ( ! lst.isEmpty() ) {
        emit deleteObjectList( lst );
    }
}

bool ResourceEditor::loadContext( const KoXmlElement &context )
{
    kDebug()<<endl;
    return m_view->loadContext( context );
}

void ResourceEditor::saveContext( QDomElement &context ) const
{
    m_view->saveContext( context );
}


} // namespace KPlato

#include "kptresourceeditor.moc"
