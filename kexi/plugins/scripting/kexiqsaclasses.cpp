/* This file is part of the KDE project
Copyright (C) 2003   Lucijan Busch <lucijan@gmx.at>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include <qmessagebox.h>

#include "kexiqsaclasses.h"

KexiQSAClasses::KexiQSAClasses(QSInterpreter *i)
:QSObjectFactory(i)
{
}

QObject*
KexiQSAClasses::create(const QString &className, const QValueList<QSArgument> &arguments, QObject *context)
{
/*	if(className == "MessageBox")
	{
		QString message;
		QString caption;
		int lap = 0;
		for(QValueList<QSArgument>::ConstIterator it = arguments.begin(); it != arguments.end(); ++it)
		{
			if(lap == 0)
				caption = (*it).variant().toString();
			else
				message = (*it).variant().toString();
		}

		return new QMessageBox(caption, message, QMessageBox::NoIcon, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
	}*/
}

QStringList
KexiQSAClasses::classes() const
{
	QStringList c;
//	c.append("MessageBox");
	return c;
}

KexiQSAClasses::~KexiQSAClasses()
{
}
