/* This file is part of the KDE project
   Copyright (C) 2003, 2004 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
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

#include "kptrequestresourcespanel.h"
#include "kpttask.h"
#include "kptproject.h"
#include "kptresource.h"
#include "kptcalendar.h"

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kptcommand.h>

#include <qlistbox.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qspinbox.h>
#include <qvalidator.h>
#include <qcombobox.h>
#include <qdatetime.h>
#include <qdatetimeedit.h>

namespace KPlato
{

KPTResourceTableItem::KPTResourceTableItem(KPTResource *resource, KPTResourceRequest *request, bool check) {
    m_resource = resource;
    m_request = request;
    m_checked = check;
    m_origChecked = check;
    m_checkitem = 0;
    m_units = 100;
    m_origUnits = 100;
    //kdDebug()<<k_funcinfo<<"Added: '"<<resource->name()<<"' checked="<<m_checked<<endl;
}

KPTResourceTableItem::~KPTResourceTableItem() {
    //kdDebug()<<k_funcinfo<<m_resource->name()<<endl;
}

void KPTResourceTableItem::update() {
    if (m_checkitem)
        m_checked = m_checkitem->isChecked();

    m_checkitem = 0;
    //kdDebug()<<k_funcinfo<<m_resource->name()<<" checked="<<m_checked<<endl;
}

void KPTResourceTableItem::insert(QTable *table, int row) {
    //kdDebug()<<k_funcinfo<<endl;
    m_checkitem = new QCheckTableItem(table, m_resource->name());
    m_checkitem->setChecked(m_checked);
    table->setItem(row, 0, m_checkitem);
    //kdDebug()<<k_funcinfo<<"Added: '"<<m_resource->name()<<"' checked="<<m_checked<<endl;
}

KPTGroupLVItem::KPTGroupLVItem(QListView *parent, KPTResourceGroup *group, KPTTask &task)
    : QListViewItem(parent, group->name(), QString("%1").arg(group->units())),
      m_group(group),
      m_units(0)
{

    m_request = task.resourceGroupRequest(group);
    if (m_request) {
        m_units = m_request->units();
    }
    QPtrListIterator<KPTResource> it(group->resources());
    for (; it.current(); ++it) {
        //kdDebug()<<k_funcinfo<<"resource="<<it.current()->name()<<endl;
        KPTResourceRequest *req=0;
        if (m_request) {
            req = m_request->find(it.current());
        }
        m_resources.append(new KPTResourceTableItem(it.current(), req, (bool)req));
    }
    
    m_resources.setAutoDelete(true);
}

KPTGroupLVItem::~KPTGroupLVItem() {
    //kdDebug()<<k_funcinfo<<m_group->name()<<endl;
}

void KPTGroupLVItem::update() {
    QPtrListIterator<KPTResourceTableItem> it(m_resources);
    for (; it.current(); ++it) {
        it.current()->update();
    }
}

void KPTGroupLVItem::insert(QTable *table) {

    // clear the table, must be a better way!
    for (int i = table->numRows(); i > 0; --i)
        table->removeRow(i-1);

    if (m_group->numResources() == 0) {
        table->setNumRows(1);
        table->setItem(0, 0, new QCheckTableItem(table,i18n("None")));
    } else {
        table->setNumRows(m_group->numResources());
        QPtrListIterator<KPTResourceTableItem> it(m_resources);
        for (int i = 0; it.current(); ++it, ++i) {
            it.current()->insert(table, i);
        }
    }
    table->adjustColumn(0);
}

int KPTGroupLVItem::numRequests() {
    //kdDebug()<<k_funcinfo<<endl;
    int value = m_units;
    QPtrListIterator<KPTResourceTableItem> it(m_resources);
    for (; it.current(); ++it) {
        value += it.current()->numRequests();
    }
    return value;
}

bool KPTGroupLVItem::isNull() const {
    //kdDebug()<<k_funcinfo<<endl;
    QPtrListIterator<KPTResourceTableItem> it(m_resources);
    for (; it.current(); ++it) {
        if (it.current()->isChecked())
            return false;
    }
    if (m_units > 0)
        return false;
    return true;
}

KPTRequestResourcesPanel::KPTRequestResourcesPanel(QWidget *parent, KPTTask &task)
    : KPTTaskResourcesPanelBase(parent),
      m_task(task),
      m_worktime(0),
      selectedGroup(0),
      m_blockChanged(false) {

    KPTProject *p = dynamic_cast<KPTProject*>(task.projectNode());
    if (p) {
        m_worktime = p->standardWorktime();

        QPtrListIterator<KPTResourceGroup> git(p->resourceGroups());
        for(int i=0; git.current(); ++git, ++i) {
            KPTResourceGroup *grp = git.current();
            KPTGroupLVItem *grpitem = new KPTGroupLVItem(groupList, grp, task);
            groupList->insertItem(grpitem);
            //kdDebug()<<k_funcinfo<<" Added group: "<<grp->name()<<endl;
        }
    }
    QListViewItem *item = groupList->firstChild();
    if (item) {
        groupList->setSelected(item, true);
        groupChanged(item);
    }

    
    connect(groupList, SIGNAL(selectionChanged(QListViewItem*)),  SLOT(groupChanged(QListViewItem*)));
    connect(resourceTable, SIGNAL(valueChanged(int, int)), SLOT(resourceChanged(int, int)));
//    connect(numUnits, SIGNAL(valueChanged(int)), SLOT(unitsChanged(int)));

}

void KPTRequestResourcesPanel::groupChanged(QListViewItem *item) {
    //kdDebug()<<k_funcinfo<<endl;
    KPTGroupLVItem *grp = dynamic_cast<KPTGroupLVItem *>(item);
    if (grp == 0)
        return;

    if (selectedGroup) {
        selectedGroup->update();
    }
    selectedGroup = grp;

/*    m_blockChanged = true;
    numUnits->setMaxValue(grp->m_group->units());
    numUnits->setValue(grp->m_units);
    m_blockChanged = false;*/
    grp->insert(resourceTable);
}

