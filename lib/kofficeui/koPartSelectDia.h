/* This file is part of the KDE libraries
    Copyright (C) 1998 Torben Weis <weis@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef DlgPartSelect_included
#define DlgPartSelect_included

#include <koQueryTypes.h>

#include "koPartSelectDia_data.h"

#include <qvaluelist.h>

/**
 *  This dialog presents the user all available
 *  KOffice components ( KSpread,KWord etc ) with name
 *  and mini icon. The user may select one and
 *  the corresponding KoDocumentEntry is returned.
 */
class KoPartSelectDia : public DlgPartSelectData
{

    Q_OBJECT

public:

    /**
     *  Constructor.
     */
    KoPartSelectDia( QWidget* _parent = NULL, const char* _name = NULL );

    /**
     *  Destructor.
     */
    virtual ~KoPartSelectDia();
  
    /**
     *  Retrieves the result of the part selection.
     *
     *  @return A document entry.
     */
    KoDocumentEntry result();

    /**
     *  Convenience function for using the dialog.
     *
     *  @returns the KoDocumentEntry of the selected KOffice components
     *           or an empty entry.
     */
    static KoDocumentEntry selectPart();

private:

    QValueList<KoDocumentEntry> m_lstEntries;
};

#endif // DlgPartSelect_included
