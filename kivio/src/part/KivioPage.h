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

#ifndef KIVIOPAGE_H
#define KIVIOPAGE_H

#include "KivioAbstractPage.h"

class KivioMasterPage;
class KoPageLayout;

class KivioPage : public KivioAbstractPage
{
  public:
    KivioPage(KivioMasterPage* master, const QString& title);
    ~KivioPage();

    void setMasterPage(KivioMasterPage* master);
    KivioMasterPage* masterPage() const;

    virtual KoPageLayout pageLayout() const;

  private:
    KivioMasterPage* m_masterPage;
};

#endif
