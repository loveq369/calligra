/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qsplitter.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>

#include <kexidb/connection.h>
#include <kexidb/parser/parser.h>

#include <kexiproject.h>
#include <keximainwindow.h>

#include "kexiquerydesigner.h"
#include "kexiquerydesignersqleditor.h"
#include "kexiquerydesignersqlhistory.h"
#include "kexiquerydesignersql.h"
#include "kexiquerypart.h"

#include "kexisectionheader.h"


static bool compareSQL(const QString& sql1, const QString& sql2)
{
	//TODO: use reformatting functions here
	return sql1.stripWhiteSpace()==sql2.stripWhiteSpace();
}

//===================

class KexiQueryDesignerSQLViewPrivate
{
	public:
		KexiQueryDesignerSQLViewPrivate() :
		 statusPixmapOk( DesktopIcon("button_ok") )
		 , history(0)
		 , historyHead(0)
		 , statusPixmapErr( DesktopIcon("button_cancel") )
		 , statusPixmapInfo( DesktopIcon("messagebox_info") )
		 , heightForStatusMode(-1)
		 , heightForHistoryMode(-1)
		 , parsedQuery(0)
		 , eventFilterForSplitterEnabled(true)
		{
		}
		KexiQueryDesignerSQLEditor *editor;
		KexiQueryDesignerSQLHistory *history;
		QLabel *pixmapStatus, *lblStatus;
		QHBox *status_hbox;
		QVBox *history_section;
		KexiSectionHeader *head, *historyHead;
		QPixmap statusPixmapOk, statusPixmapErr, statusPixmapInfo;
		QSplitter *splitter;
		KToggleAction *action_toggle_history;
		//! For internal use, this pointer is usually copied to TempData structure, 
		//! when switching out of this view (then it's cleared).
		KexiDB::QuerySchema *parsedQuery;
		//! For internal use, statement passed in switching to this view
		QString origStatement;
		//! needed to remember height for both modes, beteen switching
		int heightForStatusMode, heightForHistoryMode;
		//! helper for slotUpdateMode()
		bool action_toggle_history_was_checked : 1;
		//! helper for eventFilter()
		bool eventFilterForSplitterEnabled : 1;
};

//===================

