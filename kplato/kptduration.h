/* This file is part of the KDE project
   Copyright (C) 2001 Thomas Zander zander@kde.org

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

#ifndef kptduration_h
#define kptduration_h

#include <qglobal.h>
#include <qstring.h>

namespace KPlato
{

/**
 * The duration class can be used to store a timespan in a convenient format.
 * The timespan can be in length in many many hours down to miliseconds.
 */
class KPTDuration {
    public:
        /**
         * DayTime  = d hh:mm:ss.sss
         * Day      = d.ddd
         * Hour     = hh:mm
         * HourFraction = h.fraction of an hour
         */
        enum Format { Format_DayTime, Format_Day, Format_Hour, Format_HourFraction };

        KPTDuration();
        KPTDuration(const KPTDuration &d);
        KPTDuration(unsigned d, unsigned h, unsigned m, unsigned s=0, unsigned ms=0);
        KPTDuration(Q_INT64 seconds);
        ~KPTDuration();

        /**
         * Adds @param delta to *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void addMilliseconds(Q_INT64 delta)  { add(delta); }

        /**
         * Adds @param delta to *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void addSeconds(Q_INT64 delta) { addMilliseconds(delta * 1000); }

        /**
         * Adds @param delta to *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void addMinutes(Q_INT64 delta) { addSeconds(delta * 60); }

        /**
         * Adds @param delta to *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void addHours(Q_INT64 delta) { addMinutes(delta * 60); }

        /**
         * Adds @param delta to *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void addDays(Q_INT64 delta) { addHours(delta * 24); }

        //FIXME: overflow problem
        Q_INT64 milliseconds() const { return m_ms; }
        unsigned seconds() const { return m_ms / 1000; }
        unsigned minutes() const { return seconds() / 60; }
        unsigned hours() const { return minutes() / 60; }
        unsigned days() const { return hours() / 24; }
        void get(unsigned *days, unsigned *hours, unsigned *minutes, unsigned *seconds=0, unsigned *milliseconds=0) const;

        bool isCloseTo(const KPTDuration &d) const;

        bool   operator==( const KPTDuration &d ) const { return m_ms == d.m_ms; }
        bool   operator!=( const KPTDuration &d ) const { return m_ms != d.m_ms; }
        bool   operator<( const KPTDuration &d ) const { return m_ms < d.m_ms; }
        bool   operator<=( const KPTDuration &d ) const { return m_ms <= d.m_ms; }
        bool   operator>( const KPTDuration &d ) const { return m_ms > d.m_ms; }
        bool   operator>=( const KPTDuration &d ) const { return m_ms >= d.m_ms; }
        KPTDuration &operator=(const KPTDuration &d ) { m_ms = d.m_ms; return *this;}
        KPTDuration operator*(int unit) const; 
        KPTDuration operator/(int unit) const;
        
        KPTDuration operator+(KPTDuration &d) const
            {KPTDuration dur; dur.add(d); return dur; }
        KPTDuration &operator+=(const KPTDuration &d) {add(d); return *this; }
        
        KPTDuration operator-(const KPTDuration &d) const
            {KPTDuration dur; dur.subtract(d); return dur; }
        KPTDuration &operator-=(const KPTDuration &d) {subtract(d); return *this; }

        QString toString(Format format = Format_DayTime) const;
        static KPTDuration fromString(const QString &s, Format format = Format_DayTime);

        /**
         * This is useful for occasions where we need a zero duration.
         */
        static const KPTDuration zeroDuration;

    private:
	/**
	 * Duration in milliseconds. Signed to allow for simple calculations which
	 * might go negative for intermediate results.
	 */
        Q_INT64 m_ms;

        void add(Q_INT64 delta);
        void add(const KPTDuration &delta);

        /**
         * Subtracts @param delta from *this. If @param delta > *this, *this is set to zeroDuration.
         */
        void subtract(const KPTDuration &delta);
};

}  //KPlato namespace

#endif
