/* -*- C++ -*-

  $Id$
  
  This file is part of Kontour.
  Copyright (C) 2001 Igor Janssen (rm@linux.ru.net)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "GStyleList.h"

#include <klocale.h>

#include <qdict.h>
#include <qstringlist.h>
#include <qdom.h>

GStyleList::GStyleList()
{
  list.clear();
  GStyle *st = new GStyle();
  list.insert(i18n("defaul"), st);
  mCurStyle = st;
  mCur = 0;
}

GStyleList::GStyleList(const QDomElement &sl)
{
  list.clear();
  mCurStyle = 0L;
}

QDomElement GStyleList::writeToXml(QDomDocument &document)
{
  QDomElement sl = document.createElement("stylelist");
  QDictIterator<GStyle> it(list);
  for(;it.current(); ++it)
  {
    GStyle *st = it;
    QDomElement style = st->writeToXml(document);
    style.setAttribute("id", it.currentKey());
    sl.appendChild(style);
  }
  return sl;
}

void GStyleList::current(QString aName)
{
  mCurStyle = list.find(aName);
}

QStringList *GStyleList::stringList()
{
  QStringList *l = new QStringList();
  QDictIterator<GStyle> it(list);
  for(;it.current(); ++it)
    *l << it.currentKey();
  return l;
}

void GStyleList::addStyle()
{
  GStyle *st = new GStyle();
  list.insert(i18n("new"), st);
}

void GStyleList::deleteStyle()
{

}
