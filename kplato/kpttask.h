/* This file is part of the KDE project
   Copyright (C) 2001 Thomas Zander zander@kde.org

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kpttask_h
#define kpttask_h

#include "kptduration.h" 
#include "kptnode.h"
#include "kptresource.h"
#include "defs.h"

class KPTResource;

/** 
  * A task in the scheduling software is represented by this class. A task
  * can be anything from 'build house' to 'drill hole' It will always mean
  * an activity.
  */
class KPTTask : public KPTNode {
public:
    KPTTask();
    ~KPTTask();

    /**
     * The expected Duration is the expected time to complete a Task, Project,
     * etc. For an individual Task, this will calculate the expected duration
     * by querying the Distribution of the Task. If the Distribution is a
     * simple RiskNone, the value will equal the mode Duration, but for other
     * Distributions like RiskHigh, the value will have to be calculated. For
     * a Project or Subproject, the expected Duration is calculated by
     * PERT/CPM. 
     */
    KPTDuration *getExpectedDuration();

    /**
     * Instead of using the expected duration, generate a random value using
     * the Distribution of each Task. This can be used for Monte-Carlo
     * estimation of Project duration.
     */
    KPTDuration *getRandomDuration();

    /**
     * Retrive the time this node starts. This is either implied from the set
     * time, or calculated by asking the parents.
     */
    KPTDuration *getStartTime();

    /**
     * Retrieve the calculated float of this node
     */
    KPTDuration *getFloat();

    // resources management
    /**
     * The resources are provided as a list, each having an associated risk.
     * we return the resource here which has a pointer to the risk
     */
    const QPtrList<KPTResourceGroup> &resourceIterator() const
	{ return m_resource; }
    virtual void addResource(KPTResourceGroup *resource);
    virtual void insertResource(unsigned int index,
				KPTResourceGroup *resource);
    void removeResource(KPTResourceGroup *resource);
    void removeResource(int number);

    /**
     * TODO: Load and save
     */
    virtual bool load(QDomElement &element) { return true; }
    virtual void save(QDomElement &element) const { }

private:
    QPtrList<KPTResourceGroup> m_resource;
};
#endif
