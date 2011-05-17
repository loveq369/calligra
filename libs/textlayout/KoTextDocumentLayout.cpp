/* This file is part of the KDE project
 * Copyright (C) 2006-2007, 2009-2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010 Johannes Simon <johannes.simon@gmail.com>
 * Copyright (C) 2011 KO GmbH <cbo@kogmbh.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include "KoTextDocumentLayout.h"
#include "styles/KoParagraphStyle.h"
#include "styles/KoCharacterStyle.h"
#include "styles/KoListStyle.h"
#include "styles/KoStyleManager.h"
#include "KoTextBlockData.h"
#include "KoTextBlockBorderData.h"
#include "KoInlineTextObjectManager.h"
#include "KoTextLayoutRootArea.h"
#include "KoTextLayoutRootAreaProvider.h"
#include "KoTextLayoutObstruction.h"
#include "FrameIterator.h"
#include "InlineAnchorStrategy.h"
#include "FloatingAnchorStrategy.h"
#include "AnchorStrategy.h"

#include <KoTextAnchor.h>
#include <KoTextPage.h>
#include <KoInsets.h>
#include <KoPostscriptPaintDevice.h>
#include <KoShape.h>

#include <kdebug.h>
#include <QTextBlock>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextList>
#include <QTimer>
#include <QList>

extern int qt_defaultDpiY();


KoInlineObjectExtent::KoInlineObjectExtent(qreal ascent, qreal descent)
    : m_ascent(ascent),
      m_descent(descent)
{
}

class KoTextDocumentLayout::Private
{
public:
    Private(KoTextDocumentLayout *)
       : styleManager(0)
       , changeTracker(0)
       , inlineTextObjectManager(0)
       , provider(0)
       , anchoringIndex(0)
       , defaultTabSizing(0)
       , y(0)
       , isLayouting(false)
       , layoutScheduled(false)
       , continuousLayout(true)
       , layoutBlocked(false)
    {
    }
    KoStyleManager *styleManager;

    KoChangeTracker *changeTracker;

    KoInlineTextObjectManager *inlineTextObjectManager;
    KoTextLayoutRootAreaProvider *provider;
    KoPostscriptPaintDevice *paintDevice;
    QList<KoTextLayoutRootArea *> rootAreaList;
    FrameIterator *layoutPosition;

    QHash<int, KoInlineObjectExtent> inlineObjectExtents; // maps text-position to whole-line-height of an inline object
    int inlineObjectOffset;
    QList<KoTextAnchor *> textAnchors; // list of all inserted inline objects
    int anchoringIndex; // index of last not positioned inline object inside textAnchors
    int anchoringCycle; // how many times have we cycled in iterative mode;
    QRectF anchoringParagraphRect;

    QHash<KoShape*,KoTextLayoutObstruction*> anchoredObstructions; // all obstructions created in positionInlineObjects because KoTextAnchor from m_textAnchors is in text
    QList<KoTextLayoutObstruction*> freeObstructions; // obstructions affecting the current rootArea, and not anchored
    KoTextLayoutRootArea *anchoringRootArea;

    qreal defaultTabSizing;
    qreal y;
    bool isLayouting;
    bool layoutScheduled;
    bool continuousLayout;
    bool layoutBlocked;
    enum AnchoringState {
        AnchoringPreState
        ,AnchoringMovingState
        ,AnchoringFinalState
    };
    AnchoringState anchoringState;
};


// ------------------- KoTextDocumentLayout --------------------
KoTextDocumentLayout::KoTextDocumentLayout(QTextDocument *doc, KoTextLayoutRootAreaProvider *provider)
        : QAbstractTextDocumentLayout(doc),
        d(new Private(this))
{
    d->paintDevice = new KoPostscriptPaintDevice();
    d->provider = provider;
    setPaintDevice(d->paintDevice);

    d->styleManager = KoTextDocument(document()).styleManager();
    d->changeTracker = KoTextDocument(document()).changeTracker();
    d->inlineTextObjectManager = KoTextDocument(document()).inlineTextObjectManager();

    setTabSpacing(MM_TO_POINT(23)); // use same default as open office

    d->layoutPosition = new FrameIterator(doc->rootFrame());
}

KoTextDocumentLayout::~KoTextDocumentLayout()
{
    qDeleteAll(d->freeObstructions);
    qDeleteAll(d->anchoredObstructions);
    qDeleteAll(d->textAnchors);
    delete d;
}

KoTextLayoutRootAreaProvider *KoTextDocumentLayout::provider() const
{
    return d->provider;
}

bool KoTextDocumentLayout::relativeTabs() const
{
    return KoTextDocument(document()).relativeTabs();
}

KoInlineTextObjectManager *KoTextDocumentLayout::inlineTextObjectManager() const
{
    return d->inlineTextObjectManager;
}

KoChangeTracker *KoTextDocumentLayout::changeTracker() const
{
    return d->changeTracker;
}

KoStyleManager *KoTextDocumentLayout::styleManager() const
{
    return d->styleManager;
}

QRectF KoTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    QTextLayout *layout = block.layout();
    return layout->boundingRect();
}

QSizeF KoTextDocumentLayout::documentSize() const
{
    return QSizeF();
}

QRectF KoTextDocumentLayout::selectionBoundingBox(QTextCursor &cursor) const
{
    QRectF retval;
    foreach(const KoTextLayoutRootArea *rootArea, d->rootAreaList) {
        if (!rootArea->isDirty()) {
            QRectF areaBB  = rootArea->selectionBoundingBox(cursor);
            if (areaBB.isValid()) {
                retval |= areaBB;
            }
        }
    }
    return retval;
}


void KoTextDocumentLayout::draw(QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context)
{
    // WARNING Text shapes ask their root area directly to paint.
    // It saves a lot of extra traversal, that is quite costly for big
    // documents
    Q_UNUSED(painter);
    Q_UNUSED(context);
}


int KoTextDocumentLayout::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const
{
    Q_UNUSED(point);
    Q_UNUSED(accuracy);
    Q_ASSERT(false); //we should no longer call this method.
    // There is no need and is just slower than needed
    // call rootArea->hitTest() directly
    // root area is available through KoTextShapeData
    return -1;
}

int KoTextDocumentLayout::pageCount() const
{
    return 1;
}

void KoTextDocumentLayout::setTabSpacing(qreal spacing)
{
    d->defaultTabSizing = spacing;
}

qreal KoTextDocumentLayout::defaultTabSpacing()
{
    return d->defaultTabSizing;
}


void KoTextDocumentLayout::documentChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(charsAdded);
    Q_UNUSED(charsRemoved);

    int from = position;
    const int to = from + charsAdded;
    while (from < to) { // find blocks that have been added
        QTextBlock block = document()->findBlock(from);
        if (! block.isValid())
            break;
        if (from == block.position() && block.textList()) {
            KoTextBlockData *data = dynamic_cast<KoTextBlockData*>(block.userData());
            if (data)
                data->setCounterWidth(-1); // invalidate whole list.
        }

        from = block.position() + block.length();
    }

    // Mark the previous of the corresponding and all following root areas as dirty.
    KoTextLayoutRootArea *area = rootAreaForPosition(position);
    if (!area)
        return;
    for(int i = qMax(0, d->rootAreaList.indexOf(area) - 1); i < d->rootAreaList.count(); ++i)
        d->rootAreaList[i]->setDirty();

    emitLayoutIsDirty();
}

KoTextLayoutRootArea *KoTextDocumentLayout::rootAreaForPosition(int position) const
{
    QTextBlock block = document()->findBlock(position);
    if (!block.isValid())
        return 0;
    QTextLine line = block.layout()->lineForTextPosition(position - block.position());
    if (!line.isValid())
        return 0;

    foreach (KoTextLayoutRootArea *rootArea, d->rootAreaList) {
        QRectF rect = rootArea->boundingRect(); // should already be normalized()
        if (rect.width() <= 0.0 && rect.height() <= 0.0) // ignore the rootArea if it has a size of QSizeF(0,0)
            continue;
        QPointF pos = line.position();
        qreal x = pos.x();
        qreal y = pos.y();

        //0.125 needed since Qt Scribe works with fixed point
        if (x + 0.125 >= rect.x() && x<= rect.right() && y + 0.125 >= rect.y() && y <= rect.bottom()) {
            return rootArea;
        }
    }
    return 0;
}

void KoTextDocumentLayout::drawInlineObject(QPainter *painter, const QRectF &rect, QTextInlineObject object, int position, const QTextFormat &format)
{
    Q_ASSERT(format.isCharFormat());
    if (d->inlineTextObjectManager == 0)
        return;
    QTextCharFormat cf = format.toCharFormat();
    KoInlineObject *obj = d->inlineTextObjectManager->inlineTextObject(cf);
    if (obj)
        obj->paint(*painter, paintDevice(), document(), rect, object, position, cf);
}

void KoTextDocumentLayout::registerAnchoredObstruction(KoTextLayoutObstruction *obstruction)
{
    d->anchoredObstructions.insert(obstruction->shape(), obstruction);
}

void KoTextDocumentLayout::positionAnchoredObstructions()
{
    KoTextPage *page = d->anchoringRootArea->page();

    if (d->anchoringState == Private::AnchoringFinalState) {
        // In the final Layout run we do not try to move subjects
        return;
    }

    switch (3) {
    case 0:
        // For once-concurrently (20.172) we only layout once to place all subjects
        // and then again to flow text around.
        if (d->anchoringState == Private::AnchoringPreState) {
            return;
        }

        foreach (KoTextAnchor *textAnchor, d->textAnchors) {
            AnchorStrategy *strategy = static_cast<AnchorStrategy *>(textAnchor->anchorStrategy());

            strategy->setPageRect(page->rect());
            strategy->setPageNumber(page->pageNumber());

            strategy->moveSubject();
        }
        d->anchoringState = Private::AnchoringFinalState;
        d->anchoringRootArea->setDirty(); // make sure we do the layout to flow around
        break;
    case 1:
        // For once-successive (20.172) we layout once per anchor (and do not repeat an
        // anchor we have already done) to place subjects, and then again to flow text around
        // the last subject.
        break;
    case 2:
        // For iterative (20.172) we layout until no more movement is happening
        while (d->anchoringIndex < d->textAnchors.size()) {
            KoTextAnchor *textAnchor = d->textAnchors[d->anchoringIndex];
            AnchorStrategy *strategy = static_cast<AnchorStrategy *>(textAnchor->anchorStrategy());

            Q_ASSERT(page);
            strategy->setPageRect(page->rect());
            strategy->setPageNumber(page->pageNumber());

            if (strategy->moveSubject() == true) {
                return;
            }
            // move the index to next not positioned shape
            d->anchoringIndex++;
        }
        break;
    case 3: //experimental iterative mode
        // For iterative (20.172) we layout until no more movement is happening
        while (d->anchoringIndex < d->textAnchors.size()) {
            KoTextAnchor *textAnchor = d->textAnchors[d->anchoringIndex];
            AnchorStrategy *strategy = static_cast<AnchorStrategy *>(textAnchor->anchorStrategy());

            Q_ASSERT(page);
            strategy->setPageRect(page->rect());
            strategy->setPageNumber(page->pageNumber());

            if (strategy->moveSubject()) {
                d->anchoringState = Private::AnchoringMovingState;
                d->anchoringRootArea->setDirty(); // make sure we do the layout to flow around
            }
            // move the index to next not positioned shape
            d->anchoringIndex++;
        }
        break;
    }
}

void KoTextDocumentLayout::setAnchoringParagraphRect(const QRectF &paragraphRect)
{
    d->anchoringParagraphRect = paragraphRect;
}


// This method is called by qt every time  QTextLine.setWidth()/setNumColums() is called
void KoTextDocumentLayout::positionInlineObject(QTextInlineObject item, int position, const QTextFormat &format)
{
    //We are called before layout so that we can position objects
    Q_ASSERT(format.isCharFormat());
    if (d->inlineTextObjectManager == 0)
        return;
    QTextCharFormat cf = format.toCharFormat();
    KoInlineObject *obj = d->inlineTextObjectManager->inlineTextObject(cf);
    // We need some special treatment for anchors as they need to position their object during
    // layout and not this early
    KoTextAnchor *anchor = dynamic_cast<KoTextAnchor*>(obj);
    if (anchor) {
        // if there is no anchor strategy set then create one
        if (!anchor->anchorStrategy()) {
            //place anchored object far away, and let the layout position it later
            anchor->shape()->setPosition(QPointF(-100000, 0));
            if (anchor->behavesAsCharacter()) {
                anchor->setAnchorStrategy(new InlineAnchorStrategy(anchor, d->anchoringRootArea));
            } else {
                anchor->setAnchorStrategy(new FloatingAnchorStrategy(anchor, d->anchoringRootArea));
            }
            d->textAnchors.append(anchor);
            anchor->updatePosition(document(), item, position, cf);
        }
        static_cast<AnchorStrategy *>(anchor->anchorStrategy())->setParagraphRect(d->anchoringParagraphRect);
    }
    else if (obj) {
        obj->updatePosition(document(), item, position, cf);
    }
}

void KoTextDocumentLayout::beginAnchorCollecting(KoTextLayoutRootArea *rootArea)
{
    foreach(KoTextAnchor *anchor, d->textAnchors) {
        anchor->setAnchorStrategy(0);
    }

    qDeleteAll(d->anchoredObstructions);
    d->anchoredObstructions.clear();
    d->textAnchors.clear();

    d->anchoringIndex = 0;
    d->anchoringCycle = 0;
    d->anchoringRootArea = rootArea;
    d->anchoringState = Private::AnchoringPreState;
}

void KoTextDocumentLayout::resizeInlineObject(QTextInlineObject item, int position, const QTextFormat &format)
{
    Q_ASSERT(format.isCharFormat());
    if (d->inlineTextObjectManager == 0)
        return;
    QTextCharFormat cf = format.toCharFormat();
    KoInlineObject *obj = d->inlineTextObjectManager->inlineTextObject(cf);
    if (obj) {
        QTextDocument *doc = document();
        QVariant v;
        v.setValue(d->anchoringRootArea->page());
        doc->addResource(KoTextDocument::LayoutTextPage, QUrl("kotext://layoutTextPage"), v);
        obj->resize(doc, item, position, cf, paintDevice());
        registerInlineObject(item);
    }
}

void KoTextDocumentLayout::emitLayoutIsDirty()
{
    emit layoutIsDirty();
}

void KoTextDocumentLayout::layout()
{
    if (d->layoutBlocked) {
        return;
    }

    class LayoutState {
        public:
            LayoutState(KoTextDocumentLayout::Private *_d) : d(_d) {
                Q_ASSERT(!d->isLayouting);
                d->isLayouting = true;
            }
            ~LayoutState() {
                Q_ASSERT(d->isLayouting);
                d->isLayouting = false;
            }
        private:
            KoTextDocumentLayout::Private *d;
    };
    LayoutState layoutstate(d);

    delete d->layoutPosition;
    d->layoutPosition = new FrameIterator(document()->rootFrame());
    d->y = 0;
    d->layoutScheduled = false;
    KoTextLayoutRootArea *previousRootArea = 0;

    foreach (KoTextLayoutRootArea *rootArea, d->rootAreaList) {
        if (d->provider->suggestPageBreak(rootArea)) {
            d->provider->releaseAllAfter(previousRootArea);
            // We must also delete them from our own list too
            int newsize = d->rootAreaList.indexOf(previousRootArea) + 1;
            while (d->rootAreaList.size() > newsize) {
                d->rootAreaList.removeLast();
            }
            break;
        }

        bool shouldLayout = false;

        if (rootArea->top() != d->y) {
            shouldLayout = true;
        }
        else if (rootArea->isDirty()) {
            shouldLayout = true;
        }
        else if (!rootArea->isStartingAt(d->layoutPosition)) {
            shouldLayout = true;
        }

        if (shouldLayout) {
            QSizeF size = d->provider->suggestSize(rootArea);
            d->freeObstructions = d->provider->relevantObstructions(rootArea);

            rootArea->setReferenceRect(0, size.width(), d->y, d->y + size.height());

            beginAnchorCollecting(rootArea);

            // Layout all that can fit into that root area
            bool finished;
            FrameIterator *tmpPosition = 0;
            do {
                delete tmpPosition;
                tmpPosition = new FrameIterator(d->layoutPosition);
                finished = rootArea->layout(tmpPosition);
                if (3) { //FIXME
                    d->anchoringIndex = 0;
                    d->anchoringCycle++;
                    if (d->anchoringState == Private::AnchoringPreState || d->anchoringCycle > 10) {
                        d->anchoringState = Private::AnchoringFinalState;
                    } else {
                        d->anchoringState = Private::AnchoringPreState;
                    }
                }
            } while (rootArea->isDirty());
            delete d->layoutPosition;
            d->layoutPosition = tmpPosition;

            d->provider->doPostLayout(rootArea, false);

            if (finished) {
                d->provider->releaseAllAfter(rootArea);
                // We must also delete them from our own list too
                int newsize = d->rootAreaList.indexOf(rootArea) + 1;
                while (d->rootAreaList.size() > newsize) {
                    d->rootAreaList.removeLast();
                }
                emit finishedLayout();
                return;
            }

            if (!continuousLayout()) {
                return; // Let's take a break
            }
        } else {
            delete d->layoutPosition;
            d->layoutPosition = new FrameIterator(rootArea->nextStartOfArea());
            if (d->layoutPosition->it == document()->rootFrame()->end()) {
                Q_ASSERT(d->rootAreaList.last() == rootArea);
                return;
            }
        }
        d->y = rootArea->bottom() + qreal(50); // (post)Layout method(s) just set this
                                               // 50 just to seperate pages
        previousRootArea = rootArea;
    }

    while (d->layoutPosition->it != document()->rootFrame()->end()) {
        // Request a Root Area
        KoTextLayoutRootArea *rootArea = d->provider->provide(this);

        if (rootArea) {
            d->rootAreaList.append(rootArea);
            QSizeF size = d->provider->suggestSize(rootArea);
            d->freeObstructions = d->provider->relevantObstructions(rootArea);

            rootArea->setReferenceRect(0, size.width(), d->y, d->y + size.height());

            beginAnchorCollecting(rootArea);

            // Layout all that can fit into that root area
            FrameIterator *tmpPosition = 0;
            do {
                delete tmpPosition;
                tmpPosition = new FrameIterator(d->layoutPosition);
                rootArea->layout(tmpPosition);
                if (3) { //FIXME
                    d->anchoringIndex = 0;
                    d->anchoringCycle++;
                    if (d->anchoringState == Private::AnchoringPreState || d->anchoringCycle > 10) {
                        d->anchoringState = Private::AnchoringFinalState;
                    } else {
                        d->anchoringState = Private::AnchoringPreState;
                    }
                }
            } while (rootArea->isDirty());
            delete d->layoutPosition;
            d->layoutPosition = tmpPosition;

            d->provider->doPostLayout(rootArea, true);

            if (d->layoutPosition->it == document()->rootFrame()->end()) {
                break;
            }
            if (!continuousLayout()) {
                return; // let's take a break
            }
        } else {
            break; // with no more space there is nothing else we can do
        }
        d->y = rootArea->bottom() + qreal(50); // (post)Layout method(s) just set this
                                               // 50 just to seperate pages
    }

    emit finishedLayout();
}

void KoTextDocumentLayout::scheduleLayout()
{
    if (d->layoutScheduled) {
        return;
    }
    d->layoutScheduled = true;
    QTimer::singleShot(0, this, SLOT(executeScheduledLayout()));
}

void KoTextDocumentLayout::executeScheduledLayout()
{
    // Only do the actual layout if it wasn't done meanwhile by someone else.
    if (d->layoutScheduled) {
        d->layoutScheduled = false;
        if (!d->isLayouting)
            layout();
    }
}

bool KoTextDocumentLayout::continuousLayout()
{
    return d->continuousLayout;
}

void KoTextDocumentLayout::setContinuousLayout(bool continuous)
{
    d->continuousLayout = continuous;
}

void KoTextDocumentLayout::setBlockLayout(bool block)
{
    d->layoutBlocked = block;
}

bool KoTextDocumentLayout::layoutBlocked() const
{
    return d->layoutBlocked;
}

QRectF KoTextDocumentLayout::frameBoundingRect(QTextFrame*) const
{
    return QRectF();
}

void KoTextDocumentLayout::clearInlineObjectRegistry(QTextBlock block)
{
    d->inlineObjectExtents.clear();
    d->inlineObjectOffset = block.position();
}

void KoTextDocumentLayout::registerInlineObject(const QTextInlineObject &inlineObject)
{
    KoInlineObjectExtent pos(inlineObject.ascent(),inlineObject.descent());
    d->inlineObjectExtents.insert(d->inlineObjectOffset + inlineObject.textPosition(), pos);
}

KoInlineObjectExtent KoTextDocumentLayout::inlineObjectExtent(const QTextFragment &fragment)
{
    if (d->inlineObjectExtents.contains(fragment.position()))
        return d->inlineObjectExtents[fragment.position()];
    return KoInlineObjectExtent();
}

QList<KoTextLayoutObstruction *> KoTextDocumentLayout::currentObstructions()
{
    return d->freeObstructions + d->anchoredObstructions.values();
}

QList<KoTextLayoutRootArea *> KoTextDocumentLayout::rootAreas() const
{
    return d->rootAreaList;
}

void KoTextDocumentLayout::removeRootArea(KoTextLayoutRootArea *rootArea)
{
    int indexOf = rootArea ? qMax(0, d->rootAreaList.indexOf(rootArea)) : 0;
    for(int i = d->rootAreaList.count() - 1; i >= indexOf; --i)
        d->rootAreaList.removeAt(i);
}

QList<KoShape*> KoTextDocumentLayout::shapes() const
{
    QList<KoShape*> listOfShapes;
    foreach (KoTextLayoutRootArea *rootArea, d->rootAreaList) {
        if (rootArea->associatedShape())
            listOfShapes.append(rootArea->associatedShape());
    }
    return listOfShapes;
}

#include <KoTextDocumentLayout.moc>
