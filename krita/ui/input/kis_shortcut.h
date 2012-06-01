/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KISSHORTCUT_H
#define KISSHORTCUT_H

#include <Qt>
#include <QList>

class KisAbstractInputAction;
class KisShortcut
{

public:
    enum MatchLevel {
        NoMatch,
        PotentialMatch,
        CompleteMatch
    };

    KisShortcut();
    virtual ~KisShortcut();

//     bool containsKey(Qt::Key key) const;
//     bool containsButton(Qt::MouseButton button) const;
    int priority() const;
    KisAbstractInputAction* action() const;
    void setAction(KisAbstractInputAction *action);

    void setKeys(const QList<Qt::Key> &keys);
    void setButtons(const QList<Qt::MouseButton> &buttons);

    void keyPress(Qt::Key key);
    void keyRelease(Qt::Key key);
    void buttonPress(Qt::MouseButton button);
    void buttonRelease(Qt::MouseButton button);

    MatchLevel matchLevel();


private:
    class Private;
    Private * const d;
};

#endif // KISSHORTCUT_H
