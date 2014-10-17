/* This file is part of the KDE project
 * Copyright 2014  Dan Leinir Turthra Jensen <admin@leinir.dk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "checkoutcreator.h"

#include <kstandarddirs.h>
#include <kdirselectdialog.h>
#include <QDir>
#include <QDebug>

class CheckoutCreator::Private
{
public:
    Private()
    {}
    QString gitExecutable;
};

CheckoutCreator::CheckoutCreator(QObject* parent)
    : QObject(parent)
    , d(new Private)
{
    d->gitExecutable = KStandardDirs::findExe("git");
}

CheckoutCreator::~CheckoutCreator()
{
}

QString CheckoutCreator::getDir() const
{
    KUrl url = KDirSelectDialog::selectDirectory();
    return url.toLocalFile();
}

bool CheckoutCreator::isGitDir(QString directory) const
{
    QDir dir(directory);
    if(dir.exists(".git/config"))
        return true;
    return false;
}

QString CheckoutCreator::gitExecutable() const
{
    return d->gitExecutable;
}

#include "checkoutcreator.moc"

