/*
 * This file is part of the KDE project
 *
 * Copyright (C) 2013 Shantanu Tushar <shantanu@kde.org>
 * Copyright (C) 2013 Sujith Haridasan <sujith.h@gmail.com>
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
 *
 */

#ifndef CQTEXTDOCUMENTCANVAS_H
#define CQTEXTDOCUMENTCANVAS_H

#include <QtDeclarative/qdeclarativeitem.h>

#include <KoZoomMode.h>

class CQTextDocumentModel;
class KoFindText;
class KoDocument;
class KoZoomController;
class KoCanvasController;
class KoCanvasBase;
class KUrl;
class KoFindMatch;

class CQTextDocumentCanvas : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode WRITE setZoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(QObject* documentModel READ documentModel NOTIFY documentModelChanged)
    Q_PROPERTY(QSize documentSize READ documentSize NOTIFY documentSizeChanged)
    Q_ENUMS(ZoomMode)

public:
    CQTextDocumentCanvas();
    ~CQTextDocumentCanvas();

    enum ZoomMode
    {
        ZOOM_CONSTANT = 0,  ///< zoom x %
        ZOOM_WIDTH    = 1,  ///< zoom pagewidth
        ZOOM_PAGE     = 2,  ///< zoom to pagesize
    };

    QString source() const;
    void setSource(const QString &source);

    ZoomMode zoomMode() const;
    void setZoomMode(ZoomMode zoomMode);

    QString searchTerm() const;
    void setSearchTerm(const QString &term);

    QObject *documentModel() const;
    QSize documentSize() const;

signals:
    void sourceChanged();
    void zoomModeChanged();
    void searchTermChanged();
    void documentModelChanged();
    void documentSizeChanged();

protected:
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);

private slots:
    void updateControllerWithZoomMode();
    void findNoMatchFound();
    void findMatchFound(const KoFindMatch& match);
    void updateCanvas();
    void findPrevious();
    void findNext();
    void updateDocumentSize(const QSize &size);

private:
    bool openFile(const QString& uri);
    void createAndSetCanvasControllerOn(KoCanvasBase *canvas);
    void createAndSetZoomController(KoCanvasBase *canvas);
    void updateZoomControllerAccordingToDocument(const KoDocument *document);

    QString m_source;
    KoCanvasBase *m_canvasBase;
    KoCanvasController *m_canvasController;
    KoZoomController *m_zoomController;
    ZoomMode m_zoomMode;
    QString m_searchTerm;
    KoFindText *m_findText;
    CQTextDocumentModel *m_documentModel;
    QSize m_documentSize;
};
#endif // CQTEXTDOCUMENTCANVAS_H