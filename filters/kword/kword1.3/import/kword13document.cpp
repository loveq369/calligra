//

/*
   This file is part of the KDE project
   Copyright (C) 2004 Nicolas GOUTTE <goutte@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <qiodevice.h>

#include <kdebug.h>

#include "kword13utils.h"
#include "kword13layout.h"
#include "kword13document.h"

KWord13Document::KWord13Document( void )
{
    m_normalTextFramesetList.setAutoDelete( true );
    m_tableFramesetList.setAutoDelete( true );
    m_otherFramesetList.setAutoDelete( true );
    m_headerFooterFramesetList.setAutoDelete( true );
    m_footEndNoteFramesetList.setAutoDelete( true );
}

KWord13Document::~KWord13Document( void )
{
}

void KWord13Document::xmldump( QIODevice* io )
{
    QTextStream iostream( io );
    iostream.setEncoding( QTextStream::UnicodeUTF8 );
    
    iostream << "<?xml encoding='UTF-8'?>\n";
    iostream << "<kworddocument>\n";
    
    for ( QMap<QString,QString>::ConstIterator it = m_documentProperties.begin();
        it != m_documentProperties.end();
        ++it)
    {
        iostream << " <param key=\"" << it.key() << "\" data=\"" << EscapeXmlDump( it.data() ) << "\"/>\n";
    }
    
    iostream << " <documentinfo>\n";
    for ( QMap<QString,QString>::ConstIterator it11 = m_documentInfo.begin();
        it11 != m_documentInfo.end();
        ++it11)
    {
        iostream << "  <param key=\"" << it11.key() << "\" data=\"" << EscapeXmlDump( it11.data() ) << "\"/>\n";
    }
    iostream << " </documentinfo>\n";
    
    iostream << " <normalframesets>\n";
    for ( KWordTextFrameset* item = m_normalTextFramesetList.first();
        item;
        item = m_normalTextFramesetList.next() )
    {
        item->xmldump( iostream );
    }
    iostream << " </normalframesets>\n";
    
    iostream << " <tableframesets>\n";
    for ( KWordTextFrameset* item12 = m_tableFramesetList.first();
        item12;
        item12 = m_tableFramesetList.next() )
    {
        item12->xmldump( iostream );
    }
    iostream << " </tableframesets>\n";
    
    iostream << " <headerfooterframesets>\n";
    for ( KWordTextFrameset* item2 = m_headerFooterFramesetList.first();
        item2;
        item2 = m_headerFooterFramesetList.next() )
    {
        item2->xmldump( iostream );
    }
    iostream << " </headerfooterframesets>\n";
    
    iostream << " <footendnoteframesets>\n";
    for ( KWordTextFrameset* item3 = m_footEndNoteFramesetList.first();
        item3;
        item3 = m_footEndNoteFramesetList.next() )
    {
        item3->xmldump( iostream );
    }
    iostream << " </footendnoteframesets>\n";
    
    iostream << " <otherframesets>\n";
    for ( KWord13Frameset* item4 = m_otherFramesetList.first();
        item4;
        item4 = m_otherFramesetList.next() )
    {
        item4->xmldump( iostream );
    }
    iostream << " </otherframesets>\n";
    
    iostream << " <styles>\n";
    
    for ( QValueList<KWord13Layout>::Iterator it2 = m_styles.begin();
        it2 != m_styles.end();
        ++it2)
    {
        (*it2).xmldump( iostream );
    }
    
    iostream << " </styles>\n";
    
    iostream << "</kworddocument>\n";
}

QString KWord13Document::getDocumentInfo( const QString& name ) const
{
    QMap<QString,QString>::ConstIterator it ( m_documentInfo.find( name ) );
    if ( it == m_documentInfo.end() )
    {
        // Property does not exist
        return QString::null;
    }
    else
    {
        return it.data();
    }
}

QString KWord13Document::getProperty( const QString& name, const QString& oldName ) const
{
    const QString result ( getPropertyInternal( name ) );
    
    if ( result.isEmpty() && !oldName.isEmpty() )
    {
        // The result is empty result and we have an old name, so try the old name
        return getPropertyInternal( oldName );
    }
    else
    {
        return result;
    }
}

QString KWord13Document::getPropertyInternal( const QString& name ) const
{
    QMap<QString,QString>::ConstIterator it ( m_documentProperties.find( name ) );
    if ( it == m_documentProperties.end() )
    {
        // Property does not exist
        return QString::null;
    }
    else
    {
        return it.data();
    }
}

QDateTime KWord13Document::lastPrintingDate( void ) const
{
    const QString strDate( getPropertyInternal( "VARIABLESETTINGS:lastPrintingDate" ) );
    
    QDateTime dt;
    
    if ( strDate.isEmpty() )
    {
        // The printing date only exists in syntax 3, so we have no fallback.
        kdDebug(30520) << "No syntax 3 printing date!" << endl;
    }
    else
    {
        dt = QDateTime::fromString( strDate, Qt::ISODate );
    }
    return dt;
}

QDateTime KWord13Document::creationDate( void ) const
{
    const QString strDate( getPropertyInternal( "VARIABLESETTINGS:creationDate" ) );
    
    QDateTime dt;
    
    if ( strDate.isEmpty() )
    {
        kdDebug(30520) << "No syntax 3 creation date!" << endl;
        const int year = getPropertyInternal( "VARIABLESETTINGS:createFileYear" ).toInt();
        const int month = getPropertyInternal( "VARIABLESETTINGS:createFileMonth" ).toInt();
        const int day = getPropertyInternal( "VARIABLESETTINGS:createFileDay" ).toInt();
        dt.setDate( QDate ( year, month, day) );
    }
    else
    {
        dt = QDateTime::fromString( strDate, Qt::ISODate );
    }
    return dt;
}

QDateTime KWord13Document::modificationDate( void ) const
{
    const QString strDate( getPropertyInternal( "VARIABLESETTINGS:modificationDate" ) );
    
    QDateTime dt;
    
    if ( strDate.isEmpty() )
    {
        kdDebug(30520) << "No syntax 3 modification date!" << endl;
        const int year = getPropertyInternal( "VARIABLESETTINGS:modifyFileYear" ).toInt();
        const int month = getPropertyInternal( "VARIABLESETTINGS:modifyFileMonth" ).toInt();
        const int day = getPropertyInternal( "VARIABLESETTINGS:modifyFileDay" ).toInt();
        dt.setDate( QDate ( year, month, day) );
    }
    else
    {
        dt = QDateTime::fromString( strDate, Qt::ISODate );
    }
    return dt;
}
