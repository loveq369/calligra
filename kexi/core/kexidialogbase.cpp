/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Joseph Wenninger <jowenn@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#include "kexidialogbase.h"
#include "kexiapplication.h"
#include "keximainwindow.h"

#include <kdockwidget.h>
#include <kiconloader.h>
#include <netwm_def.h>
#include <qtimer.h>
#include <kdebug.h>

KexiDialogBase *KexiDialogBase::s_activeDocumentWindow=0;
KexiDialogBase *KexiDialogBase::s_activeToolWindow=0;
QPtrList<KexiDialogBase> *KexiDialogBase::s_DocumentWindows=0;
QPtrList<KexiDialogBase> *KexiDialogBase::s_ToolWindows=0;

KexiDialogBase::KexiDialogBase(QWidget *parent, const char *name) : QWidget(parent, name)
{
	if (s_DocumentWindows==0) s_DocumentWindows=new QPtrList<KexiDialogBase>();
	if (s_ToolWindows==0) s_ToolWindows=new QPtrList<KexiDialogBase>();
	m_mainWindow=(parent==0)?((KexiApplication*)kapp)->mainWindow():
			(parent->qt_cast("KexiMainWindow")==0)?
			((KexiApplication*)kapp)->mainWindow():
			static_cast<KexiMainWindow*>(parent->qt_cast("KexiMainWindow"));
	myDock=0;
//	myDock->toDesktop();
//	myDock->undock();
}


void KexiDialogBase::registerAs(KexiDialogBase::WindowType wt)
{
	myDock=m_mainWindow->createDockWidget( "Widget", 
		(icon()?(*(icon())):SmallIcon("kexi")), 0, caption());
	myDock->setWidget(this);
	myDock->setEnableDocking(KDockWidget::DockFullDocking);
	myDock->setDockSite(KDockWidget::DockFullDocking);
	myDock->setDockWindowType(NET::Normal);
	myDock->setDockWindowTransient(m_mainWindow,true);

	if (wt==DocumentWindow) {
		if ((s_activeDocumentWindow==0) || (s_activeDocumentWindow->myDock==0)) {
			if (m_mainWindow->windowMode()==KexiMainWindow::SingleWindowMode)
				myDock->manualDock(m_mainWindow->getMainDockWidget(),KDockWidget::DockTop, 100);
			else
				myDock->toDesktop();

		}
		else
			myDock->manualDock(s_activeDocumentWindow->myDock,KDockWidget::DockCenter);
		s_DocumentWindows->insert(0,this);
		s_activeDocumentWindow=this;
	}
	else {
		if ((s_activeToolWindow==0) || (s_activeToolWindow->myDock==0)) {
			if (m_mainWindow->windowMode()==KexiMainWindow::SingleWindowMode)
				myDock->manualDock(m_mainWindow->getMainDockWidget(),KDockWidget::DockLeft, 20);
			else
				myDock->toDesktop();
		}
		else
			myDock->manualDock(s_activeToolWindow->myDock,KDockWidget::DockCenter);
		s_ToolWindows->insert(0,this);
		s_activeToolWindow=this;
	}
	
	myDock->makeDockVisible();
}

void KexiDialogBase::focusInEvent ( QFocusEvent *)
{
kdDebug()<<"FocusInEvent";

}
void KexiDialogBase::closeEvent(QCloseEvent *ev)
{
	emit closing(this);
//	close();
	ev->accept();
}

KexiDialogBase::~KexiDialogBase()
{
}

void KexiDialogBase::activateActions(){;}

void KexiDialogBase::deactivateActions(){;}

#include "kexidialogbase.moc"
