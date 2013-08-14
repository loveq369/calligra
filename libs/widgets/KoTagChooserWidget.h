/*
 *    This file is part of the KDE project
 *    Copyright (c) 2013 Sascha Suelzer <s.suelzer@gmail.com>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 */

#ifndef KOTAGCHOOSERWIDGET_H
#define KOTAGCHOOSERWIDGET_H

#include <QWidget>

class KoTagChooserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KoTagChooserWidget(QWidget* parent);
    virtual ~KoTagChooserWidget();
    void setCurrentIndex(int index);
    int findIndexOf(QString tagName);
    void insertItem(QString tagName);
    void insertItemAt(int index, QString tag);
    QString currentlySelectedTag();
    QStringList allTags();
    bool selectedTagIsReadOnly();
    void removeItem(QString item);
    void addItems(QStringList tagNames);
    void addReadOnlyItem(QString tagName);
    void clear();
    void setUndeletionCandidate(const QString &tag);

signals:
    void newTagRequested(const QString &tagname);
    void tagDeletionRequested(const QString &tagname);
    void tagRenamingRequested(const QString &oldTagname, const QString &newTagname);
    void tagUndeletionRequested(const QString &tagname);
    void tagUndeletionListPurgeRequested();
    void popupMenuAboutToShow();
    void tagChosen(const QString &tag);

private slots:
    void tagRenamingRequested(const QString &newName);
    void tagOptionsContextMenuAboutToShow();
    void contextDeleteCurrentTag();

private:
    /// pimpl because chooser will most likely get upgraded at some point
    class Private;
    Private* const d;

};

class KoTagToolButton : public QWidget
{
    Q_OBJECT

private:
    explicit KoTagToolButton(QWidget* parent = 0);
    virtual ~KoTagToolButton();
    void readOnlyMode(bool activate);
    void setUndeletionCandidate(const QString &deletedTagName);

signals:
    void newTagRequested(const QString &tagname);
    void renamingOfCurrentTagRequested(const QString &tagname);
    void deletionOfCurrentTagRequested();
    void undeletionOfTagRequested(const QString &tagname);
    void purgingOfTagUndeleteListRequested();
    void popupMenuAboutToShow();

private slots:
    void onTagUndeleteClicked();
    
private:
    class Private;
    Private* const d;
    friend class KoTagChooserWidget;
};

#endif // KOTAGCHOOSERWIDGET_H
