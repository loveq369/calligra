/* This file is part of the KDE project
   Copyright (C) 2005 Dag Andersen <danders@get2net.dk>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KPTCHARTDATASET_H
#define KPTCHARTDATASET_H

#include <qdatetime.h>
#include <qptrlist.h>
#include <qpen.h>
#include <qbrush.h>

namespace KPlato
{

class KPTChartDataSet;

class KPTChartDataSetItem
{
public:
    KPTChartDataSetItem(const QDateTime &x1, const QDateTime &x2, double y1, double y2);
    KPTChartDataSetItem(double y1, QPen *pen=0);
    KPTChartDataSetItem(const QDateTime &x1, QPen *pen=0);
    ~KPTChartDataSetItem();
    
    KPTChartDataSet *parent() const { return m_parent; }
    void setParent(KPTChartDataSet *parent) { m_parent = parent; }
    
    void setPrio(double prio);
    double prio() const { return m_prio; }
    QPen *pen() const;
    void setPen(QPen *pen);
    QBrush *brush() const;
    void setBrush(QBrush *brush);
    
    QDateTime x1() const { return m_x1; }
    QDateTime x2() const { return m_x2; }
    double y1() const { return m_y1; }
    double y2() const { return m_y2; }
    
private:
    KPTChartDataSet *m_parent;
    double m_prio;
    QPen *m_pen;
    QBrush *m_brush;
    QDateTime m_x1;
    QDateTime m_x2;
    double m_y1;
    double m_y2;
};

class KPTChartDataSet 
{
public:
    enum DrawMode { Mode_Default, Mode_Bar, Mode_Line };
    
    KPTChartDataSet(QPen *pen=0, QBrush *brush=0);
    ~KPTChartDataSet();
    
    void setDrawMode(DrawMode mode) { m_mode = mode; }
    DrawMode drawMode() const { return m_mode; }
    QPen *pen() const { return m_pen; }
    void setPen(QPen *pen) { delete m_pen; m_pen = pen; }
    QBrush *brush() const { return m_brush; }
    void setBrush(QBrush *brush) { delete m_brush; m_brush = brush; }
    
    void append(KPTChartDataSetItem *item);
    const QPtrList<KPTChartDataSetItem> &dataItems() const { return m_dataItems; }

private:
    DrawMode m_mode;
    QPen *m_pen;
    QBrush *m_brush;
    QPtrList<KPTChartDataSetItem> m_dataItems;
};

}  //KPlato namespace

#endif
