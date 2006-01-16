/* This file is part of the KDE project
   Copyright (C) 2003 - 2005 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation;
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kptcalendar.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptproject.h"

#include <qdom.h>
#include <qptrlist.h>

#include <klocale.h>
#include <kdebug.h>

namespace KPlato
{

/////   CalendarDay   ////
CalendarDay::CalendarDay()
    : m_date(),
      m_state(0),
      m_workingIntervals() {

    //kdDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
    m_workingIntervals.setAutoDelete(true);
}

CalendarDay::CalendarDay(int state)
    : m_date(),
      m_state(state),
      m_workingIntervals() {

    //kdDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
    m_workingIntervals.setAutoDelete(true);
}

CalendarDay::CalendarDay(QDate date, int state)
    : m_date(date),
      m_state(state),
      m_workingIntervals() {

    //kdDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
    m_workingIntervals.setAutoDelete(true);
}

CalendarDay::CalendarDay(CalendarDay *day)
    : m_workingIntervals() {

    //kdDebug()<<k_funcinfo<<"("<<this<<") from ("<<day<<")"<<endl;
    m_workingIntervals.setAutoDelete(true);
    copy(*day);
}

CalendarDay::~CalendarDay() {
    //kdDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
}

const CalendarDay &CalendarDay::copy(const CalendarDay &day) {
    //kdDebug()<<k_funcinfo<<"("<<&day<<") date="<<day.date().toString()<<endl;
    m_date = day.date();
    m_state = day.state();
    m_workingIntervals.clear();
    QPtrListIterator<QPair<QTime, QTime> > it = day.workingIntervals();
    for(; it.current(); ++it) {
        m_workingIntervals.append(new QPair<QTime, QTime>(it.current()->first, it.current()->second));
    }
    return *this;
}

bool CalendarDay::load(QDomElement &element) {
    //kdDebug()<<k_funcinfo<<endl;
    bool ok=false;
    m_state = QString(element.attribute("state", "-1")).toInt(&ok);
    if (m_state < 0)
        return false;
    //kdDebug()<<k_funcinfo<<" state="<<m_state<<endl;
    if (element.hasAttribute("date"))
        m_date = QDate::fromString(element.attribute("date"));
        
    clearIntervals();
    QDomNodeList list = element.childNodes();
    for (unsigned int i=0; i<list.count(); ++i) {
        if (list.item(i).isElement()) {
            QDomElement e = list.item(i).toElement();
            if (e.tagName() == "interval") {
                //kdDebug()<<k_funcinfo<<"Interval start="<<e.attribute("start")<<" end="<<e.attribute("end")<<endl;
                QTime start = QTime::fromString(e.attribute("start"));
                QTime end = QTime::fromString(e.attribute("end"));
                addInterval(new QPair<QTime, QTime>(start,end));
            }
        }
    }
    return true;
}

void CalendarDay::save(QDomElement &element) const {
    //kdDebug()<<k_funcinfo<<m_date.toString()<<endl;
    if (m_state == Map::None)
        return;
    if (m_date.isValid()) {
        element.setAttribute("date", m_date.toString());
    }
    element.setAttribute("state", m_state);
    if (m_workingIntervals.count() == 0)
        return;
    
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        QDomElement me = element.ownerDocument().createElement("interval");
        element.appendChild(me);
        me.setAttribute("end", it.current()->second.toString());
        me.setAttribute("start", it.current()->first.toString());
    }
} 

void CalendarDay::addInterval(QPair<QTime, QTime> *interval) {
    m_workingIntervals.append(interval);
}

QTime CalendarDay::startOfDay() const {
    QTime t;
    if (!m_workingIntervals.isEmpty()) {
        QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
        t = it.current()->first;
        for (++it; it.current(); ++it) {
            if (t > it.current()->first)
                t = it.current()->first;
        }
    }
    return t;
}

QTime CalendarDay::endOfDay() const {
    QTime t;
    if (!m_workingIntervals.isEmpty()) {
        QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
        t = it.current()->second;
        for (++it; it.current(); ++it) {
            if (t > it.current()->second)
                t = it.current()->second;
        }
    }
    return t;
}
    
bool CalendarDay::operator==(const CalendarDay *day) const {
    return operator==(*day);
}
bool CalendarDay::operator==(const CalendarDay &day) const {
    //kdDebug()<<k_funcinfo<<endl;
    if (m_date.isValid() && day.date().isValid()) {
        if (m_date != day.date()) {
            //kdDebug()<<k_funcinfo<<m_date.toString()<<" != "<<day.date().toString()<<endl;
            return false;
        }
    } else if (m_date.isValid() != day.date().isValid()) {
        //kdDebug()<<k_funcinfo<<"one of the dates is not valid"<<endl;
        return false;
    }
    if (m_state != day.state()) {
        //kdDebug()<<k_funcinfo<<m_state<<" != "<<day.state()<<endl;
        return false;
    }
    if (m_workingIntervals.count() != day.workingIntervals().count()) {
        //kdDebug()<<k_funcinfo<<m_workingIntervals.count()<<" != "<<day.workingIntervals().count()<<endl;
        return false;
    }
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    QPtrListIterator<QPair<QTime, QTime> > dit = day.workingIntervals();
    for (; it.current(); ++it) {
        bool res = false;
        QPair<QTime, QTime> *a = it.current();
        for (dit.toFirst(); dit.current(); ++dit) {
            QPair<QTime, QTime> *b = dit.current();
            if (a->first == b->first && a->second == b->second) {
                res = true;
                break;
            }
        }
        if (res == false) {
            //kdDebug()<<k_funcinfo<<"interval mismatch "<<a->first.toString()<<"-"<<a->second.toString()<<endl;
            return false;
        }
    }
    return true;
}
bool CalendarDay::operator!=(const CalendarDay *day) const {
    return operator!=(*day);
}
bool CalendarDay::operator!=(const CalendarDay &day) const {
    return !operator==(day);
}

Duration CalendarDay::effort(const QTime &start, const QTime &end) {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    Duration eff;
    if (m_state != Map::Working) {
        //kdDebug()<<k_funcinfo<<"Non working day"<<endl;
        return eff;
    }
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        //kdDebug()<<k_funcinfo<<"Interval: "<<it.current()->first.toString()<<" - "<<it.current()->second.toString()<<endl;
        if (end > it.current()->first && start < it.current()->second) {
            DateTime dtStart(QDate::currentDate(), start);
            if (start < it.current()->first) {
                dtStart.setTime(it.current()->first);
            }
            DateTime dtEnd(QDate::currentDate(), end);
            if (end > it.current()->second) {
                dtEnd.setTime(it.current()->second);
            }
            eff += dtEnd - dtStart;
            //kdDebug()<<k_funcinfo<<dtStart.time().toString()<<" - "<<dtEnd.time().toString()<<"="<<eff.toString(Duration::Format_Day)<<endl;
        }
    }
    //kdDebug()<<k_funcinfo<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<": "<<start.toString()<<" - "<<end.toString()<<": total="<<eff.toString(Duration::Format_Day)<<endl;
    return eff;
}

QPair<QTime, QTime> CalendarDay::interval(const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<endl;
    if (m_state == Map::Working) {
        QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
        for (; it.current(); ++it) {
            QTime t1, t2;
            if (start < it.current()->second && end > it.current()->first) {
                start > it.current()->first ? t1 = start : t1 = it.current()->first;
                end < it.current()->second ? t2 = end : t2 = it.current()->second;
                //kdDebug()<<k_funcinfo<<t1.toString()<<" to "<<t2.toString()<<endl;
                return QPair<QTime, QTime>(t1, t2);
            }
        }
    }
    kdError()<<k_funcinfo<<"No interval"<<endl;
    return QPair<QTime, QTime>(start, end); // hmmmm, what else to do?
}

bool CalendarDay::hasInterval() const {
    return m_state == Map::Working && m_workingIntervals.count() > 0;
}

bool CalendarDay::hasInterval(const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<" "<<start.toString()<<" - "<<end.toString()<<endl;
    if (m_state != Map::Working) {
        return false;
    }
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        if (start < it.current()->second && end > it.current()->first) {
            //kdDebug()<<k_funcinfo<<"true:"<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<" "<<it.current()->first.toString()<<" - "<<it.current()->second.toString()<<endl;
            return true;
        }
    }
    return false;
}

bool CalendarDay::hasIntervalBefore(const QTime &time) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        if (time > it.current()->first) {
            //kdDebug()<<k_funcinfo<<"true:"<<it.current()->first.toString()<<" - "<<it.current()->second.toString()<<endl;
            return true;
        }
    }
    return false;
}

bool CalendarDay::hasIntervalAfter(const QTime &time) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        if (time < it.current()->second) {
            //kdDebug()<<k_funcinfo<<"true:"<<it.current()->first.toString()<<" - "<<it.current()->second.toString()<<endl;
            return true;
        }
    }
    return false;
}

Duration CalendarDay::duration() const {
    Duration dur;
    QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
    for (; it.current(); ++it) {
        DateTime start(QDate::currentDate(), it.current()->first);
        DateTime end(QDate::currentDate(), it.current()->second);
        dur += end - start;
    }
    return dur;
}

/////   CalendarWeekdays   ////
CalendarWeekdays::CalendarWeekdays()
    : m_weekdays(),
      m_workHours(40) {

    //kdDebug()<<k_funcinfo<<"--->"<<endl;
    for (int i=0; i < 7; ++i) {
        m_weekdays.append(new CalendarDay());
    }
    m_weekdays.setAutoDelete(true);
    //kdDebug()<<k_funcinfo<<"<---"<<endl;
}

CalendarWeekdays::CalendarWeekdays(CalendarWeekdays *weekdays)
    : m_weekdays() {
    //kdDebug()<<k_funcinfo<<"--->"<<endl;
    m_weekdays.setAutoDelete(true);
    copy(*weekdays);
    //kdDebug()<<k_funcinfo<<"<---"<<endl;
}

CalendarWeekdays::~CalendarWeekdays() {
    //kdDebug()<<k_funcinfo<<endl;
}

const CalendarWeekdays &CalendarWeekdays::copy(const CalendarWeekdays &weekdays) {
    //kdDebug()<<k_funcinfo<<endl;
    m_weekdays.clear();
    QPtrListIterator<CalendarDay> it = weekdays.weekdays();
    for (; it.current(); ++it) {
        m_weekdays.append(new CalendarDay(it.current()));
    }
    return *this;
}

bool CalendarWeekdays::load(QDomElement &element) {
    //kdDebug()<<k_funcinfo<<endl;
    bool ok;
    int dayNo = QString(element.attribute("day","-1")).toInt(&ok);
    if (dayNo < 0 || dayNo > 6) {
        kdError()<<k_funcinfo<<"Illegal weekday: "<<dayNo<<endl;
        return true; // we continue anyway
    }
    CalendarDay *day = m_weekdays.at(dayNo);
    if (!day)
        day = new CalendarDay();
    if (!day->load(element))
        day->setState(Map::None);
    return true;
}

void CalendarWeekdays::save(QDomElement &element) const {
    //kdDebug()<<k_funcinfo<<endl;
    QPtrListIterator<CalendarDay> it = m_weekdays;
    for (int i=0; it.current(); ++it) {
        QDomElement me = element.ownerDocument().createElement("weekday");
        element.appendChild(me);
        me.setAttribute("day", i++);
        it.current()->save(me);
    }
}    

IntMap CalendarWeekdays::map() {
    IntMap days;
    for (unsigned int i=0; i < m_weekdays.count(); ++i) {
        if (m_weekdays.at(i)->state() > 0)
            days.insert(i+1, m_weekdays.at(i)->state()); //Note: day numbers 1..7
    }
    return days;
}

int CalendarWeekdays::state(const QDate &date) const {
    return state(date.dayOfWeek()-1);
}

int CalendarWeekdays::state(int weekday) const {
    CalendarDay *day = const_cast<CalendarWeekdays*>(this)->m_weekdays.at(weekday);
    return day ? day->state() : Map::None;
}

void CalendarWeekdays::setState(int weekday, int state) {
    CalendarDay *day = m_weekdays.at(weekday);
    if (!day)
        return;
    day->setState(state);
}

const QPtrList<QPair<QTime, QTime> > &CalendarWeekdays::intervals(int weekday) const { 
    CalendarDay *day = const_cast<CalendarWeekdays*>(this)->m_weekdays.at(weekday);
    Q_ASSERT(day);
    return day->workingIntervals();
}

void CalendarWeekdays::setIntervals(int weekday, QPtrList<QPair<QTime, QTime> >intervals) {
    CalendarDay *day = m_weekdays.at(weekday);
    if (day)
        day->setIntervals(intervals); 
}

void CalendarWeekdays::clearIntervals(int weekday) {
    CalendarDay *day = m_weekdays.at(weekday);
    if (day)
        day->clearIntervals(); 
}

bool CalendarWeekdays::operator==(const CalendarWeekdays *wd) const {
    if (m_weekdays.count() != wd->weekdays().count())
        return false;
    for (unsigned int i=0; i < m_weekdays.count(); ++i) {
        // is there a better way to get around this const stuff?
        CalendarDay *day1 = const_cast<CalendarWeekdays*>(this)->m_weekdays.at(i);
        CalendarDay *day2 = const_cast<QPtrList<CalendarDay>&>(wd->weekdays()).at(i);
        if (day1 != day2)
            return false;
    }
    return true;
}
bool CalendarWeekdays::operator!=(const CalendarWeekdays *wd) const {
    if (m_weekdays.count() != wd->weekdays().count())
        return true;
    for (unsigned int i=0; i < m_weekdays.count(); ++i) {
        // is there a better way to get around this const stuff?
        CalendarDay *day1 = const_cast<CalendarWeekdays*>(this)->m_weekdays.at(i);
        CalendarDay *day2 = const_cast<QPtrList<CalendarDay>&>(wd->weekdays()).at(i);
        if (day1 != day2)
            return true;
    }
    return false;
}

Duration CalendarWeekdays::effort(const QDate &date, const QTime &start, const QTime &end) {
    //kdDebug()<<k_funcinfo<<"Day of week="<<date.dayOfWeek()-1<<endl;
    CalendarDay *day = weekday(date.dayOfWeek()-1);
    if (day && day->state() == Map::Working) {
        return day->effort(start, end);
    }
    return Duration::zeroDuration;
}

QPair<QTime, QTime> CalendarWeekdays::interval(const QDate date, const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<endl;
    CalendarDay *day = weekday(date.dayOfWeek()-1);
    if (day && day->state() == Map::Working) {
        if (day->hasInterval(start, end)) {
            return day->interval(start, end);
        }
    }    
    return QPair<QTime, QTime>(start, end); // what to do?
}

bool CalendarWeekdays::hasInterval(const QDate date, const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<date.toString()<<": "<<start.toString()<<" - "<<end.toString()<<endl;
    CalendarDay *day = weekday(date.dayOfWeek()-1);
    return day && day->hasInterval(start, end);
}

bool CalendarWeekdays::hasIntervalAfter(const QDate date, const QTime &time) const {
    //kdDebug()<<k_funcinfo<<date.toString(Qt::ISODate)<<": "<<time.toString()<<endl;
    CalendarDay *day = weekday(date.dayOfWeek()-1);
    return day && day->hasIntervalAfter(time);
}

bool CalendarWeekdays::hasIntervalBefore(const QDate date, const QTime &time) const {
    //kdDebug()<<k_funcinfo<<date.toString(Qt::ISODate)<<": "<<time.toString()<<endl;
    CalendarDay *day = weekday(date.dayOfWeek()-1);
    return day && day->hasIntervalBefore(time);
}

bool CalendarWeekdays::hasInterval() const {
    //kdDebug()<<k_funcinfo<<endl;
    QPtrListIterator<CalendarDay> it = m_weekdays;
    for (; it.current(); ++it) {
        if (it.current()->hasInterval())
            return true;
    }
    return false;
}

CalendarDay *CalendarWeekdays::weekday(int day) const {
    QPtrListIterator<CalendarDay> it = m_weekdays;
    for (int i=0; it.current(); ++it, ++i) {
        if (i == day)
            return it.current();
    }
    return 0;
}

Duration CalendarWeekdays::duration() const {
    Duration dur;
    QPtrListIterator<CalendarDay> it = m_weekdays;
    for (; it.current(); ++it) {
        dur += it.current()->duration();
    }
    return dur;
}

Duration CalendarWeekdays::duration(int _weekday) const {
    CalendarDay *day = weekday(_weekday);
    if (day)
        return day->duration();
    return Duration();
}

QTime CalendarWeekdays::startOfDay(int _weekday) const {
    CalendarDay *day = weekday(_weekday);
    if (day)
        return day->startOfDay();
    return QTime();
}

QTime CalendarWeekdays::endOfDay(int _weekday) const {
    CalendarDay *day = weekday(_weekday);
    if (day)
        return day->endOfDay();
    return QTime();
}
    

/////   CalendarWeek  ////
CalendarWeeks::CalendarWeeks() {
}

CalendarWeeks::~CalendarWeeks() {
    //kdDebug()<<k_funcinfo<<endl;
}

CalendarWeeks::CalendarWeeks(CalendarWeeks *weeks) {
    copy(*weeks);
}

CalendarWeeks &CalendarWeeks::copy(CalendarWeeks& weeks) {
    m_weeks = weeks.weeks();
    return *this;
}

bool CalendarWeeks::load(QDomElement &element) {
    //kdDebug()<<k_funcinfo<<endl;
    bool ok;
    int w = QString(element.attribute("week","-1")).toInt(&ok);
    int y = QString(element.attribute("year","-1")).toInt(&ok);
    int s = QString(element.attribute("state","-1")).toInt(&ok);
    m_weeks.insert(w, y, s);
    return true;
}

void CalendarWeeks::save(QDomElement &element) const {
    //kdDebug()<<k_funcinfo<<endl;
    WeekMap::ConstIterator it;
    for (it = m_weeks.constBegin(); it != m_weeks.constEnd(); ++it) {
        QDomElement me = element.ownerDocument().createElement("week");
        element.appendChild(me);
        QPair<int, int> w = WeekMap::week(it.key());
        me.setAttribute("week", w.first);
        me.setAttribute("year", w.second);
        me.setAttribute("state", it.data());
    }
}    

void CalendarWeeks::setWeek(int week, int year, int type) {
    m_weeks.insert(week, year, type);
}

int CalendarWeeks::state(const QDate &date) {
    int year;
    int week = date.weekNumber(&year);
    int st = m_weeks.state(week, year);
    //kdDebug()<<k_funcinfo<<week<<", "<<year<<"="<<st<<endl;
    return st;
}

/////   Calendar   ////

Calendar::Calendar()
    : m_parent(0),
      m_project(0),
      m_deleted(false) {

    init();
}

Calendar::Calendar(QString name, Calendar *parent)
    : m_name(name),
      m_parent(parent),
      m_project(0),
      m_deleted(false),
      m_days() {
    
    init();
}

Calendar::~Calendar() {
    //kdDebug()<<k_funcinfo<<"deleting "<<m_name<<endl;
    removeId();
    delete m_weeks; 
    delete m_weekdays; 
}
Calendar::Calendar(Calendar *calendar)
    : m_days() {
    m_days.setAutoDelete(true);
    copy(*calendar);
}

const Calendar &Calendar::copy(Calendar &calendar) {
    m_name = calendar.name();
    m_parent = calendar.parent();
    m_deleted = calendar.isDeleted();
    m_id = calendar.id();
    
    QPtrListIterator<CalendarDay> it = calendar.days();
    for (; it.current(); ++it) {
        m_days.append(new CalendarDay(it.current()));
    }
    m_weeks = new CalendarWeeks(calendar.weeks());
    m_weekdays = new CalendarWeekdays(calendar.weekdays());
    return *this;
}

void Calendar::init() {
    m_days.setAutoDelete(true);
    m_weeks = new CalendarWeeks();
    m_weekdays = new CalendarWeekdays();
}

void Calendar::setProject(Project *project) { 
    m_project = project;
    generateId();
}

void Calendar::setDeleted(bool yes) {
    if (yes) {
        removeId();
    } else {
        setId(m_id);
    }
    m_deleted = yes;
}
bool Calendar::setId(QString id) {
    //kdDebug()<<k_funcinfo<<id<<endl;
    if (id.isEmpty()) {
        kdError()<<k_funcinfo<<"id is empty"<<endl;
        m_id = id;
        return false;
    }
    Calendar *c = findCalendar();
    if (c == this) {
        //kdDebug()<<k_funcinfo<<"My id found, remove it"<<endl;
        removeId();
    } else if (c) {
        //can happen when making a copy
        kdError()<<k_funcinfo<<"My id '"<<m_id<<"' already used for different node: "<<c->name()<<endl;
    }
    if (findCalendar(id)) {
        kdError()<<k_funcinfo<<"id '"<<id<<"' is already used for different node: "<<findCalendar(id)->name()<<endl;
        m_id = QString(); // hmmm
        return false;
    }
    m_id = id;
    insertId(id);
    //kdDebug()<<k_funcinfo<<m_name<<": inserted id="<<id<<endl;
    return true;
}

void Calendar::generateId() {
    if (!m_id.isEmpty()) {
        removeId();
    }
    for (int i=0; i<32000 ; ++i) {
        m_id = m_id.setNum(i);
        if (!findCalendar()) {
            insertId(m_id);
            return;
        }
    }
    m_id = QString();
}

bool Calendar::load(QDomElement &element) {
    //kdDebug()<<k_funcinfo<<element.text()<<endl;
    //bool ok;
    setId(element.attribute("id"));
    m_parentId = element.attribute("parent");
    m_name = element.attribute("name","");
    //TODO parent
    
    QDomNodeList list = element.childNodes();
    for (unsigned int i=0; i<list.count(); ++i) {
        if (list.item(i).isElement()) {
            QDomElement e = list.item(i).toElement();
            if (e.tagName() == "weekday") {
                if (!m_weekdays->load(e))
                    return false;
            }
            if (e.tagName() == "week") {
                if (!m_weeks->load(e)) {
                    return false;
                }
            }
            if (e.tagName() == "day") {
                CalendarDay *day = new CalendarDay();
                if (day->load(e)) {
                    if (!day->date().isValid()) {
                        delete day;
                        kdError()<<k_funcinfo<<m_name<<": Failed to load calendarDay - Invalid date"<<endl;
                    } else {
                        CalendarDay *d = findDay(day->date());
                        if (d) {
                            // already exists, keep the new
                            removeDay(d);
                            kdWarning()<<k_funcinfo<<m_name<<" Load calendarDay - Date already exists"<<endl;
                        }
                    }
                    addDay(day);
                } else {
                    delete day;
                    kdError()<<k_funcinfo<<"Failed to load calendarDay"<<endl;
                    return true; //false; don't throw away the whole calendar
                }
            }
        }
    }
    return true;
}

void Calendar::save(QDomElement &element) const {
    //kdDebug()<<k_funcinfo<<m_name<<endl;
    if (m_deleted)
        return;
    
    QDomElement me = element.ownerDocument().createElement("calendar");
    element.appendChild(me);
    if (m_parent && !m_parent->isDeleted()) 
        me.setAttribute("parent", m_parent->id());
    me.setAttribute("name", m_name);
    me.setAttribute("id", m_id);
    m_weeks->save(me);
    m_weekdays->save(me);
    QPtrListIterator<CalendarDay> it = m_days;
    for (; it.current(); ++it) {
        QDomElement e = me.ownerDocument().createElement("day");
        me.appendChild(e);
        it.current()->save(e);
    }
    
}

CalendarDay *Calendar::findDay(const QDate &date, bool skipNone) const {
    //kdDebug()<<k_funcinfo<<date.toString()<<endl;
    QPtrListIterator<CalendarDay> it = m_days;
    for (; it.current(); ++it) {
        if (it.current()->date() == date) {
            if (skipNone  && it.current()->state() == Map::None) {
                continue; // hmmm, break?
            }
            return it.current();
        }
    }
    //kdDebug()<<k_funcinfo<<date.toString()<<" not found"<<endl;
    return 0;
}

int Calendar::weekState(const QDate &date) const {
    return m_weeks ? m_weeks->state(date) : Map::None;
}

bool Calendar::hasParent(Calendar *cal) {
    //kdDebug()<<k_funcinfo<<endl;
    if (!m_parent)
        return false;
    if (m_parent == cal)
        return true;
    return m_parent->hasParent(cal);
}

Duration Calendar::parentDayWeekEffort(const QDate &date, const QTime &start, const QTime &end) const {
    if (!m_parent || m_parent->isDeleted()) {
        return Duration::zeroDuration;
    }    
    //kdDebug()<<k_funcinfo<<m_name<<" "<<date.toString(Qt::ISODate)<<": "<<start.toString()<<" - "<<end.toString()<<endl;
    CalendarDay *day = m_parent->findDay(date, true);
    if (day) {
        return day->effort(start, end);
    }
    if (m_parent->weekState(date) == Map::NonWorking) {
        return Duration::zeroDuration;
    }
    return m_parent->parentDayWeekEffort(date, start, end);
}

Duration Calendar::parentWeekdayEffort(const QDate &date, const QTime &start, const QTime &end) const {
    if (!m_parent || m_parent->isDeleted()) {
        return Duration::zeroDuration;
    }
    //kdDebug()<<k_funcinfo<<m_name<<" "<<date.toString(Qt::ISODate)<<": "<<start.toString()<<" - "<<end.toString()<<endl;
    if (m_parent->weekdays()) {
        if (m_parent->weekdays()->state(date) == Map::Working) {
            return m_parent->weekdays()->effort(date, start, end);
        }
        if (m_parent->weekdays()->state(date) == Map::NonWorking) {
            return Duration::zeroDuration;
        }
    }
    return m_parent->parentWeekdayEffort(date, start, end);
}

Duration Calendar::effort(const QDate &date, const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<m_name<<": "<<date.toString(Qt::ISODate)<<" "<<start.toString()<<" - "<<end.toString()<<endl;
    if (start == end) {
        return Duration::zeroDuration;
    }
    QTime _start = start;
    QTime _end = end;
    if (start > end) {
        _start = end;
        _end = start;
    }
    // first, check my own day
    CalendarDay *day = findDay(date, true);
    if (day&& day->state() != Map::None) {
        if (day->state() == Map::Working) {
            return day->effort(_start, _end);
        } else {
            return Duration::zeroDuration;
        }
    }
    // check my own weeks
    if (weekState(date) == Map::NonWorking) {
        // nonworking week, no effort
        //kdDebug()<<k_funcinfo<<"Non working week"<<endl;
        return Duration::zeroDuration;
    }
    // check my parent's day & weeks
    int st = parentDayWeekHasInterval(date, _start, _end);
    if (st == Map::NonWorking) {
        return Duration::zeroDuration;
    }
    if (st == Map::Working) {
        return parentDayWeekEffort(date, _start, _end);
    }
    // check my own weekdays
    if (m_weekdays && m_weekdays->state(date) != Map::None) {
        return m_weekdays->effort(date, _start, _end);
    }
    // check my parent's weekdays
    if (parentWeekdayHasInterval(date, _start, _end)) {
        return parentWeekdayEffort(date, _start, _end);
    }
    return Duration::zeroDuration;
}

Duration Calendar::effort(const DateTime &start, const Duration &duration) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" duration "<<duration.toString()<<endl;
    Duration eff;
    if (duration == Duration::zeroDuration)
        return eff;
    QDate date = start.date();
    QTime startTime = start.time();
    DateTime end = start + duration;
    QTime endTime = end.time();
    if (end.date() > date) {
        endTime.setHMS(23, 59, 59, 999);
    }
    eff = effort(date, startTime, endTime); // first day
    // Now get all the rest of the days
    int i=0; //FIXME: Set a period for which this calendar is valid
    for (date = date.addDays(1); i < 50 && date <= end.date(); date = date.addDays(1)) {
        if (date < end.date())
             eff += effort(date, QTime(), QTime(23, 59, 59, 999)); // whole day
        else 
             eff += effort(date, QTime(), end.time());
        //kdDebug()<<k_funcinfo<<": eff now="<<eff.toString(Duration::Format_Day)<<endl;
    }
    //kdDebug()<<k_funcinfo<<start.date().toString()<<"- "<<end.date().toString()<<": total="<<eff.toString(Duration::Format_Day)<<endl;
    return eff;
}

QPair<QTime, QTime> Calendar::parentDayWeekInterval(const QDate &date, const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    if (!m_parent)
        return QPair<QTime, QTime>(start, end);
    
    CalendarDay *day = m_parent->findDay(date, true);
    if (day && day->state() != Map::None) {
        if (day->hasInterval(start, end))
            return day->interval(start, end);
    } 
    if (m_parent->weekState(date) == Map::NonWorking) {
            return QPair<QTime, QTime>(start, end);
    }
    return m_parent->parentDayWeekInterval(date, start, end);
}

QPair<QTime, QTime> Calendar::parentWeekdayInterval(const QDate &date, const QTime &start, const QTime &end) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    if (!m_parent || m_parent->isDeleted())
        return QPair<QTime, QTime>(start, end);
    
    if (m_parent->weekdays() && m_parent->weekdays()->hasInterval(date, start, end)) {
        return m_parent->weekdays()->interval(date, start, end);
    }
    return m_parent->parentWeekdayInterval(date, start, end);
}

QPair<DateTime, DateTime> Calendar::interval(const DateTime &start, const DateTime &end) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    QTime startTime;
    QTime endTime;
    QDate date = start.date();
    for (int i=0; date <= end.date() && i<20; date = date.addDays(1), ++i) { //FIXME Set a period for which this calendar is valid
        if (date < end.date())
            endTime = QTime(23, 59, 59, 999);
        else
            endTime = end.time();
        if (date > start.date())
            startTime = QTime();
        else 
            startTime = start.time();
            
        //kdDebug()<<k_funcinfo<<date.toString()<<": "<<startTime.toString()<<" - "<<endTime.toString()<<endl;
        CalendarDay *day = findDay(date, true);
        if (day && day->state() != Map::None) {
            if (day->hasInterval(startTime, endTime)) {
                QPair<QTime, QTime> res = day->interval(startTime, endTime);
                //kdDebug()<<k_funcinfo<<res.first.toString()<<" to "<<res.second.toString()<<endl;
                return QPair<DateTime, DateTime>(DateTime(date,res.first), DateTime(date,res.second));
            }
        }
        if (weekState(date) == Map::NonWorking) {
            //kdDebug()<<k_funcinfo<<date.toString()<<" is nonworking day"<<endl;
            continue;
        }
        int st = parentDayWeekHasInterval(date, startTime, endTime);
        if (st == Map::NonWorking) {
            continue;
        }
        if (st == Map::Working) {
            QPair<QTime, QTime> res = parentDayWeekInterval(date, startTime, endTime);
            return QPair<DateTime, DateTime>(DateTime(date,res.first), DateTime(date,res.second));
        }
        if (m_weekdays && m_weekdays->hasInterval(date, startTime, endTime)) {
            QPair<QTime, QTime> res = m_weekdays->interval(date, startTime, endTime);
            //kdDebug()<<k_funcinfo<<res.first.toString()<<" to "<<res.second.toString()<<endl;
            return QPair<DateTime, DateTime>(DateTime(date,res.first), DateTime(date,res.second));
        }
        if (parentWeekdayHasInterval(date, startTime, endTime)) {
            QPair<QTime, QTime> res = parentWeekdayInterval(date, startTime, endTime);
            return QPair<DateTime, DateTime>(DateTime(date,res.first), DateTime(date,res.second)); 
        }
    }
    kdError()<<k_funcinfo<<"Didn't find an interval"<<endl;
    return QPair<DateTime, DateTime>(start, end); // hmmmm, what else to do?
}

// FIXME: This logic is full of holes
bool Calendar::hasIntervalBefore(const DateTime &time) const {
    if (m_weekdays->hasInterval()) {
        return true;
    }
    QPtrListIterator<CalendarDay> it = m_days;
    for (; it.current(); ++it) {
        if (it.current()->state() == Map::Working) {
            if (it.current()->date() == time.date()) {
                if (it.current()->hasIntervalBefore(time.time()))
                    return true;
            } else if (it.current()->date() < time.date()) {
                if (it.current()->hasInterval())
                    return true;
            }
        }
    }
    return m_parent ? m_parent->hasIntervalBefore(time) : false;
}

// FIXME: This logic is full of holes
bool Calendar::hasIntervalAfter(const DateTime &time) const {
    if (m_weekdays->hasInterval()) {
        return true;
    }
    QPtrListIterator<CalendarDay> it = m_days;
    for (; it.current(); ++it) {
        if (it.current()->state() == Map::Working) {
            if (it.current()->date() == time.date()) {
                if (it.current()->hasIntervalAfter(time.time()))
                    return true;
            } else if (it.current()->date() > time.date()) {
                if (it.current()->hasInterval())
                    return true;
            }
        }
    }
    return m_parent ? m_parent->hasIntervalAfter(time) : false;
}

int Calendar::parentDayWeekHasInterval(const QDate &date, const QTime &start, const QTime &end) const {
    if (!m_parent || m_parent->isDeleted()) {
        //kdDebug()<<k_funcinfo<<m_name<<" "<<date.toString(Qt::ISODate)<<": "<<start.toString()<<" - "<<end.toString()<<"=false"<<endl;
        return Map::None;
    }
    CalendarDay *day = m_parent->findDay(date, true);
    if (day) {
        if (day->state() == Map::Working && day->hasInterval(start, end))
            return Map::Working;
        else
            return Map::NonWorking;
    } 
    if (m_parent->weekState(date) == Map::NonWorking) {
        //kdDebug()<<k_funcinfo<<m_name<<" "<<date.toString(Qt::ISODate)<<": "<<start.toString()<<" - "<<end.toString()<<"weekState=NonWorking"<<endl;
        return Map::NonWorking;
    }
    return m_parent->parentDayWeekHasInterval(date, start, end);
}

bool Calendar::parentWeekdayHasInterval(const QDate &date, const QTime &start, const QTime &end) const {
    if (!m_parent || m_parent->isDeleted()) {
        //kdDebug()<<k_funcinfo<<m_name<<" "<<date.toString(Qt::ISODate)<<": "<<start.toString()<<" - "<<end.toString()<<"=false"<<endl;
        return false;
    }
    if (m_parent->weekdays()) {
        CalendarDay *day = m_parent->weekdays()->weekday(date);
        if (day && day->state() == Map::NonWorking) {
            return false;
        }
        if (day && day->state() == Map::Working) {
            if (day->hasInterval(start, end)) {
                return true;
            }
            return false;
        }
    }
    return m_parent->parentWeekdayHasInterval(date, start, end);
}

bool Calendar::hasInterval(const DateTime &start, const DateTime &end) const {
    //kdDebug()<<k_funcinfo<<start.toString()<<" - "<<end.toString()<<endl;
    QTime startTime;
    QTime endTime;
    QDate date = start.date();
    for (int i=0; date <= end.date() && i<50; date = date.addDays(1), ++i) { // FIXME Set a period for which this calendar is valid
        if (date < end.date())
            endTime = QTime(23, 59, 59, 999);
        else
            endTime = end.time();
        if (date > start.date())
            startTime = QTime();
        else 
            startTime = start.time();
        
        CalendarDay *day = findDay(date, true);
        if (day) {
            return day->hasInterval(startTime, endTime);
        } 
        if (weekState(date) == Map::NonWorking) {
             continue;
        }
        int st = parentDayWeekHasInterval(date, startTime, endTime);
        if (st != Map::None) {
            return st == Map::Working;
        }
        if (m_weekdays && m_weekdays->hasInterval(date, startTime, endTime)) {
            return true;
        }
        if (parentWeekdayHasInterval(date, startTime, endTime)) {
            return true;
        }
    }
    return false;
}

DateTime Calendar::availableAfter(const DateTime &time, int days) {
    kdDebug()<<k_funcinfo<<m_name<<": check from "<<time.toString()<<" days="<<days<<endl;
    DateTime start = time;
    DateTime end(time.date(), QTime(23,59,59,999));
    DateTime t = time;
    int i = days == -1 ? 28 : days;
    for (; i > 0 && !hasInterval(start, end); --i) { //FIXME Set a period for which this calendar is valid
        end = end.addDays(1);
        start = DateTime(end.date(), QTime());
    }
    if (i > 0)
        t = interval(start, end).first;
    kdDebug()<<k_funcinfo<<m_name<<" "<<i<<": "<<t.toString()<<endl;
    return t;
}

DateTime Calendar::availableBefore(const DateTime &time, int days) {
    kdDebug()<<k_funcinfo<<m_name<<": check from "<<time.toString()<<" days="<<days<<endl;
    DateTime start(time.date(), QTime());
    DateTime end = time;
    DateTime t = time;
    int i = days == -1 ? 28 : days;
    for (; i > 0 && !hasInterval(start, end); --i) { //FIXME Set a period for which this calendar is valid
        start = start.addDays(-1);
        end = DateTime(start.date(), QTime(23,59,59,999));
    }
    if (i > 0)
        t = interval(start, end).second;
    kdDebug()<<k_funcinfo<<m_name<<" "<<i<<": "<<t.toString()<<endl;
    return t;
}

Calendar *Calendar::findCalendar(const QString &id) const { 
    return (m_project ? m_project->findCalendar(id) : 0); 
}

bool Calendar::removeId(const QString &id) { 
    return (m_project ? m_project->removeCalendarId(id) : false); 
}

void Calendar::insertId(const QString &id){ 
    if (m_project)
        m_project->insertCalendarId(id, this); 
}

/////////////
StandardWorktime::StandardWorktime() {
    init();
}

StandardWorktime::StandardWorktime(StandardWorktime *worktime) {
    if (worktime) {
        m_year = worktime->durationYear();
        m_month = worktime->durationMonth();
        m_week = worktime->durationWeek();
        m_day = worktime->durationDay();
    } else {
        init();
    }
}

StandardWorktime::~StandardWorktime() {
    //kdDebug()<<k_funcinfo<<"("<<this<<")"<<endl;
}

void StandardWorktime::init() {
    // Some temporary sane default values
    m_year = Duration(0, 1760, 0);
    m_month = Duration(0, 176, 0);
    m_week = Duration(0, 40, 0);
    m_day = Duration(0, 8, 0);
}

bool StandardWorktime::load(QDomElement &element) {
    //kdDebug()<<k_funcinfo<<endl;
    m_year = Duration::fromString(element.attribute("year"), Duration::Format_Hour); 
    m_month = Duration::fromString(element.attribute("month"), Duration::Format_Hour); 
    m_week = Duration::fromString(element.attribute("week"), Duration::Format_Hour); 
    m_day = Duration::fromString(element.attribute("day"), Duration::Format_Hour); 
    return true;
}

void StandardWorktime::save(QDomElement &element) const {
    //kdDebug()<<k_funcinfo<<endl;
    QDomElement me = element.ownerDocument().createElement("standard-worktime");
    element.appendChild(me);
    me.setAttribute("year", m_year.toString(Duration::Format_Hour));
    me.setAttribute("month", m_month.toString(Duration::Format_Hour));
    me.setAttribute("week", m_week.toString(Duration::Format_Hour));
    me.setAttribute("day", m_day.toString(Duration::Format_Hour));
}

#ifndef NDEBUG
void CalendarDay::printDebug(QCString indent) {
    QString s[] = {"None", "Non-working", "Working"};
    kdDebug()<<indent<<" "<<m_date.toString()<<" = "<<s[m_state]<<endl;
    if (m_state == Map::Working) {
        indent += "  ";
        QPtrListIterator<QPair<QTime, QTime> > it = m_workingIntervals;
        for (; it.current(); ++it) {
            kdDebug()<<indent<<" Interval: "<<it.current()->first<<" to "<<it.current()->second<<endl;
        }
    }
    
}
void CalendarWeekdays::printDebug(QCString indent) {
    kdDebug()<<indent<<"Weekdays ------"<<endl;
    QPtrListIterator<CalendarDay> it = m_weekdays;
    for (char c='0'; it.current(); ++it) {
        it.current()->printDebug(indent + "  Day " + c++ + ": ");
    }

}
void CalendarWeeks::printDebug(QCString indent) {
    QCString s[] = {"None", "Non-working", "Working"};
    kdDebug()<<indent<<" Weeks ------"<<endl;
    indent += "  ";
    WeekMap::iterator it;
    for (it = m_weeks.begin(); it != m_weeks.end(); ++it) {
        kdDebug()<<indent<<" Week: "<<it.key()<<" = "<<s[it.data()]<<endl;
    }
}
void Calendar::printDebug(QCString indent) {
    kdDebug()<<indent<<"Calendar "<<m_id<<": '"<<m_name<<"' Deleted="<<m_deleted<<endl;
    kdDebug()<<indent<<"  Parent: "<<(m_parent ? m_parent->name() : "No parent")<<endl;
    m_weekdays->printDebug(indent + "  ");
    m_weeks->printDebug(indent + "  ");
    kdDebug()<<indent<<"  Days --------"<<endl;
    QPtrListIterator<CalendarDay> it = m_days;
    for (; it.current(); ++it) {
        it.current()->printDebug(indent + "  ");
    }
}

void StandardWorktime::printDebug(QCString indent) {
    kdDebug()<<indent<<"StandardWorktime "<<endl;
    kdDebug()<<indent<<"Year: "<<m_year.toString()<<endl;
    kdDebug()<<indent<<"Month: "<<m_month.toString()<<endl;
    kdDebug()<<indent<<"Week: "<<m_week.toString()<<endl;
    kdDebug()<<indent<<"Day: "<<m_day.toString()<<endl;
}

#endif

}  //KPlato namespace