KexiQueryDesignerSQLView::KexiQueryDesignerSQLView(KexiMainWindow *mainWin, QWidget *parent, const char *name)
 : KexiViewBase(mainWin, parent, name)
 , d( new KexiQueryDesignerSQLViewPrivate() )
{
	d->splitter = new QSplitter(this);
	d->splitter->setOrientation(Vertical);
	d->head = new KexiSectionHeader(i18n("SQL Query Text"), Vertical, d->splitter);
	d->editor = new KexiQueryDesignerSQLEditor(mainWin, d->head, "sqle");
	connect(d->editor, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
	addChildView(d->editor);
	setViewWidget(d->editor);
	d->splitter->setFocusProxy(d->editor);
	setFocusProxy(d->editor);

	d->history_section = new QVBox(d->splitter);

	d->status_hbox = new QHBox(d->history_section);
	d->status_hbox->installEventFilter(this);
	d->splitter->setResizeMode(d->history_section, QSplitter::KeepSize);
	d->status_hbox->setSpacing(0);
	d->pixmapStatus = new QLabel(d->status_hbox);
	d->pixmapStatus->setFixedWidth(d->statusPixmapOk.width()*3/2);
	d->pixmapStatus->setAlignment(AlignHCenter | AlignTop);
	d->pixmapStatus->setMargin(d->statusPixmapOk.width()/4);
	d->pixmapStatus->setPaletteBackgroundColor( palette().active().color(QColorGroup::Base) );

	d->lblStatus = new QLabel(d->status_hbox);
	d->lblStatus->setAlignment(AlignLeft | AlignTop);
	d->lblStatus->setMargin(d->statusPixmapOk.width()/4);
	d->lblStatus->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
	d->lblStatus->resize(d->lblStatus->width(),d->statusPixmapOk.width()*3);
	d->lblStatus->setPaletteBackgroundColor( palette().active().color(QColorGroup::Base) );

	QHBoxLayout *b = new QHBoxLayout(this);
	b->addWidget(d->splitter);

	plugSharedAction("querypart_check_query", this, SLOT(slotCheckQuery())); 
	plugSharedAction("querypart_view_toggle_history", this, SLOT(slotUpdateMode())); 
	d->action_toggle_history = static_cast<KToggleAction*>( sharedAction( "querypart_view_toggle_history" ) );

	d->historyHead = new KexiSectionHeader(i18n("SQL Query History"), Vertical, d->history_section);
	d->historyHead->installEventFilter(this);
	d->history = new KexiQueryDesignerSQLHistory(d->historyHead, "sql_history");

	static const QString msg_back = i18n("Back to selected query");
	static const QString msg_clear = i18n("Clear history");
	d->historyHead->addButton("select_item", msg_back, this, SLOT(slotSelectQuery()));
	d->historyHead->addButton("editclear", msg_clear, d->history, SLOT(clear()));
	d->history->popupMenu()->insertItem(SmallIcon("select_item"), msg_back, this, SLOT(slotSelectQuery()));
	d->history->popupMenu()->insertItem(SmallIcon("editclear"), msg_clear, d->history, SLOT(clear()));

	d->heightForHistoryMode = -1; //height() / 2;
	//d->historyHead->hide();
	d->action_toggle_history_was_checked = !d->action_toggle_history->isChecked(); //to force update
	slotUpdateMode();
	slotCheckQuery();
}

KexiQueryDesignerSQLView::~KexiQueryDesignerSQLView()
{
	delete d;
}

KexiQueryDesignerSQLEditor *KexiQueryDesignerSQLView::editor() const
{
	return d->editor;
}

void KexiQueryDesignerSQLView::setStatusOk()
{
	d->pixmapStatus->setPixmap(d->statusPixmapOk);
	setStatusText("<h2>"+i18n("The query is correct")+"</h2>");
	d->history->addEvent(d->editor->text().stripWhiteSpace(), true, QString::null);
}

void KexiQueryDesignerSQLView::setStatusError(const QString& msg)
{
	d->pixmapStatus->setPixmap(d->statusPixmapErr);
	setStatusText("<h2>"+i18n("The query is incorrect")+"</h2><p>"+msg+"</p>");
	d->history->addEvent(d->editor->text().stripWhiteSpace(), false, msg);
}

void KexiQueryDesignerSQLView::setStatusEmpty()
{
	d->pixmapStatus->setPixmap(d->statusPixmapInfo);
	setStatusText(i18n("Please enter your query and execute \"Check query\" function to verify it."));
}

void KexiQueryDesignerSQLView::setStatusText(const QString& text)
{
	if (!d->action_toggle_history->isChecked()) {
		QSimpleRichText rt(text, d->lblStatus->font());
		rt.setWidth(d->lblStatus->width());
		QValueList<int> sz = d->splitter->sizes();
		const int newHeight = rt.height()+d->lblStatus->margin()*2;
		if (sz[1]<newHeight) {
			sz[1] = newHeight;
			d->splitter->setSizes(sz);
		}
		d->lblStatus->setText(text);
	}
}

bool
KexiQueryDesignerSQLView::beforeSwitchTo(int mode, bool &cancelled, bool &dontStore)
{
//TODO
	dontStore = true;
	if (mode==Kexi::DesignViewMode || mode==Kexi::DataViewMode) {
		QString sqlText = d->editor->text().stripWhiteSpace();
		KexiQueryPart::TempData * temp = tempData();
		if (sqlText.isEmpty()) {
			//special case: empty SQL text
			if (temp->query) {
				temp->queryChangedInPreviousView = true; //query changed
				delete temp->query; //safe?
				temp->query = 0;
			}
		}
		else {
			if (compareSQL(d->origStatement, d->editor->text())) {
				//statement unchanged! - nothing to do
				temp->queryChangedInPreviousView = false;
			}
			else {
				//parse SQL text
				if (!slotCheckQuery()) {
					if (KMessageBox::warningYesNo(this, "<p>"+i18n("The query you entered is incorrect.")
						+"</p><p>"+i18n("Do you want to cancel any changes made to this SQL text?")+"</p>"
						+"</p><p>"+i18n("Answering \"No\" allows you to make corrections.")+"</p>")==KMessageBox::No)
					{
						cancelled = true;
						return false;
					}
					else {
						//do not change original query
						temp->queryChangedInPreviousView = false;
						return true;
					}
				}
				//replace old query schema with new one
				delete temp->query; //safe?
				temp->query = d->parsedQuery;
				d->parsedQuery = 0;
				temp->queryChangedInPreviousView = true;
			}
		}
	}

	//TODO
	/*
	if (d->doc) {
		KexiDB::Parser *parser = new KexiDB::Parser(mainWin()->project()->dbConnection());
		parser->parse(getQuery());
		d->doc->setSchema(parser->select());

		if(parser->operation() == KexiDB::Parser::OP_Error)
		{
			d->history->addEvent(getQuery(), false, parser->error().error());
			kdDebug() << "KexiQueryDesignerSQLView::beforeSwitchTo(): syntax error!" << endl;
			return false;
		}
		delete parser;
	}

	setDirty(true);*/
	return true;
}

bool
KexiQueryDesignerSQLView::afterSwitchFrom(int mode, bool &cancelled)
{
	kdDebug() << "KexiQueryDesignerSQLView::afterSwitchFrom()" << endl;
	if (mode==Kexi::DesignViewMode || mode==Kexi::DataViewMode) {
		KexiQueryPart::TempData * temp = tempData();
		if (!temp->query) {
			//TODO msg
			return false;
		}
		d->origStatement = mainWin()->project()->dbConnection()->selectStatement( *temp->query ).stripWhiteSpace();
		d->editor->setText( d->origStatement );
	}

/*	if (d->doc && d->doc->schema()) {
		d->editor->setText(d->doc->schema()->connection()->selectStatement(*d->doc->schema()));
	}*/
	return true;
}

QString
KexiQueryDesignerSQLView::sqlText() const
{
	return d->editor->text();
}

bool KexiQueryDesignerSQLView::slotCheckQuery()
{
	QString sqlText = d->editor->text().stripWhiteSpace();
	if (sqlText.isEmpty()) {
		delete d->parsedQuery;
		d->parsedQuery = 0;
		setStatusEmpty();
		return true;
	}

	kdDebug() << "KexiQueryDesignerSQLView::slotCheckQuery()" << endl;
	KexiQueryPart::TempData * temp = tempData();
	KexiDB::Parser *parser = mainWin()->project()->sqlParser();
	parser->parse( sqlText );
	delete d->parsedQuery;
	d->parsedQuery = parser->query();
	if (!d->parsedQuery || !parser->error().type().isEmpty()) {
		KexiDB::ParserError err = parser->error();
		setStatusError(err.error());
		d->editor->jump(err.at());
		delete d->parsedQuery;
		d->parsedQuery = 0;
		return false;
	}

	setStatusOk();
	return true;
}

void KexiQueryDesignerSQLView::slotUpdateMode()
{
	if (d->action_toggle_history->isChecked() == d->action_toggle_history_was_checked)
		return;

	d->eventFilterForSplitterEnabled = false;

	QValueList<int> sz = d->splitter->sizes();
	d->action_toggle_history_was_checked = d->action_toggle_history->isChecked();
	int heightToSet = -1;
	if (d->action_toggle_history->isChecked()) {
		d->status_hbox->hide();
		d->historyHead->show();
		d->history->show();
		if (d->heightForHistoryMode==-1)
			d->heightForHistoryMode = m_dialog->height() / 2;
		heightToSet = d->heightForHistoryMode;
		d->heightForStatusMode = sz[1]; //remember
	}
	else {
		if (d->historyHead)
			d->historyHead->hide();
		d->status_hbox->show();
		if (d->heightForStatusMode>=0) {
			heightToSet = d->heightForStatusMode;
		} else {
			d->heightForStatusMode = d->status_hbox->height();
		}
		if (d->heightForHistoryMode>=0)
			d->heightForHistoryMode = sz[1];
	}
	
	if (heightToSet>=0) {
		sz[1] = heightToSet;
		d->splitter->setSizes(sz);
	}
	d->eventFilterForSplitterEnabled = true;
	slotCheckQuery();
}

void KexiQueryDesignerSQLView::slotTextChanged()
{
	setDirty(true);
	setStatusEmpty();
}

bool KexiQueryDesignerSQLView::eventFilter( QObject *o, QEvent *e )
{
	if (d->eventFilterForSplitterEnabled) {
		if (e->type()==QEvent::Resize && o && o==d->historyHead && d->historyHead->isVisible()) {
			d->heightForHistoryMode = d->historyHead->height();
		}
		else if (e->type()==QEvent::Resize && o && o==d->status_hbox && d->status_hbox->isVisible()) {
			d->heightForStatusMode = d->status_hbox->height();
		}
	}
	return KexiViewBase::eventFilter(o, e);
}

void KexiQueryDesignerSQLView::updateActions(bool activated)
{
	if (activated) {
		slotUpdateMode();
	}
	KexiViewBase::updateActions(activated);
}

void KexiQueryDesignerSQLView::slotSelectQuery()
{
	QString sql = d->history->selectedStatement();
	if (!sql.isEmpty()) {
		d->editor->setText( sql );
	}
}

KexiQueryPart::TempData *
KexiQueryDesignerSQLView::tempData() const
{	
	return static_cast<KexiQueryPart::TempData*>(parentDialog()->tempData());
}

/*void KexiQueryDesignerSQLView::slotHistoryHeaderButtonClicked(const QString& buttonIdentifier)
{
	if (buttonIdentifier=="select_query") {
		slotSelectQuery();
	}
	else if (buttonIdentifier=="clear_history") {
		d->history->clear();
	}
}*/

#include "kexiquerydesignersql.moc"

