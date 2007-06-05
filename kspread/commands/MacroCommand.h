/* This file is part of the KDE project
   Copyright 2005,2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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

#ifndef KSPREAD_MACRO_COMMAND
#define KSPREAD_MACRO_COMMAND

#include <QList>

#include "AbstractRegionCommand.h"

namespace KSpread
{

/**
 * The macro manipulator holds a set of manipulators and calls them all at once.
 * Each of the manipulators has its own range, MacroCommand does not take
 * care of that.
 */
class KSPREAD_EXPORT MacroCommand : public AbstractRegionCommand {
  public:
    void redo ();
    void undo ();
    void add (AbstractRegionCommand *manipulator);
  protected:
    QList<AbstractRegionCommand *> manipulators;
};

} // namespace KSpread

#endif // KSPREAD_MACRO_COMMAND
