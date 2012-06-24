/* This file is part of the KDE project
 * Copyright (C) 2012 Paul Mendez <paulestebanms@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (  at your option ) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KPrTimeLineView.h"

//Stage Headers
#include "KPrAnimationsTimeLineView.h"
#include "KPrAnimationsDataModel.h"

//QT HEADERS
#include <QScrollArea>
#include <QVBoxLayout>
#include <QRect>
#include <QModelIndex>
#include <QPixmap>
#include <QEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolTip>
#include <qmath.h>

//Default height for rows
const int LINE_HEIGHT = 25;

//Default invalid value for columns and rows index
const int INVALID = -1;

KPrTimeLineView::KPrTimeLineView(QWidget *parent)
    : QWidget(parent)
    , m_resize(false)
    , m_move(false)
    , m_adjust(false)
    , m_resizedRow(INVALID)
    , startDragPos(0)
{
    m_mainView = qobject_cast<KPrAnimationsTimeLineView*>(parent);
    Q_ASSERT(m_mainView);
    setFocusPolicy(Qt::WheelFocus);
    setMinimumSize(minimumSizeHint());
    setMouseTracking(true);
}

QSize KPrTimeLineView::sizeHint() const
{
    int rows = m_mainView->model()
            ? m_mainView->rowCount() : 1;
    return QSize(m_mainView->totalWidth(), rows * m_mainView->rowsHeigth());
}

QSize KPrTimeLineView::minimumSizeHint() const
{

    int rows = m_mainView->model()
            ? m_mainView->rowCount() : 1;
    return QSize(m_mainView->totalWidth(), rows * m_mainView->rowsHeigth());
}

bool KPrTimeLineView::eventFilter(QObject *target, QEvent *event)
{
    if (QScrollArea *scrollArea = m_mainView->scrollArea()) {
        if (target == scrollArea && event->type() == QEvent::Resize) {
            if (QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event)) {
                const int ExtraWidth = 5;
                QSize size = resizeEvent->size();
                size.setHeight(sizeHint().height());
                int width = size.width() - (ExtraWidth +
                                            scrollArea->verticalScrollBar()->sizeHint().width());
                size.setWidth(width);
                resize(size);
            }
        }
    }
    return QWidget::eventFilter(target, event);
}

void KPrTimeLineView::keyPressEvent(QKeyEvent *event)
{
    if (m_mainView->model()) {
        int row = INVALID;
        int column = INVALID;
        if (event->key() == Qt::Key_Left) {
            column = qMax(0, m_mainView->selectedColumn() - 1);
        }
        else if (event->key() == Qt::Key_Right) {
            column = qMin(m_mainView->model()->columnCount(QModelIndex()) - 3,
                          m_mainView->selectedColumn() + 1);
        }
        else if (event->key() == Qt::Key_Up) {
            row = qMax(0, m_mainView->selectedRow() - 1);
        }
        else if (event->key() == Qt::Key_Down) {
            row = qMin(m_mainView->model()->rowCount() - 1,
                       m_mainView->selectedRow() + 1);
        }
        row = row == INVALID ? m_mainView->selectedRow() : row;
        column = column == INVALID ? m_mainView->selectedColumn() : column;
        if (row != m_mainView->selectedRow() ||
                column != m_mainView->selectedColumn()) {
            QModelIndex index = m_mainView->model()->index(row, column);
            m_mainView->setCurrentIndex(index);
            emit clicked(index);
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void KPrTimeLineView::mousePressEvent(QMouseEvent *event)
{
    int row = rowAt(event->y());
    int column = columnAt(event->x());

    m_mainView->setSelectedRow(row);
    m_mainView->setSelectedColumn(column);
    if (column == StartTime) {
        int startPos = 0;
        for (int i = 0; i < StartTime; i++) {
            startPos = startPos + m_mainView->widthOfColumn(i);
        }
        int y = row*m_mainView->rowsHeigth();
        QRect rect(startPos,y,startPos+m_mainView->widthOfColumn(column), m_mainView->rowsHeigth());

        int lineHeigth = qMin(LINE_HEIGHT , rect.height());
        int yCenter = (rect.height() - lineHeigth)/2;
        qreal stepSize  = m_mainView->widthOfColumn(StartTime)/m_mainView->numberOfSteps();
        qreal duration = m_mainView->model()->data(m_mainView->model()->index(row, Duration)).toDouble();
        int startOffSet = m_mainView->calculateStartOffset(row);
        qreal start = m_mainView->model()->data(m_mainView->model()->index(row, StartTime)).toDouble() +
                startOffSet;

        QRectF lineRect(rect.x() + stepSize*start+stepSize*duration - 6, rect.y() + yCenter,
                        8, lineHeigth);
        // If user click near the end of the line he could resize
        if (lineRect.contains(event->x(), event->y())) {
            m_resize = true;
            m_resizedRow = row;
            setCursor(Qt::SizeHorCursor);
        } else {
            m_resize = false;
            m_move = false;
            lineRect = QRectF(rect.x() + stepSize*start, rect.y() + yCenter, stepSize*duration, lineHeigth);
            if (lineRect.contains(event->x(), event->y())) {
                startDragPos = event->x() - lineRect.x();
                m_move = true;
                m_resizedRow = row;
                setCursor(Qt::DragMoveCursor);
            }
        }
    }
    emit clicked(m_mainView->model()->index(row, column));


}

void KPrTimeLineView::mouseMoveEvent(QMouseEvent *event)
{
    // Change size of line
    if (m_resize) {
        const qreal subSteps = 0.2;
        int startPos = 0;
        for (int i = 0; i < StartTime; i++) {
            startPos = startPos + m_mainView->widthOfColumn(i);
        }
        int row = m_resizedRow;
        //calculate real start
        int startOffSet = m_mainView->calculateStartOffset(row);

        qreal start = m_mainView->model()->data(m_mainView->model()->index(row, StartTime)).toDouble() + startOffSet;
        qreal duration = m_mainView->model()->data(m_mainView->model()->index(row, Duration)).toDouble();
        qreal totalSteps = m_mainView->numberOfSteps();
        qreal stepSize  = m_mainView->widthOfColumn(StartTime)/totalSteps;

        if ((event->pos().x() > (startPos+stepSize*start - 5)) &&
                ((event->pos().x()) < (startPos+m_mainView->widthOfColumn(StartTime)))) {
            qreal newLength = (event->pos().x() - startPos - stepSize*start)/(stepSize);
            newLength = qFloor((newLength - modD(newLength, subSteps))*100.0)/100.0;
            m_mainView->model()->setData(m_mainView->model()->index(row, Duration),newLength*1000);
            emit timeValuesChanged(m_mainView->model()->index(row, Duration));
            m_adjust = false;
            if (newLength < duration)
                m_adjust = true;
        } else if ( ((event->pos().x()) > (startPos+m_mainView->widthOfColumn(StartTime)))) {
            m_adjust = true;
        }
        update();
    }
    if (m_move) {
        const int Padding = 2;
        int startPos = 0;
        const qreal subSteps = 0.2;
        for (int i = 0; i < StartTime; i++) {
            startPos = startPos + m_mainView->widthOfColumn(i);
        }
        int row = m_resizedRow;
        //calculate real start
        int startOffSet = m_mainView->calculateStartOffset(row);

        qreal duration = m_mainView->model()->data(m_mainView->model()->index(row, Duration)).toDouble();
        qreal start = m_mainView->model()->data(m_mainView->model()->index(row, StartTime)).toDouble() + startOffSet;
        qreal totalSteps = m_mainView->numberOfSteps();
        qreal stepSize  = m_mainView->widthOfColumn(StartTime)/totalSteps;

        if ((event->pos().x() > (startPos+startDragPos)) &&
                ((event->pos().x() + (duration*stepSize-startDragPos) + Padding*2)  <
                 (startPos+m_mainView->widthOfColumn(StartTime)))) {
            qreal newPos = (event->pos().x() - (startPos + startDragPos))/(stepSize);
            newPos = qFloor((newPos - modD(newPos, subSteps))*100.0)/100.0;
            m_mainView->model()->setData(m_mainView->model()->index(row, StartTime),(newPos - startOffSet) * 1000);
            emit timeValuesChanged(m_mainView->model()->index(row, StartTime));
            m_adjust = false;
            if (newPos <= start) {
                m_adjust = true;
            }
        } else if (((event->pos().x() + (duration*stepSize-startDragPos) + Padding*2)  >
                    (startPos+m_mainView->widthOfColumn(StartTime)))) {
            m_mainView->incrementScale();
        }
        update();
    }
    QWidget::mouseMoveEvent(event);

}

void KPrTimeLineView::mouseReleaseEvent(QMouseEvent *event)
{
    m_resize = false;
    m_move = false;
    if (m_adjust) {
        m_mainView->adjustScale();
        m_adjust = false;
    }
    setCursor(Qt::ArrowCursor);
    QWidget::mouseReleaseEvent(event);
    update();
}

bool KPrTimeLineView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QModelIndex index = m_mainView->model()->index(rowAt(helpEvent->pos().y()),columnAt(helpEvent->pos().x()));
        if (index.isValid()) {
            QString text = m_mainView->model()->data(index, Qt::ToolTipRole).toString();
            QToolTip::showText(helpEvent->globalPos(), text);
        } else {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}

int KPrTimeLineView::rowAt(int ypos)
{
    int row = static_cast<int>(ypos / m_mainView->rowsHeigth());
    return row;
}

int KPrTimeLineView::columnAt(int xpos)
{
    int column;
    if (xpos  < m_mainView->widthOfColumn(ShapeThumbnail))
        column = ShapeThumbnail;
    else if (xpos  < m_mainView->widthOfColumn(ShapeThumbnail) + m_mainView->widthOfColumn(AnimationIcon))
        column = AnimationIcon;
    else
        column = StartTime;
    return column;
}

void KPrTimeLineView::paintEvent(QPaintEvent *event)
{
    if (!m_mainView->model())
        return;
    const int RowHeigth = m_mainView->rowsHeigth();
    const int MinY = qMax(0, event->rect().y() - RowHeigth);
    const int MaxY = MinY + event->rect().height() + RowHeigth;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing);
    int row = MinY/RowHeigth;
    int y = row * RowHeigth;

    for (; row < m_mainView->rowCount(); ++row) {
        paintRow(&painter, row, y, RowHeigth);
        y += RowHeigth;
        if (y > MaxY)
            break;
    }
}

void KPrTimeLineView::paintRow(QPainter *painter, int row, int y, const int RowHeight)
{
    int start = 0;
    //Column 0
    int column = ShapeThumbnail;
    paintIconRow(painter, start, y, row, column, RowHeight - 2, RowHeight);

    //Column 1
    column = AnimationIcon;
    start = start + m_mainView->widthOfColumn(column - 1);
    paintIconRow(painter, start, y, row, column, RowHeight/2, RowHeight);

    //Column 2 (6 y 7)
    column = StartTime;
    start = start + m_mainView->widthOfColumn(column - 1);
    QRect rect(start, y, m_mainView->widthOfColumn(column), RowHeight);
    paintItemBackground(painter, rect,
                        row == m_mainView->selectedRow());
    paintLine(painter, row, rect,
              row == m_mainView->selectedRow());


}

void KPrTimeLineView::paintLine(QPainter *painter, int row, const QRect &rect, bool selected)
{
    QColor m_color = m_mainView->colorforRow(row);
    int lineHeigth = qMin(LINE_HEIGHT , rect.height());
    int vPadding = (rect.height() - lineHeigth)/2;
    int stepSize  = m_mainView->widthOfColumn(StartTime)/m_mainView->numberOfSteps();
    int startOffSet = m_mainView->calculateStartOffset(row);
    qreal duration = m_mainView->model()->data(m_mainView->model()->index(row, Duration)).toDouble();

    qreal start = m_mainView->model()->data(m_mainView->model()->index(row, StartTime)).toDouble() + startOffSet;
    QRectF lineRect(rect.x()+stepSize*start, rect.y()+vPadding, stepSize*duration, lineHeigth);

    QRectF fillRect (lineRect.x(),lineRect.y()+2,lineRect.width(),lineRect.height() - 4);
    QLinearGradient s_grad(lineRect.center().x(), lineRect.top(),
                           lineRect.center().x(), lineRect.bottom());
    if (selected) {
        s_grad.setColorAt(0, m_color.darker(150));
        s_grad.setColorAt(0.5, m_color.lighter(150));
        s_grad.setColorAt(1, m_color.darker(150));
        s_grad.setSpread(QGradient::ReflectSpread);
        painter->fillRect(fillRect, s_grad);
    }
    else {
        s_grad.setColorAt(0, m_color.darker(200));
        s_grad.setColorAt(0.5, m_color.lighter(125));
        s_grad.setColorAt(1, m_color.darker(200));
        s_grad.setSpread(QGradient::ReflectSpread);
        painter->fillRect(fillRect, s_grad);
    }
    QRect startRect(lineRect.x(), lineRect.y(), 3, lineRect.height());
    painter->fillRect(startRect, Qt::black);
    QRect endRect(lineRect.x() + lineRect.width(), lineRect.y(), 3, lineRect.height());
    painter->fillRect(endRect, Qt::black);
}

void KPrTimeLineView::paintTextRow(QPainter *painter, int x, int y, int row, int column, const int RowHeight)
{
    QRect rect(x,y,m_mainView->widthOfColumn(column), RowHeight);
    paintItemBackground(painter, rect,
                        row == m_mainView->selectedRow());
    painter->drawText(rect,
                      m_mainView->model()->data(m_mainView->model()->index(row,column)).toString(),
                      QTextOption(Qt::AlignCenter));
}

void KPrTimeLineView::paintIconRow(QPainter *painter, int x, int y, int row, int column, int iconSize, const int RowHeight)
{
    QRect rect(x,y,m_mainView->widthOfColumn(column), RowHeight);
    paintItemBackground(painter, rect,
                        row == m_mainView->selectedRow());
    QPixmap thumbnail =  (m_mainView->model()->data(m_mainView->model()->index(row,column), Qt::DecorationRole)).value<QPixmap>();
    thumbnail.scaled(iconSize, iconSize , Qt::KeepAspectRatio);
    int width = 0;
    int heigth = 0;
    if (thumbnail.width() > thumbnail.height()) {
        width = iconSize;
        heigth = width*thumbnail.height()/thumbnail.width();
    } else {
        heigth = iconSize;
        width = heigth*thumbnail.width()/thumbnail.height();
    }

    qreal centerX = (m_mainView->widthOfColumn(column) - width)/2;
    qreal centerY = (RowHeight-heigth)/2;
    QRectF target(rect.x()+centerX, rect.y()+centerY, width, heigth);
    painter->save();
    if (row == m_mainView->selectedRow()) {
        painter->setCompositionMode(QPainter::CompositionMode_ColorBurn);
    }
    painter->drawPixmap(target, thumbnail, thumbnail.rect());
    painter->restore();
}

double KPrTimeLineView::modD(double x, double y)
{
    int intPart = static_cast<int>(x / y);
    return x - static_cast<double>(intPart) * y;
}

void KPrTimeLineView::paintItemBackground(QPainter *painter, const QRect &rect, bool selected)
{
    QLinearGradient gradient(rect.center().x(), rect.top(),
                             rect.center().x(), rect.bottom());
    QColor color = palette().highlight().color();
    gradient.setColorAt(0, color.lighter(125));
    gradient.setColorAt(1, color);
    painter->fillRect(rect, selected ? gradient : palette().base());
    m_mainView->paintItemBorder(painter, palette(), rect);
    painter->setPen(selected ? palette().highlightedText().color()
                             : palette().windowText().color());
}
