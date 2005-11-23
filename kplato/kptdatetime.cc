/* This file is part of the KDE project
   Copyright (C) 2003 Dag Andersen <danders@get2net.dk>

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
 * Boston, MA 02110-1301, USA.
*/

#include "kptdatetime.h"

#include <kdebug.h>

namespace KPlato
{

DateTime::DateTime() : QDateTime() {
}

DateTime::DateTime(const QDateTime &dt) : QDateTime(dt.date(), dt.time()) {
}

DateTime::DateTime(const QDate &date, const QTime &time) : QDateTime(date, time) {
}

void DateTime::add(const Duration &duration) {
    *this = addSecs(duration.seconds());
    //kdDebug()<<k_funcinfo<<"days,secs: "<<days<<","<<secs<<" gives: "<<toString()<<endl;
}

void DateTime::subtract(const Duration &duration) {
    *this = addSecs(-duration.seconds());
    //kdDebug()<<k_funcinfo<<"days,secs: "<<days<<","<<secs<<" gives: "<<toString()<<endl;
}

Duration DateTime::duration(const DateTime &dt) const {
    Duration dur;
    dur.addDays(QABS(dt.daysTo(*this)));
    dur.addSeconds(QABS(dt.time().secsTo(time())));
    return dur;
}

DateTime DateTime::operator+(const Duration &duration) const {
    DateTime d(*this);
    d.add(duration);
    return d;
}

DateTime& DateTime::operator+=(const Duration &duration) {
    add(duration);
    return *this;
}

DateTime DateTime::operator-(const Duration &duration) const {
    DateTime d(*this);
    d.subtract(duration);
    return d;
}

DateTime& DateTime::operator-=(const Duration &duration) {
    subtract(duration);
    return *this;
}

}  //KPlato namespace
