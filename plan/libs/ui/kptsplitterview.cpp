/* This file is part of the KDE project
  Copyright (C) 2007. 2012 Dag Andersen <danders@get2net.dk>

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

#include "kptsplitterview.h"

#include "KoDocument.h"
#include <KoXmlReader.h>

#include <QVBoxLayout>
#include <QSplitter>
#include <QTabWidget>

#include "kptdebug.h"

namespace KPlato
{

SplitterView::SplitterView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_activeview( 0 )
{
    QVBoxLayout *b = new QVBoxLayout( this );
    b->setContentsMargins( 0, 0, 0, 0 );
    m_splitter = new QSplitter( this );
    m_splitter->setOrientation( Qt::Vertical );
    b->addWidget( m_splitter );
}
    
QTabWidget *SplitterView::addTabWidget(  )
{
    QTabWidget *w = new QTabWidget( m_splitter );
    m_splitter->addWidget( w );
    connect( w, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)) );
    return w;
}

void SplitterView::currentTabChanged( int )
{
    ViewBase *v = qobject_cast<ViewBase*>( qobject_cast<QTabWidget*>( sender() )->currentWidget() );
    if ( v && v != m_activeview ) {
        if ( m_activeview ) {
            m_activeview->setGuiActive( false );
        }
        v->setGuiActive( true );
    }
}

void SplitterView::addView( ViewBase *view )
{
    m_splitter->addWidget( view );
    connect( view, SIGNAL(guiActivated(ViewBase*,bool)), this, SLOT(slotGuiActivated(ViewBase*,bool)) );
    connect( view, SIGNAL(requestPopupMenu(QString,QPoint)), SIGNAL(requestPopupMenu(QString,QPoint)) );
    connect(view, SIGNAL(optionsModified()), SIGNAL(optionsModified()));
}

void SplitterView::addView( ViewBase *view, QTabWidget *tab, const QString &label )
{
    tab->addTab( view, label );
    connect( view, SIGNAL(guiActivated(ViewBase*,bool)), this, SLOT(slotGuiActivated(ViewBase*,bool)) );
    connect( view, SIGNAL(requestPopupMenu(QString,QPoint)), SIGNAL(requestPopupMenu(QString,QPoint)) );
    connect(view, SIGNAL(optionsModified()), SIGNAL(optionsModified()));
}

// reimp
void SplitterView::setGuiActive( bool active ) // virtual slot
{
    debugPlan<<active<<m_activeview;
    if ( m_activeview ) {
        m_activeview->setGuiActive( active );
    } else {
        emit guiActivated( this, active );
    }
}

void SplitterView::slotGuiActivated( ViewBase *v, bool active )
{
    debugPlan<<active<<m_activeview<<" -> "<<v;
    if ( active ) {
        if ( m_activeview ) {
            emit guiActivated( m_activeview, false );
        }
        m_activeview = v;
    } else {
        m_activeview = 0;
    }
    emit guiActivated( v, active );
}

ViewBase *SplitterView::findView( const QPoint &pos ) const
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *w = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( w && w->frameGeometry().contains( pos ) ) {
            debugPlan<<pos<<" in "<<w->frameGeometry();
            return w;
        }
        QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
        if (tw && tw->frameGeometry().contains( pos ) ) {
            w = dynamic_cast<ViewBase*>( tw->currentWidget() );
            if ( w ) {
                debugPlan<<pos<<" in "<<w->frameGeometry();
                return w;
            }
        }
    }
    return const_cast<SplitterView*>( this );
}

void SplitterView::setProject( Project *project )
{
    foreach ( ViewBase *v, findChildren<ViewBase*>() ) {
        v->setProject( project );
    }
    ViewBase::setProject( project );
}

void SplitterView::setScheduleManager( ScheduleManager *sm )
{
    foreach ( ViewBase *v, findChildren<ViewBase*>() ) {
        v->setScheduleManager( sm );
    }
    ViewBase::setScheduleManager( sm );
}

void SplitterView::draw()
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->draw();
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->widget( j ) );
                    if ( v ) {
                        v->draw();
                    }
                }
            }
        }
    }
}

void SplitterView::draw( Project &project )
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->draw( project );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->widget( j ) );
                    if ( v ) {
                        v->draw( project );
                    }
                }
            }
        }
    }
}


void SplitterView::updateReadWrite( bool mode )
{
    for ( int i = 0; i < m_splitter->count(); ++i ) {
        ViewBase *v = dynamic_cast<ViewBase*>( m_splitter->widget( i ) );
        if ( v ) {
            v->updateReadWrite( mode );
        } else {
            QTabWidget *tw = dynamic_cast<QTabWidget*>( m_splitter->widget( i ) );
            if (tw ) {
                for ( int j = 0; j < tw->count(); ++j ) {
                    v = dynamic_cast<ViewBase*>( tw->widget( j ) );
                    if ( v ) {
                        v->updateReadWrite( mode );
                    }
                }
            }
        }
    }
}

ViewBase *SplitterView::focusView() const
{
    QList<ViewBase*> lst = findChildren<ViewBase*>();
    debugPlan<<lst;
    foreach ( ViewBase *v, lst ) {
        if ( v->isActive() ) {
            debugPlan<<v;
            return v;
        }
    }
    return 0;
}

QStringList SplitterView::actionListNames() const
{
    QStringList lst = ViewActionLists::actionListNames();
    ViewBase *view = focusView();
    if ( view && view != this ) {
        lst << view->actionListNames();
    }
    return lst;
}

QList<QAction*> SplitterView::actionList( const QString &name ) const
{
    QList<QAction*> lst = ViewActionLists::actionList( name );
    if ( lst.isEmpty() ) {
        ViewBase *view = focusView();
        if ( view && view != this ) {
            lst = view->actionList( name );
        }
    }
    return lst;
}
    
QList<QAction*> SplitterView::contextActionList() const
{
    ViewBase *view = focusView();
    debugPlan<<this<<view;
    if ( view ) {
        return view->contextActionList();
    }
    return QList<QAction*>();
}

Node* SplitterView::currentNode() const
{
    ViewBase *view = focusView();
    if ( view ) {
        return view->currentNode();
    }
    return 0;
}
    
Resource* SplitterView::currentResource() const
{
    ViewBase *view = focusView();
    if ( view ) {
        return view->currentResource();
    }
    return 0;
}

ResourceGroup* SplitterView::currentResourceGroup() const
{
    ViewBase *view = focusView();
    if ( view ) {
        return view->currentResourceGroup();
    }
    return 0;
}

Calendar* SplitterView::currentCalendar() const
{
    ViewBase *view = focusView();
    if ( view ) {
        return view->currentCalendar();
    }
    return 0;
}

Relation *SplitterView::currentRelation() const
{
    ViewBase *view = focusView();
    if ( view ) {
        return view->currentRelation();
    }
    return 0;
}

bool SplitterView::loadContext( const KoXmlElement &context )
{
    KoXmlElement e = context.namedItem( "views" ).toElement();
    if ( e.isNull() ) {
        return true;
    }
#ifndef KOXML_USE_QDOM
    foreach ( const QString &s, e.attributeNames() ) {
        ViewBase *v = findChildren<ViewBase*>( s ).value( 0 );
        if ( v == 0 ) {
            continue;
        }
        KoXmlElement e1 = e.namedItem( s ).toElement();
        if ( e1.isNull() ) {
            continue;
        }
        v->loadContext( e1 );
    }
#endif
    return true;
}

void SplitterView::saveContext( QDomElement &context ) const
{
    QList<ViewBase*> lst = findChildren<ViewBase*>();
    if ( lst.isEmpty() ) {
        return;
    }
    QDomElement e = context.ownerDocument().createElement( "views" );
    context.appendChild( e );
    foreach ( ViewBase *v, lst ) {
        e.setAttribute( v->objectName(), "" );
    }
    foreach ( ViewBase *v, lst ) {
        QDomElement e1 = e.ownerDocument().createElement( v->objectName() );
        e.appendChild( e1 );
        v->saveContext( e1 );
    }
}

void SplitterView::slotEditCopy()
{
    ViewBase *v = focusView();
    if ( v ) {
        v->slotEditCopy();
    }
}

} // namespace KPlato
