/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __kimageshop_shell_h__
#define __kimageshop_shell_h__

#include <koMainWindow.h>

class KImageShopShell : public KoMainWindow
{
    Q_OBJECT
public:
    KImageShopShell( QWidget* parent = 0, const char* name = 0 );
    ~KImageShopShell();

    /**
     * Change these according to your native mimetype.
     */
    QString nativeFormatMimeType() const { return "application/x-kimageshop"; }
    QString nativeFormatPattern() const { return "*.kis"; }
    QString nativeFormatName() const { return "KImageShop"; }

public slots:
   virtual void slotFileNew();
   virtual void slotFileOpen();

protected:
    virtual QString configFile() const;
    virtual KoDocument* createDoc();
};

#endif