void KPTRequestResourcesPanel::resourceChanged(int, int) {
    //kdDebug()<<k_funcinfo<<endl;
    sendChanged();
}

void KPTRequestResourcesPanel::unitsChanged(int units) {
    //kdDebug()<<k_funcinfo<<endl;
    if (selectedGroup) {
        selectedGroup->m_units = units;
        sendChanged();
    }
}

//FIXME
KCommand *KPTRequestResourcesPanel::buildCommand(KPTPart *part) {
    //kdDebug()<<k_funcinfo<<endl;
    KMacroCommand *cmd = 0;
    QListViewItem *item = groupList->firstChild();
    for (; item; item = item->nextSibling()) {
        KPTGroupLVItem *grp = static_cast<KPTGroupLVItem*>(item);
        QPtrListIterator<KPTResourceTableItem> it = grp->resources();
        for (; it.current(); ++it) {
            if (it.current()->isChecked() != it.current()->isOrigChecked()) {
                if (!cmd) cmd = new KMacroCommand("");
                if (it.current()->isChecked()) {
                    if (!grp->m_request) {
                        grp->m_request = new KPTResourceGroupRequest(grp->m_group, grp->m_units);
                        cmd->addCommand(new KPTAddResourceGroupRequestCmd(part, m_task, grp->m_request));
                    }
                    cmd->addCommand(new KPTAddResourceRequestCmd(part, grp->m_request, new KPTResourceRequest(it.current()->resource(), it.current()->units())));
                } else {
                    if (grp->m_request && it.current()->request()) {
                        cmd->addCommand(new KPTRemoveResourceRequestCmd(part, grp->m_request, it.current()->request()));
                        if (grp->isNull()) {
                            cmd->addCommand(new KPTRemoveResourceGroupRequestCmd(part, m_task, grp->m_request));
                        }
                    } else {
                        kdError()<<k_funcinfo<<"Remove failed"<<endl;
                    }
                }
            }
        }
    }
    return cmd;
}

void KPTRequestResourcesPanel::slotOk() {
    if (selectedGroup)
        selectedGroup->update();
}

void KPTRequestResourcesPanel::sendChanged() {
    if (!m_blockChanged) emit changed();
}

}  //KPlato namespace

#include "kptrequestresourcespanel.moc"
