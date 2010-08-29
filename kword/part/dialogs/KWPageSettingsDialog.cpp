/* This file is part of the KDE project
 * Copyright (C) 2007, 2010 Thomas Zander <zander@kde.org>
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

#include "KWPageSettingsDialog.h"
#include "KWDocumentColumns.h"

#include <KWDocument.h>
#include <commands/KWPagePropertiesCommand.h>

//#include <KDebug>

KWPageSettingsDialog::KWPageSettingsDialog(QWidget *parent, KWDocument *document, const KWPage &page)
        : KoPageLayoutDialog(parent, page.pageStyle().pageLayout()),
        m_document(document),
        m_page(page)
{
    Q_ASSERT(document);
    Q_ASSERT(page.isValid());
    m_columns = new KWDocumentColumns(this, m_page.pageStyle().columns());
    addPage(m_columns, i18n("Columns"));
    showPageSpread(true);
    showTextDirection(true); // TODO can we hide this in selected usecases? Use the resource manager bidi-check maybe?
    //showApplyToDocument(true); // TODO uncommand when we can handle it.

    bool simpleSetup = document->pageCount() == 1
            || (document->pageCount() == 2 && page.pageSide() == KWPage::PageSpread);
    if (!simpleSetup) { // if there is one style, its still a simple doc
        bool onlyOneStyle = true;
        foreach (const KWPage &p, document->pageManager()->pages()) {
            if (p.pageStyle() != m_page.pageStyle()) {
                onlyOneStyle = false;
                break;
            }
        }
        if (onlyOneStyle)
            simpleSetup = true;
    }

    if (simpleSetup) {
        /*
          Simple setup means we have currently exactly one page style; so the common usecase
          is that that one will be changed, and it will apply to all pages in the document.
          Lets make that usecase as simple and as straight forward as possible

          Notice that if the user unchecks the "apply to document' checkbox we will automatically
          switch to the not simple setup and use page styles.
         */
        KWPageStyle pageStyle = m_page.pageStyle();
        setPageSpread(pageStyle.isPageSpread());
        setTextDirection(pageStyle.direction());
    }
    else {
        /*
          The document is already not simple anymore; there is more than one page style
          in use. We should show that fact and the user is allowed to use the power of
          page styles.
         */
        setPageSpread(m_page.pageSide() == KWPage::PageSpread);
        setTextDirection(m_page.directionHint());

        /* TODO
        fill the styles combobox and please be smart about it with an auto-generated
        style being named after the pages its assigned to.
        Make sure the current page style is the first entry.

        */
    }
}

void KWPageSettingsDialog::accept()
{
    // TODO rename 'applyToDocument' to 'onlyThisPage'
    if (!applyToDocument()) {
        /* TODO
            use new command to create a new page style
           clone the current page' page style.
           use new command to change a page to get a new style. (i.e. setPageStyle())
        */
    }

    KoText::Direction newDir = textDirection();
    KoPageLayout lay = pageLayout();
    if (lay.pageEdge >= 0 || lay.bindingSide >= 0) {
        // asserts check if our super didn't somehow mess up
        Q_ASSERT(lay.pageEdge >= 0);
        Q_ASSERT(lay.bindingSide >= 0);
        Q_ASSERT(lay.leftMargin == -1);
        Q_ASSERT(lay.rightMargin == -1);

        // its a page spread, which kword can handle, so we can safely set the
        // normal page size and assume that the page object will do the right thing
        lay.width /= (qreal) 2;
    }
    KWPagePropertiesCommand *cmd = new KWPagePropertiesCommand(m_document, m_page,
            lay, newDir, m_columns->columns());
    m_document->addCommand(cmd);

    KoPageLayoutDialog::accept();
}

void KWPageSettingsDialog::reject()
{
    KoPageLayoutDialog::reject();
}
