/* This file is part of the KDE project
   Copyright 2007 Ariya Hidayat <ariya@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <math.h>

#include "qtest_kde.h"

#include <Formula.h>
#include <Value.h>

#include "TestMathFunctions.h"

#include <float.h> // DBL_EPSILON

using namespace KSpread;

// NOTE: we do not compare the numbers _exactly_ because it is difficult
// to get one "true correct" expected values for the functions due to:
//  - different algorithms among spreadsheet programs
//  - precision limitation of floating-point number representation
//  - accuracy problem due to propagated error in the implementation
#define CHECK_EVAL(x,y) QCOMPARE(evaluate(x),RoundNumber(y))

// round to get at most 15-digits number
static Value RoundNumber(double f)
{
  return Value( QString::number(f, 'g', 15) );
}

// round to get at most 15-digits number
static Value RoundNumber(const Value& v)
{
  if(v.isNumber())
  {
    double d = v.asFloat();
    if(fabs(d) < DBL_EPSILON)
      d = 0.0;
    return Value( QString::number(d, 'g', 15) );
  }
  else
    return v;  
}

Value TestMathFunctions::evaluate(const QString& formula)
{
  Formula f;
  QString expr = formula;
  if ( expr[0] != '=' )
    expr.prepend( '=' );
  f.setExpression( expr );
  Value result = f.eval();

#if 0
  // this magically generates the CHECKs
  printf("  CHECK_EVAL( \"%s\",  %15g) );\n", qPrintable(formula), result.asFloat());
#endif

  return RoundNumber(result);
}
namespace QTest 
{
  template<>
  char *toString(const Value& value)
  {
    QString message;
    QTextStream ts( &message, QIODevice::WriteOnly );
    if( value.isFloat() )
      ts << QString::number(value.asFloat(), 'g', 20);
    else  
      ts << value;
    return qstrdup(message.toLatin1());
  }
}

void TestMathFunctions::testABS()
{
  CHECK_EVAL( "ABS(0)", 0 );
  CHECK_EVAL( "ABS(-1)", 1 );
  CHECK_EVAL( "ABS(-2)", 2 );
  CHECK_EVAL( "ABS(-3)", 3 );
  CHECK_EVAL( "ABS(-4)", 4 );
  CHECK_EVAL( "ABS(1)", 1 );
  CHECK_EVAL( "ABS(2)", 2 );
  CHECK_EVAL( "ABS(3)", 3 );
  CHECK_EVAL( "ABS(4)", 4 );
  
  CHECK_EVAL( "ABS(1/0)", Value::errorDIV0() );

}

void TestMathFunctions::testCEIL()
{
  CHECK_EVAL( "CEIL(0)", 0 );
  
  CHECK_EVAL( "CEIL(0.1)", 1 );
  CHECK_EVAL( "CEIL(0.01)", 1 );
  CHECK_EVAL( "CEIL(0.001)", 1 );
  CHECK_EVAL( "CEIL(0.0001)", 1 );
  CHECK_EVAL( "CEIL(0.00001)", 1 );
  CHECK_EVAL( "CEIL(0.000001)", 1 );
  CHECK_EVAL( "CEIL(0.0000001)", 1 );
  
  CHECK_EVAL( "CEIL(1.1)", 2 );
  CHECK_EVAL( "CEIL(1.01)", 2 );
  CHECK_EVAL( "CEIL(1.001)", 2 );
  CHECK_EVAL( "CEIL(1.0001)", 2 );
  CHECK_EVAL( "CEIL(1.00001)", 2 );
  CHECK_EVAL( "CEIL(1.000001)", 2 );
  CHECK_EVAL( "CEIL(1.0000001)", 2 );

  CHECK_EVAL( "CEIL(-0.1)", 0 );
  CHECK_EVAL( "CEIL(-0.01)", 0 );
  CHECK_EVAL( "CEIL(-0.001)", 0 );
  CHECK_EVAL( "CEIL(-0.0001)", 0 );
  CHECK_EVAL( "CEIL(-0.00001)", 0 );
  CHECK_EVAL( "CEIL(-0.000001)", 0 );
  CHECK_EVAL( "CEIL(-0.0000001)", 0 );
  

  CHECK_EVAL( "CEIL(-1.1)", -1 );
  CHECK_EVAL( "CEIL(-1.01)", -1 );
  CHECK_EVAL( "CEIL(-1.001)", -1 );
  CHECK_EVAL( "CEIL(-1.0001)", -1 );
  CHECK_EVAL( "CEIL(-1.00001)", -1 );
  CHECK_EVAL( "CEIL(-1.000001)", -1 );
  CHECK_EVAL( "CEIL(-1.0000001)", -1 );
}

void TestMathFunctions::testCEILING()
{
  CHECK_EVAL( "CEILING(0; 0.1)", 0 );
  CHECK_EVAL( "CEILING(0; 0.2)", 0 );
  CHECK_EVAL( "CEILING(0; 1.0)", 0 );
  CHECK_EVAL( "CEILING(0; 10.0)", 0 );

  CHECK_EVAL( "CEILING(0.1; 0.2)", 0.2 );
  CHECK_EVAL( "CEILING(0.1; 0.4)", 0.4 );
  CHECK_EVAL( "CEILING(1.1; 0.2)", 1.2 );

  // because can't divide by 0
  CHECK_EVAL( "CEILING(1; 0)", Value::errorDIV0() );
  CHECK_EVAL( "CEILING(2; 0)", Value::errorDIV0() );
  
  // but this one should be just fine !
  CHECK_EVAL( "CEILING(0; 0)", 0 );
  
  // different sign does not make sense
  CHECK_EVAL( "CEILING(-1; 2)", Value::errorNUM() );
  CHECK_EVAL( "CEILING(1; -2)", Value::errorNUM() );
}

void TestMathFunctions::testFIB()
{
  CHECK_EVAL( "FIB(1)", 1 );
  CHECK_EVAL( "FIB(2)", 1 );
  CHECK_EVAL( "FIB(3)", 2 );
  CHECK_EVAL( "FIB(4)", 3 );
  CHECK_EVAL( "FIB(5)", 5 );
  CHECK_EVAL( "FIB(6)", 8 );
  CHECK_EVAL( "FIB(7)", 13 );
  CHECK_EVAL( "FIB(8)", 21 );
  CHECK_EVAL( "FIB(9)", 34 );
  CHECK_EVAL( "FIB(10)", 55 );
  
  // large number
  CHECK_EVAL( "FIB(100)", 3.54224848179263E+20 );
  CHECK_EVAL( "FIB(200)", 2.80571172992512E+41 );
  CHECK_EVAL( "FIB(300)", 2.22232244629423E+62 );
  CHECK_EVAL( "FIB(400)", 1.76023680645016E+83 );
  CHECK_EVAL( "FIB(500)", 1.394232245617E+104 );
  CHECK_EVAL( "FIB(600)", 1.10433070572954E+125 );
  
  // invalid   
  CHECK_EVAL( "FIB(0)", Value::errorNUM() );
  CHECK_EVAL( "FIB(-1)", Value::errorNUM() );
  CHECK_EVAL( "FIB(\"text\")", Value::errorVALUE() );
}


#include <QtTest/QtTest>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kapplication.h>

#define KSPREAD_TEST(TestObject) \
int main(int argc, char *argv[]) \
{ \
    setenv("LC_ALL", "C", 1); \
    setenv("KDEHOME", QFile::encodeName( QDir::homePath() + "/.kde-unit-test" ), 1); \
    KAboutData aboutData( "qttest", "qttest", "version" );  \
    KCmdLineArgs::init(&aboutData); \
    KApplication app; \
    TestObject tc; \
    return QTest::qExec( &tc, argc, argv ); \
}

KSPREAD_TEST(TestMathFunctions)
//QTEST_KDEMAIN(TestMathFunctions, GUI)

#include "TestMathFunctions.moc"
