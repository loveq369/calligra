/*
 *  Copyright (c) 2007 Emanuele Tamponi <emanuele@valinor.it>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <qtest_kde.h>

#include "kis_illuminant_profile_test.h"
#include "kis_illuminant_profile.h"

#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

#include <iostream>
using namespace std;

void gsl_print(const gsl_matrix *M, const char *name)
{
    cout << name << endl;
    for (uint i = 0; i < M->size1; i++) {
        cout << "\t";
        for (uint j = 0; j < M->size2; j++) {
            if (gsl_matrix_get(M, i, j) >= 0)
                cout << " ";
            cout << gsl_matrix_get(M, i, j) << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void gsl_print(const gsl_vector *V, const char *name)
{
    cout << name << endl;
    cout << "\t";
    for (uint i = 0; i < V->size; i++) {
        if (gsl_vector_get(V, i) >= 0)
            cout << " ";
        cout << gsl_vector_get(V, i) << " ";
    }
    cout << endl;
}

void KisIlluminantProfileTest::testLoading()
{
    KGlobal::mainComponent().dirs()->addResourceType("illuminant_profiles", 0, "share/color/illuminants/");
    QString d653 = KGlobal::mainComponent().dirs()->findAllResources("illuminant_profiles", "D65_3.ill",  KStandardDirs::Recursive)[0];
    QString d659 = KGlobal::mainComponent().dirs()->findAllResources("illuminant_profiles", "D65_9.ill",  KStandardDirs::Recursive)[0];

    KisIlluminantProfile *p = new KisIlluminantProfile;
    QVERIFY(p->valid() == false);
    p->setFileName(d653);
    QVERIFY(p->valid() == false);
    p->load();
    QVERIFY(p->valid() == true);
//     QVERIFY(p->name() == "D65 3 Test Profile");
    qDebug() << "Profile name: " << p->name();

    QVERIFY(p->wavelenghts() == 3);
    QCOMPARE(p->Kblack(), 4.3);
    QCOMPARE(p->Sblack(), 0.14);
    gsl_print(p->T(), "Transformation matrix: ");
    gsl_print(p->P(), "Positions vector: ");

    KisIlluminantProfile *c = dynamic_cast<KisIlluminantProfile*>(p->clone());
    delete p;
    QVERIFY(c->valid() == true);
//     QVERIFY(c->name() == "D65 3 Test Profile");
    qDebug() << "Profile name: " << c->name();
    QVERIFY(c->wavelenghts() == 3);
    QCOMPARE(c->Kblack(), 4.3);
    QCOMPARE(c->Sblack(), 0.14);
    gsl_print(c->T(), "Transformation matrix: ");
    gsl_print(c->P(), "Positions vector: ");
    delete c;

    p = new KisIlluminantProfile(d659);
    QVERIFY(p->valid() == true);
//     QVERIFY(p->name() == "D65 9 Test Profile");
    qDebug() << "Profile name: " << p->name();

    QVERIFY(p->wavelenghts() == 9);
    QCOMPARE(p->Kblack(), 4.3);
    QCOMPARE(p->Sblack(), 0.14);
    gsl_print(p->T(), "Transformation matrix: ");
    gsl_print(p->P(), "Positions vector: ");
    delete p;
}

void KisIlluminantProfileTest::testSaving()
{
    QString d659 = KGlobal::mainComponent().dirs()->findAllResources("illuminant_profiles", "D65_9.ill",  KStandardDirs::Recursive)[0];

    KisIlluminantProfile *p = new KisIlluminantProfile(d659);
    p->save("D659Save.ill");
    delete p;
    p = new KisIlluminantProfile("D659Save.ill");

    QVERIFY(p->valid() == true);
//     QVERIFY(p->name() == "D65 9 Test Profile");
    qDebug() << "Profile name: " << p->name();

    QVERIFY(p->wavelenghts() == 9);
    QCOMPARE(p->Kblack(), 4.3);
    QCOMPARE(p->Sblack(), 0.14);
    gsl_print(p->T(), "Transformation matrix: ");
    gsl_print(p->P(), "Positions vector: ");
    delete p;
}

QTEST_KDEMAIN(KisIlluminantProfileTest, NoGUI)
#include "kis_illuminant_profile_test.moc"
