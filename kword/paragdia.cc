/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

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

#include <paragdia.h>
#include <kwdoc.h>
#include <kcharselectdia.h>
#include <defs.h>

#include <qwidget.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qpen.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qlistbox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qwhatsthis.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kapp.h>
#include <kbuttonbox.h>
#include <kcolorbtn.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <koRuler.h>

#include <stdlib.h>
#include <stdio.h>

/******************************************************************/
/* class KWPagePreview                                            */
/******************************************************************/

/*================================================================*/
KWPagePreview::KWPagePreview( QWidget* parent, const char* name )
    : QGroupBox( i18n( "Preview" ), parent, name )
{
    left = 0;
    right = 0;
    first = 0;
    spacing = 0;
    before = 0;
    after = 0;
}

/*================================================================*/
void KWPagePreview::drawContents( QPainter* p )
{
    int wid = 148;
    int hei = 210;
    int _x = ( width() - wid ) / 2;
    int _y = ( height() - hei ) / 2;

    int dl = static_cast<int>( left / 2 );
    int dr = static_cast<int>( right / 2 );
    //first+left because firstlineIndent is relative to leftIndent
    int df = static_cast<int>( (first+left) / 2 );

    int spc = static_cast<int>( POINT_TO_MM( spacing ) / 5 );

    // draw page
    p->setPen( QPen( black ) );
    p->setBrush( QBrush( black ) );

    p->drawRect( _x + 1, _y + 1, wid, hei );

    p->setBrush( QBrush( white ) );
    p->drawRect( _x, _y, wid, hei );

    // draw parags
    p->setPen( NoPen );
    p->setBrush( QBrush( lightGray ) );

    for ( int i = 1; i <= 4; i++ )
        p->drawRect( _x + 6, _y + 6 + ( i - 1 ) * 12 + 2, wid - 12 - ( ( i / 4 ) * 4 == i ? 50 : 0 ), 6 );

    p->setBrush( QBrush( darkGray ) );

    for ( int i = 5; i <= 8; i++ )
      {
	QRect rect( ( i == 5 ? df : dl ) + _x + 6, _y + 6 + ( i - 1 ) * 12 + 2 + ( i - 5 ) * spc + static_cast<int>( before / 2 ),
		    wid - 12 - ( ( i / 4 ) * 4 == i ? 50 : 0 ) - ( ( i == 12 ? 0 : dr ) + ( i == 5 ? df : dl ) ), 6);

	if(rect.width ()>=0)
	  p->drawRect( rect );
      }
    p->setBrush( QBrush( lightGray ) );

    for ( int i = 9; i <= 12; i++ )
        p->drawRect( _x + 6, _y + 6 + ( i - 1 ) * 12 + 2 + 3 * spc +
                     static_cast<int>( before / 2 ) + static_cast<int>( after / 2 ),
                     wid - 12 - ( ( i / 4 ) * 4 == i ? 50 : 0 ), 6 );

}

/******************************************************************/
/* class KWPagePreview2                                           */
/******************************************************************/

/*================================================================*/
KWPagePreview2::KWPagePreview2( QWidget* parent, const char* name )
    : QGroupBox( i18n( "Preview" ), parent, name )
{
    align = Qt::AlignLeft;
}

/*================================================================*/
void KWPagePreview2::drawContents( QPainter* p )
{
    int wid = 148;
    int hei = 210;
    int _x = ( width() - wid ) / 2;
    int _y = ( height() - hei ) / 2;

    // draw page
    p->setPen( QPen( black ) );
    p->setBrush( QBrush( black ) );

    p->drawRect( _x + 1, _y + 1, wid, hei );

    p->setBrush( QBrush( white ) );
    p->drawRect( _x, _y, wid, hei );

    // draw parags
    p->setPen( NoPen );
    p->setBrush( QBrush( lightGray ) );

    for ( int i = 1; i <= 4; i++ )
        p->drawRect( _x + 6, _y + 6 + ( i - 1 ) * 12 + 2, wid - 12 - ( ( i / 4 ) * 4 == i ? 50 : 0 ), 6 );

    p->setBrush( QBrush( darkGray ) );

    int __x = 0, __w = 0;
    for ( int i = 5; i <= 8; i++ ) {
        switch ( i ) {
        case 5: __w = wid - 12;
            break;
        case 6: __w = wid - 52;
            break;
        case 7: __w = wid - 33;
            break;
        case 8: __w = wid - 62;
        default: break;
        }

        switch ( align ) {
            case Qt3::AlignAuto:
            case Qt::AlignLeft:
                __x = _x + 6;
                break;
            case Qt::AlignCenter:
                __x = _x + ( wid - __w ) / 2;
                break;
            case Qt::AlignRight:
                __x = _x + ( wid - __w ) - 6;
                break;
            case Qt3::AlignJustify:
            {
                if ( i < 8 ) __w = wid - 12;
                __x = _x + 6;
            } break;
        }

        p->drawRect( __x, _y + 6 + ( i - 1 ) * 12 + 2 + ( i - 5 ), __w, 6 );
    }

    p->setBrush( QBrush( lightGray ) );

    for ( int i = 9; i <= 12; i++ )
        p->drawRect( _x + 6, _y + 6 + ( i - 1 ) * 12 + 2 + 3, wid - 12 - ( ( i / 4 ) * 4 == i ? 50 : 0 ), 6 );

}

/******************************************************************/
/* class KWBorderPreview                                          */
/******************************************************************/

/*================================================================*/
KWBorderPreview::KWBorderPreview( QWidget* parent, const char* name )
    : QGroupBox( i18n( "Preview" ), parent, name )
{
}

/*================================================================*/
void KWBorderPreview::drawContents( QPainter* painter )
{
    QRect r = contentsRect();
    QFontMetrics fm( font() );

    painter->fillRect( r.x() + fm.width( 'W' ), r.y() + fm.height(), r.width() - 2 * fm.width( 'W' ),
                       r.height() - 2 * fm.height(), white );
    painter->setClipRect( r.x() + fm.width( 'W' ), r.y() + fm.height(), r.width() - 2 * fm.width( 'W' ),
                          r.height() - 2 * fm.height() );

    if ( topBorder.ptWidth > 0 ) {
        painter->setPen( setBorderPen( topBorder ) );
        painter->drawLine( r.x() + 20, r.y() + 20, r.right() - 20, r.y() + 20 );
    }

    if ( bottomBorder.ptWidth > 0 ) {
        painter->setPen( setBorderPen( bottomBorder ) );
        painter->drawLine( r.x() + 20, r.bottom() - 20, r.right() - 20, r.bottom() - 20 );
    }

    if ( leftBorder.ptWidth > 0 ) {
        painter->setPen( setBorderPen( leftBorder ) );
        painter->drawLine( r.x() + 20, r.y() + 20, r.x() + 20, r.bottom() - 20 );
    }

    if ( rightBorder.ptWidth > 0 ) {
        painter->setPen( setBorderPen( rightBorder ) );
        painter->drawLine( r.right() - 20, r.y() + 20, r.right() - 20, r.bottom() - 20 );
    }
}

/*================================================================*/
QPen KWBorderPreview::setBorderPen( Border _brd )
{
    QPen pen( black, 1, SolidLine );

    pen.setWidth( _brd.ptWidth );
    pen.setColor( _brd.color );

    switch ( _brd.style ) {
    case Border::SOLID:
        pen.setStyle( SolidLine );
        break;
    case Border::DASH:
        pen.setStyle( DashLine );
        break;
    case Border::DOT:
        pen.setStyle( DotLine );
        break;
    case Border::DASH_DOT:
        pen.setStyle( DashDotLine );
        break;
    case Border::DASH_DOT_DOT:
        pen.setStyle( DashDotDotLine );
        break;
    }

    return QPen( pen );
}

/******************************************************************/
/* class KWNumPreview                                             */
/******************************************************************/

/*================================================================*/
KWNumPreview::KWNumPreview( QWidget* parent, const char* name )
    : QGroupBox( i18n( "Preview" ), parent, name )
{
}

/*================================================================*/
void KWNumPreview::drawContents( QPainter* )
{
}

/******************************************************************/
/* Class: KWParagDia                                              */
/******************************************************************/

/*================================================================*/
KWParagDia::KWParagDia( QWidget* parent, const char* name, QStringList _fontList,
                        int _flags, KWDocument *_doc )
    : KDialogBase(Tabbed, QString::null, Ok | Cancel, Ok, parent, name, true )
{
    flags = _flags;
    fontList = _fontList;
    doc = _doc;
    unit=KWUnit::unitType( doc->getUnit() );
    if ( _flags & PD_SPACING )
        setupTab1();
    if ( _flags & PD_ALIGN )
        setupTab2();
    if ( _flags & PD_BORDERS )
        setupTab3();
    if ( _flags & PD_NUMBERING )
        setupTab4();
    if ( _flags & PD_TABS )
        setupTab5();

    setInitialSize( QSize(600, 500) );
}

/*================================================================*/
KWParagDia::~KWParagDia()
{
}

/*================================================================*/
void KWParagDia::setLeftIndent( KWUnit _left )
{
    kdDebug() << "KWParagDia::setLeftIndent mm=" << _left.mm() << " pt=" << _left.pt() << endl;
    QString str = QString::number( _left.value( unit ) );
    eLeft->setText( str );
    prev1->setLeft( _left.mm() );
}

/*================================================================*/
void KWParagDia::setRightIndent( KWUnit _right )
{
    kdDebug() << "KWParagDia::setRightIndent mm=" << _right.mm() << " pt=" << _right.pt() << endl;
    QString str = QString::number( _right.value( unit ) );
    eRight->setText( str );
    prev1->setRight( _right.mm() );
}

/*================================================================*/
void KWParagDia::setFirstLineIndent( KWUnit _first )
{
    QString str = QString::number( _first.value( unit ) );
    eFirstLine->setText( str );
    prev1->setFirst( _first.mm() );
}

/*================================================================*/
void KWParagDia::setSpaceBeforeParag( KWUnit _before )
{
    QString str = QString::number( _before.value( unit ) );
    eBefore->setText( str );
    prev1->setBefore( _before.mm() );
}

/*================================================================*/
void KWParagDia::setSpaceAfterParag( KWUnit _after )
{
    QString str = QString::number( _after.value( unit ) );
    eAfter->setText( str );
    prev1->setAfter( _after.mm() );
}

/*================================================================*/
void KWParagDia::setLineSpacing( KWUnit _spacing )
{
    QString str = QString::number( _spacing.value( unit ) );
    eSpacing->setText( str );
    prev1->setSpacing( _spacing.mm() );
}

/*================================================================*/
void KWParagDia::setAlign( int align )
{
    prev2->setAlign( align );

    clearAligns();
    switch ( align ) {
        case Qt3::AlignAuto: // see KWView::setAlign
        case Qt::AlignLeft:
            rLeft->setChecked( true );
            break;
        case Qt::AlignCenter:
            rCenter->setChecked( true );
            break;
        case Qt::AlignRight:
            rRight->setChecked( true );
            break;
        case Qt3::AlignJustify:
            rJustify->setChecked( true );
            break;
    }
}

/*================================================================*/
int KWParagDia::align() const
{
    if ( rLeft->isChecked() ) return Qt::AlignLeft;
    else if ( rCenter->isChecked() ) return Qt::AlignCenter;
    else if ( rRight->isChecked() ) return Qt::AlignRight;
    else if ( rJustify->isChecked() ) return Qt3::AlignJustify;

    return Qt::AlignLeft;
}

/*================================================================*/
void KWParagDia::setupTab1()
{
    kdDebug() << "KWParagDia::setupTab1" << endl;
    tab1 = addPage( i18n( "Indent and Spacing" ) );
    grid1 = new QGridLayout( tab1, 4, 2, 15, 7 );

    // --------------- indent ---------------
    indentFrame = new QGroupBox( i18n( "Indent" ), tab1 );
    indentGrid = new QGridLayout( indentFrame, 4, 2, 15, 7 );

    lLeft = new QLabel( i18n("Left ( %1 ):").arg(doc->getUnit()), indentFrame );
    lLeft->setAlignment( AlignRight );
    indentGrid->addWidget( lLeft, 1, 0 );

    eLeft = new QLineEdit( indentFrame );
    if ( unit == U_PT )
        eLeft->setValidator( new QIntValidator( eLeft ) );
    else
        eLeft->setValidator( new QDoubleValidator( eLeft ) );
    eLeft->setText( i18n("0.00") );
    eLeft->setMaxLength( 5 );
    eLeft->setEchoMode( QLineEdit::Normal );
    eLeft->setFrame( true );
    indentGrid->addWidget( eLeft, 1, 1 );
    connect( eLeft, SIGNAL( textChanged( const QString & ) ), this, SLOT( leftChanged( const QString & ) ) );

    lRight = new QLabel( i18n("Right ( %1 ):").arg(doc->getUnit()), indentFrame );
    lRight->setAlignment( AlignRight );
    indentGrid->addWidget( lRight, 2, 0 );

    eRight = new QLineEdit( indentFrame );
    if ( unit == U_PT )
        eRight->setValidator( new QIntValidator( eRight ) );
    else
        eRight->setValidator( new QDoubleValidator( eRight ) );
    eRight->setText( i18n("0.00") );
    eRight->setMaxLength( 5 );
    eRight->setEchoMode( QLineEdit::Normal );
    eRight->setFrame( true );
    indentGrid->addWidget( eRight, 2, 1 );
    connect( eRight, SIGNAL( textChanged( const QString & ) ), this, SLOT( rightChanged( const QString & ) ) );

    lFirstLine = new QLabel( i18n("First Line ( %1 ):").arg(doc->getUnit()), indentFrame );
    lFirstLine->setAlignment( AlignRight );
    indentGrid->addWidget( lFirstLine, 3, 0 );

    eFirstLine = new QLineEdit( indentFrame );
    if ( unit == U_PT )
        eFirstLine->setValidator( new QIntValidator( eFirstLine ) );
    else
        eFirstLine->setValidator( new QDoubleValidator( eFirstLine ) );
    eFirstLine->setText( i18n("0.00") );
    eFirstLine->setMaxLength( 5 );
    eFirstLine->setEchoMode( QLineEdit::Normal );
    eFirstLine->setFrame( true );
    connect( eFirstLine, SIGNAL( textChanged( const QString & ) ), this, SLOT( firstChanged( const QString & ) ) );
    indentGrid->addWidget( eFirstLine, 3, 1 );

     // grid row spacing
    indentGrid->addRowSpacing( 0, 5 );
    grid1->addWidget( indentFrame, 0, 0 );

    // --------------- spacing ---------------
    spacingFrame = new QGroupBox( i18n( "Line Spacing" ), tab1 );
    spacingGrid = new QGridLayout( spacingFrame, 3, 1, 15, 7 );

    cSpacing = new QComboBox( false, spacingFrame, "" );
    cSpacing->insertItem( i18n( "0.5 lines" ) );
    cSpacing->insertItem( i18n( "1.0 line" ) );
    cSpacing->insertItem( i18n( "1.5 lines" ) );
    cSpacing->insertItem( i18n( "2.0 lines" ) );
    cSpacing->insertItem( i18n( "Space ( %1 )" ).arg(doc->getUnit()) );
    connect( cSpacing, SIGNAL( activated( int ) ), this, SLOT( spacingActivated( int ) ) );
    spacingGrid->addWidget( cSpacing, 1, 0 );

    eSpacing = new QLineEdit( spacingFrame );
    if ( unit == U_PT )
        eSpacing->setValidator( new QIntValidator( eSpacing ) );
    else
        eSpacing->setValidator( new QDoubleValidator( eSpacing ) );
    eSpacing->setText( i18n("0") );
    eSpacing->setMaxLength( 2 );
    eSpacing->setEchoMode( QLineEdit::Normal );
    eSpacing->setFrame( true );
    connect( eSpacing, SIGNAL( textChanged( const QString & ) ), this, SLOT( spacingChanged( const QString & ) ) );
    spacingGrid->addWidget( eSpacing, 2, 0 );


    // grid row spacing
    spacingGrid->addRowSpacing( 0, 5 );
    grid1->addWidget( spacingFrame, 1, 0 );

    cSpacing->setCurrentItem( 4 );
    cSpacing->setEnabled( false ); // TODO: handle 0.5 lines, 1 line etc
    eSpacing->setEnabled( true );

    // --------------- paragraph spacing ---------------
    pSpaceFrame = new QGroupBox( i18n( "Paragraph Space" ), tab1 );
    pSpaceGrid = new QGridLayout( pSpaceFrame, 3, 2, 15, 7 );

    lBefore = new QLabel( i18n("Before ( %1 ):").arg(doc->getUnit()), pSpaceFrame );
    lBefore->setAlignment( AlignRight );
    pSpaceGrid->addWidget( lBefore, 1, 0 );

    eBefore = new QLineEdit( pSpaceFrame );
    if ( unit == U_PT )
        eBefore->setValidator( new QIntValidator( eBefore ) );
    else
        eBefore->setValidator( new QDoubleValidator( eBefore ) );
    eBefore->setText( i18n("0.00") );
    eBefore->setMaxLength( 5 );
    eBefore->setEchoMode( QLineEdit::Normal );
    eBefore->setFrame( true );
    connect( eBefore, SIGNAL( textChanged( const QString & ) ), this, SLOT( beforeChanged( const QString & ) ) );
    pSpaceGrid->addWidget( eBefore, 1, 1 );

    lAfter = new QLabel( i18n("After ( %1 ):").arg(doc->getUnit()), pSpaceFrame );
    lAfter->setAlignment( AlignRight );
    pSpaceGrid->addWidget( lAfter, 2, 0 );

    eAfter = new QLineEdit( pSpaceFrame );
    if ( unit == U_PT )
        eAfter->setValidator( new QIntValidator( eAfter ) );
    else
        eAfter->setValidator( new QDoubleValidator( eAfter ) );
    eAfter->setText( i18n("0.00") );
    eAfter->setMaxLength( 5 );
    eAfter->setEchoMode( QLineEdit::Normal );
    eAfter->setFrame( true );
    connect( eAfter, SIGNAL( textChanged( const QString & ) ), this, SLOT( afterChanged( const QString & ) ) );
    pSpaceGrid->addWidget( eAfter, 2, 1 );

    // grid row spacing
    pSpaceGrid->addRowSpacing( 0, 5 );
    grid1->addWidget( pSpaceFrame, 2, 0 );

    // --------------- preview --------------------
    prev1 = new KWPagePreview( tab1 );
    grid1->addMultiCellWidget( prev1, 0, 3, 1, 1 );

    grid1->setColStretch( 1, 1 );
    grid1->setRowStretch( 3, 1 );
}

/*================================================================*/
void KWParagDia::setupTab2()
{
    tab2 = addPage( i18n( "Aligns" ) );

    grid2 = new QGridLayout( tab2, 6, 2, 15, 7 );

    lAlign = new QLabel( i18n( "Align:" ), tab2 );
    grid2->addWidget( lAlign, 0, 0 );

    rLeft = new QRadioButton( i18n( "Left" ), tab2 );
    grid2->addWidget( rLeft, 1, 0 );
    connect( rLeft, SIGNAL( clicked() ), this, SLOT( alignLeft() ) );

    rCenter = new QRadioButton( i18n( "Center" ), tab2 );
    grid2->addWidget( rCenter, 2, 0 );
    connect( rCenter, SIGNAL( clicked() ), this, SLOT( alignCenter() ) );

    rRight = new QRadioButton( i18n( "Right" ), tab2 );
    grid2->addWidget( rRight, 3, 0 );
    connect( rRight, SIGNAL( clicked() ), this, SLOT( alignRight() ) );

    rJustify = new QRadioButton( i18n( "Justify" ), tab2 );
    grid2->addWidget( rJustify, 4, 0 );
    connect( rJustify, SIGNAL( clicked() ), this, SLOT( alignJustify() ) );

    clearAligns();
    rLeft->setChecked( true );

    // --------------- preview --------------------
    prev2 = new KWPagePreview2( tab2 );
    grid2->addMultiCellWidget( prev2, 0, 5, 1, 1 );

    // --------------- main grid ------------------
    grid2->setColStretch( 1, 1 );
    grid2->setRowStretch( 5, 1 );
}

/*================================================================*/
void KWParagDia::setupTab3()
{
    tab3 = addPage( i18n( "Borders" ) );

    grid3 = new QGridLayout( tab3, 8, 2, 15, 7 );

    lStyle = new QLabel( i18n( "Style:" ), tab3 );
    grid3->addWidget( lStyle, 0, 0 );

    cStyle = new QComboBox( false, tab3 );
    cStyle->insertItem( i18n( "solid line" ) );
    cStyle->insertItem( i18n( "dash line ( ---- )" ) );
    cStyle->insertItem( i18n( "dot line ( **** )" ) );
    cStyle->insertItem( i18n( "dash dot line ( -*-* )" ) );
    cStyle->insertItem( i18n( "dash dot dot line ( -**- )" ) );
    grid3->addWidget( cStyle, 1, 0 );
    connect( cStyle, SIGNAL( activated( const QString & ) ), this, SLOT( brdStyleChanged( const QString & ) ) );

    lWidth = new QLabel( i18n( "Width:" ), tab3 );
    grid3->addWidget( lWidth, 2, 0 );

    cWidth = new QComboBox( false, tab3 );
    for( unsigned int i = 1; i <= 10; i++ )
        cWidth->insertItem(QString::number(i));
    grid3->addWidget( cWidth, 3, 0 );
    connect( cWidth, SIGNAL( activated( const QString & ) ), this, SLOT( brdWidthChanged( const QString & ) ) );

    lColor = new QLabel( i18n( "Color:" ), tab3 );
    grid3->addWidget( lColor, 4, 0 );

    bColor = new KColorButton( tab3 );
    grid3->addWidget( bColor, 5, 0 );
    connect( bColor, SIGNAL( changed( const QColor& ) ), this, SLOT( brdColorChanged( const QColor& ) ) );

    QButtonGroup * bb = new QHButtonGroup( tab3 );
    bb->setFrameStyle(QFrame::NoFrame);
    bLeft = new QPushButton(bb);
    bLeft->setPixmap( KWBarIcon( "borderleft" ) );
    bLeft->setToggleButton( true );
    bRight = new QPushButton(bb);
    bRight->setPixmap( KWBarIcon( "borderright" ) );
    bRight->setToggleButton( true );
    bTop = new QPushButton(bb);
    bTop->setPixmap( KWBarIcon( "bordertop" ) );
    bTop->setToggleButton( true );
    bBottom = new QPushButton(bb);
    bBottom->setPixmap( KWBarIcon( "borderbottom" ) );
    bBottom->setToggleButton( true );
    grid3->addWidget( bb, 6, 0 );

    connect( bLeft, SIGNAL( toggled( bool ) ), this, SLOT( brdLeftToggled( bool ) ) );
    connect( bRight, SIGNAL( toggled( bool ) ), this, SLOT( brdRightToggled( bool ) ) );
    connect( bTop, SIGNAL( toggled( bool ) ), this, SLOT( brdTopToggled( bool ) ) );
    connect( bBottom, SIGNAL( toggled( bool ) ), this, SLOT( brdBottomToggled( bool ) ) );


    prev3 = new KWBorderPreview( tab3 );
    grid3->addMultiCellWidget( prev3, 0, 7, 1, 1 );

    grid3->setRowStretch( 7, 1 );
    grid3->setColStretch( 1, 1 );

    m_bAfterInitBorder=false;
}

/*================================================================*/
void KWParagDia::setupTab4()
{
    tab4 = addPage( i18n( "Bullets/Numbers" ) );

    grid4 = new QGridLayout( tab4, 4, 2, 15, 7 );

    gType = new QGroupBox( i18n("Type"), tab4 );
    tgrid = new QGridLayout( gType, 13, 3, 5, 5 );

    g1 = new QButtonGroup( gType );
    g1->hide();
    g1->setExclusive( true );

    rNone = new QRadioButton( i18n( "&No numbering" ), gType );
    tgrid->addMultiCellWidget( rNone, 1, 1, 0, 2 );
    g1->insert( rNone, 0 );

    rANums = new QRadioButton( i18n( "&Arabic Numbers ( 1, 2, 3, 4, ... )" ), gType );
    tgrid->addMultiCellWidget( rANums, 2, 2, 0, 2 );
    g1->insert( rANums, 1 );

    rLRNums = new QRadioButton( i18n( "&Lower Roman Numbers ( i, ii, iii, iv, ... )" ), gType );
    tgrid->addMultiCellWidget( rLRNums, 3, 3, 0, 2 );
    g1->insert( rLRNums, 4 );

    rURNums = new QRadioButton( i18n( "&Upper Roman Numbers ( I, II, III, IV, ... )" ), gType );
    tgrid->addMultiCellWidget( rURNums, 4, 4, 0, 2 );
    g1->insert( rURNums, 5 );

    rLAlph = new QRadioButton( i18n( "L&ower Alphabetical ( a, b, c, d, ... )" ), gType );
    tgrid->addMultiCellWidget( rLAlph, 5, 5, 0, 2 );
    g1->insert( rLAlph, 2 );

    rUAlph = new QRadioButton( i18n( "U&pper Alphabetical ( A, B, C, D, ... )" ), gType );
    tgrid->addMultiCellWidget( rUAlph, 6, 6, 0, 2 );
    g1->insert( rUAlph, 3 );

    rCustom = new QRadioButton( i18n( "&Custom" ), gType );
    tgrid->addWidget( rCustom, 7, 0 );
    g1->insert( rCustom, 7 );
    rCustom->setEnabled(false); // Not implemented

    eCustomNum = new QLineEdit( gType );
    eCustomNum->setEnabled( false );
    tgrid->addMultiCellWidget( eCustomNum, 7, 7, 1, 2 );
    connect( rCustom, SIGNAL( toggled(bool) ), eCustomNum, SLOT( setEnabled(bool) ));
    connect( eCustomNum, SIGNAL( textChanged(const QString&) ),
             this, SLOT( counterDefChanged(const QString&) ) );


    QString custcountwt(i18n("<h1>Create custom counters</h1>\n"
        "<p>You can enter a string describing your custom counter, consisting of \n"
        " the following symbols. For now, this string may not contain any whitespace \n"
        " or additional text. This will change.</p>\n"
        "<ul><li>\\arabic - arabic numbers (1, 2, 3, ...)</li><li>\\roman or \\Roman - lower or uppercase roman numbers</li>\n"
        "<li>\\alph or \\Alph - lower or uppercase latin letters</li></ul>\n"
        "<p>This will hopefully have more options in the future (like enumerated lists or greek letters).</p>" ));
    QWhatsThis::add( rCustom, custcountwt );
    QWhatsThis::add( eCustomNum, custcountwt );

    rDiscBullet = new QRadioButton( i18n( "&Disc Bullet" ), gType );
    tgrid->addMultiCellWidget( rDiscBullet, 9, 9, 0, 2 );
    g1->insert( rDiscBullet, 6 );

    rSquareBullet = new QRadioButton( i18n( "&Square Bullet" ), gType );
    tgrid->addMultiCellWidget( rSquareBullet, 10, 10, 0, 2 );
    g1->insert( rSquareBullet, 7 );

    rCircleBullet = new QRadioButton( i18n( "&Circle Bullet" ), gType );
    tgrid->addMultiCellWidget( rCircleBullet, 11, 11, 0, 2 );
    g1->insert( rCircleBullet, 8 );

    rBullets = new QRadioButton( i18n( "Custom Bullet" ), gType );
    tgrid->addWidget( rBullets, 12, 0 );
    g1->insert( rBullets, 9 );

    bBullets = new QPushButton( gType );
    tgrid->addWidget( bBullets, 12, 1 );
    connect( bBullets, SIGNAL( clicked() ), this, SLOT( changeBullet() ) );

    connect( g1, SIGNAL( clicked( int ) ), this, SLOT( typeChanged( int ) ) );

    tgrid->addRowSpacing( 0, 10 );
    tgrid->setRowStretch( 8, 1 );
    tgrid->setRowStretch( 12, 1 );
    tgrid->setColStretch( 2, 10 );

    grid4->addWidget( gType, 0, 0 );

    gText = new QGroupBox( i18n("Text"), tab4 );
    txtgrid = new QGridLayout( gText, 4, 2, 5, 5 );

    lcLeft = new QLabel( i18n( "Left" ), gText );
    txtgrid->addWidget( lcLeft, 1, 0 );

    lcRight = new QLabel( i18n( "Right" ), gText );
    txtgrid->addWidget( lcRight, 1, 1 );

    ecLeft = new QLineEdit( gText );
    txtgrid->addWidget( ecLeft, 2, 0 );
    connect( ecLeft, SIGNAL( textChanged( const QString & ) ), this, SLOT( leftTextChanged( const QString & ) ) );

    ecRight = new QLineEdit( gText );
    txtgrid->addWidget( ecRight, 2, 1 );
    connect( ecRight, SIGNAL( textChanged( const QString & ) ), this, SLOT( rightTextChanged( const QString & ) ) );

    txtgrid->addRowSpacing( 0, 10 );
    txtgrid->setRowStretch( 3, 1 );

    txtgrid->setColStretch( 0, 1 );
    txtgrid->setColStretch( 1, 1 );

    grid4->addWidget( gText, 1, 0 );

    ///

    gOther = new QGroupBox( i18n("Other Settings"), tab4 );
    ogrid = new QGridLayout( gOther, 6, 2, 5, 5 );
    g2 = new QButtonGroup( gOther );
    g2->hide();
    g2->setExclusive( true );
    ogrid->addRowSpacing( 0, 10 );

    lStart = new QLabel( i18n( "Start at ( 1, 2, ... ) :" ), gOther );
    lStart->setAlignment( AlignRight | AlignVCenter );
    ogrid->addWidget( lStart, 1, 0 );

    // TODO: make this a spinbox or a combo, with values depending on the type
    // of numbering.
    eStart = new QLineEdit( gOther );
    ogrid->addWidget( eStart, 1, 1 );
    connect( eStart, SIGNAL( textChanged( const QString & ) ), this, SLOT( startChanged( const QString & ) ) );

    rList = new QRadioButton( i18n( "&List Numbering" ), gOther );
    ogrid->addMultiCellWidget( rList, 2, 2, 0, 1 );
    g2->insert( rList, 0 );

    rChapter = new QRadioButton( i18n( "&Chapter Numbering" ), gOther );
    ogrid->addMultiCellWidget( rChapter, 3, 3, 0, 1 );
    g2->insert( rChapter, 1 );

    lDepth = new QLabel( i18n( "Depth:" ), gOther );
    lDepth->setAlignment( AlignRight | AlignVCenter );
    ogrid->addWidget( lDepth, 4, 0 );

    sDepth = new QSpinBox( 0, 15, 1, gOther );
    ogrid->addWidget( sDepth, 4, 1 );
    connect( sDepth, SIGNAL( valueChanged( int ) ), this, SLOT( depthChanged( int ) ) );

    connect( g2, SIGNAL( clicked( int ) ), this, SLOT( numTypeChanged( int ) ) );

    ogrid->setRowStretch( 5, 1 );
    ogrid->setColStretch( 2, 1 );

    grid4->addWidget( gOther, 2, 0 );

    prev4 = new KWNumPreview( tab4 );
    grid4->addMultiCellWidget( prev4, 0, 2, 1, 1 );
    grid4->addColSpacing( 1, 100 );
    grid4->setColStretch( 1, 1 );
}

/*================================================================*/
void KWParagDia::setupTab5()
{
    tab5 = addPage( i18n( "Tabulators" ) );
    grid5 = new QGridLayout( tab5, 4, 2, 15, 7 );

    lTab = new QLabel(  tab5 );
    grid5->addWidget( lTab, 0, 0 );

    eTabPos = new QLineEdit( tab5 );

    if ( unit == U_PT )
        eTabPos->setValidator( new QIntValidator( eTabPos ) );
    else
        eTabPos->setValidator( new QDoubleValidator( eTabPos ) );
    grid5->addWidget( eTabPos, 1, 0 );

    QString unitText;
    switch ( unit )
      {
      case U_MM:
	unitText=i18n("in Millimeters (mm)");
	break;
      case U_INCH:
	unitText=i18n("in Inches (inch)");
	break;
      case U_PT:
      default:
	unitText=i18n("in points ( pt )" );
      }
    lTab->setText(i18n( "Tabulator positions are given %1" ).arg(unitText));

    KButtonBox * bbTabs = new KButtonBox( tab5 );
    bAdd = bbTabs->addButton( i18n( "Add" ), false );
    bDel = bbTabs->addButton( i18n( "Delete" ), false );
    bModify = bbTabs->addButton( i18n( "Modify" ), false );
    bModify->setEnabled(false);
    grid5->addWidget( bbTabs, 2, 0 );

    lTabs = new QListBox( tab5 );
    grid5->addWidget( lTabs, 3, 0 );

    g3 = new QButtonGroup( "", tab5 );
    tabGrid = new QGridLayout( g3, 5, 1, 15, 7 );
    g3->setExclusive( true );

    rtLeft = new QRadioButton( i18n( "Left" ), g3 );
    rtLeft->setChecked(true);
    tabGrid->addWidget( rtLeft, 0, 0 );
    g3->insert( rtLeft );

    rtCenter = new QRadioButton( i18n( "Center" ), g3 );
    tabGrid->addWidget( rtCenter, 1, 0 );
    g3->insert( rtCenter );

    rtRight = new QRadioButton( i18n( "Right" ), g3 );
    tabGrid->addWidget( rtRight, 2, 0 );
    g3->insert( rtRight );

    rtDecimal = new QRadioButton( i18n( "Decimal" ), g3 );
    tabGrid->addWidget( rtDecimal, 3, 0 );
    g3->insert( rtDecimal );

    tabGrid->setRowStretch( 4, 1 );
    tabGrid->setColStretch( 0, 1 );
    grid5->addWidget( g3, 3, 1 );
    grid5->setRowStretch( 3, 1 );
    if(lTabs->count()==0)
      {
	bDel->setEnabled(false);
	bModify->setEnabled(false);
      }

    _tabList.setAutoDelete( TRUE );


    connect(bAdd,SIGNAL(clicked ()),this,SLOT(addClicked()));
    connect(bModify,SIGNAL(clicked ()),this,SLOT(modifyClicked()));
    connect(bDel,SIGNAL(clicked ()),this,SLOT(delClicked()));
    connect(lTabs,SIGNAL(doubleClicked( QListBoxItem * ) ),this,SLOT(slotDoubleClicked( QListBoxItem * ) ));
    connect(lTabs,SIGNAL(clicked( QListBoxItem * ) ),this,SLOT(slotDoubleClicked( QListBoxItem * ) ));
}


/*================================================================*/
void KWParagDia::addClicked()
{
  if(!eTabPos->text().isEmpty())
    {
      if(findExistingValue(eTabPos->text().toDouble()))
	{
	  QString tmp=i18n("There is a tabulator at this position");
	  KMessageBox::error( this, tmp);
	  eTabPos->setText("");
	  return;
	}

      lTabs->insertItem(eTabPos->text());
      bDel->setEnabled(true);
      //bModify->setEnabled(true);

      KoTabulator *tab=new KoTabulator;
      if(rtLeft->isChecked())
	tab->type=T_LEFT;
      else if(rtCenter->isChecked())
	tab->type=T_CENTER;
      else if(rtRight->isChecked())
	tab->type=T_RIGHT;
      else if(rtDecimal->isChecked())
	tab->type=T_DEC_PNT;
      else
	tab->type=T_LEFT;
      double val=eTabPos->text().toDouble();
      switch ( unit )
	{
	case U_MM:
	  tab->mmPos=val;
	  tab->inchPos=MM_TO_INCH(val);
	  tab->ptPos=MM_TO_POINT(val);
	  break;
	case U_INCH:
	  tab->mmPos=INCH_TO_MM(val);
	  tab->inchPos=val;
	  tab->ptPos= INCH_TO_POINT(val);
	  break;
	case U_PT:
	default:
	  tab->mmPos=POINT_TO_MM(val);
	  tab->inchPos=POINT_TO_INCH(val);
	  tab->ptPos=val;
	}
      _tabList.append(tab);
      eTabPos->setText("");
    }
}

bool KWParagDia::findExistingValue(double val)
{
  KoTabulator *tmp;
  for ( tmp=_tabList.first(); tmp != 0; tmp= _tabList.next() )
    {
       switch ( unit )
	 {
	 case U_MM:
	   if(tmp->mmPos==val)
	     return true;
	   break;
	 case U_INCH:
	   if(tmp->inchPos==val)
	     return true;
	   break;
	 case U_PT:
	   if(tmp->ptPos==val)
	      return true;
	   break;
	 }
    }
  return false;
}

/*================================================================*/
void KWParagDia::modifyClicked()
{
  if(!eTabPos->text().isEmpty() && lTabs->currentItem()!=-1)
    {
      int pos=lTabs->currentItem();
      lTabs->removeItem(lTabs->currentItem());
      lTabs->insertItem(eTabPos->text(),pos);
      lTabs->setCurrentItem(pos);
      eTabPos->setText("");
    }

}

/*================================================================*/
void KWParagDia::delClicked()
{
  if(lTabs->currentItem()!=-1)
    {
      double value=lTabs->currentText().toDouble();
      lTabs->removeItem(lTabs->currentItem());
      KoTabulator *tmp;
      for ( tmp=_tabList.first(); tmp != 0; tmp= _tabList.next() )
	{
	  switch ( unit )
	    {
	    case U_MM:
	      if(tmp->mmPos==value)
		{
		  switch(tmp->type)
		    {
		    case T_LEFT:
		      if(rtLeft->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_CENTER:
		      if(rtCenter->isChecked())
			_tabList.remove(tmp);
		      break;
		    case  T_RIGHT:
		      if(rtRight->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_DEC_PNT:
		      if(rtDecimal->isChecked())
			_tabList.remove(tmp);
		      break;
		    }
		}
	      break;
	    case U_INCH:
	      if(tmp->inchPos==value)
		{
		  switch(tmp->type)
		    {
		    case T_LEFT:
		      if(rtLeft->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_CENTER:
		      if(rtCenter->isChecked())
			_tabList.remove(tmp);
		      break;
		    case  T_RIGHT:
		      if(rtRight->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_DEC_PNT:
		      if(rtDecimal->isChecked())
			_tabList.remove(tmp);
		      break;
		    }
		}

	      break;
	    case U_PT:
	    default:
	      if(tmp->ptPos==value)
		{
		  switch(tmp->type)
		    {
		    case T_LEFT:
		      if(rtLeft->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_CENTER:
		      if(rtCenter->isChecked())
			_tabList.remove(tmp);
		      break;
		    case  T_RIGHT:
		      if(rtRight->isChecked())
			_tabList.remove(tmp);
		      break;
		    case T_DEC_PNT:
		      if(rtDecimal->isChecked())
			_tabList.remove(tmp);
		      break;
		    }
		}
	    }

	}
      eTabPos->setText("");
      if(lTabs->count()==0)
	{
	  bDel->setEnabled(false);
	  bModify->setEnabled(false);

	}
      else
	{
	  lTabs->setCurrentItem(0);
	  setActifItem(lTabs->currentText().toDouble());
	}
    }
}

void KWParagDia::setActifItem(double value)
{
  KoTabulator *tmp;
  for ( tmp=_tabList.first(); tmp != 0; tmp= _tabList.next() )
    {
      switch ( unit )
	{
	case U_MM:
	  if(tmp->mmPos==value)
	    {
	      switch(tmp->type)
		{
		case T_LEFT:
		  rtLeft->setChecked(true);
		  break;
		case T_CENTER:
		  rtCenter->setChecked(true);
		  break;
		case  T_RIGHT:
		  rtRight->setChecked(true);
		  break;
		case T_DEC_PNT:
		  rtDecimal->setChecked(true);
		  break;
		}
	    }
	  break;
	case U_INCH:
	  if(tmp->inchPos==value)
	    {
	      switch(tmp->type)
		{
		case T_LEFT:
		  rtLeft->setChecked(true);
		  break;
		case T_CENTER:
		  rtCenter->setChecked(true);
		  break;
		case  T_RIGHT:
		  rtRight->setChecked(true);
		  break;
		case T_DEC_PNT:
		  rtDecimal->setChecked(true);
		  break;
		}
	    }

	  break;
	case U_PT:
	default:
	  if(tmp->ptPos==value)
	    {
	      switch(tmp->type)
		{
		case T_LEFT:
		  rtLeft->setChecked(true);
		  break;
		case T_CENTER:
		  rtCenter->setChecked(true);
		  break;
		case  T_RIGHT:
		  rtRight->setChecked(true);
		  break;
		case T_DEC_PNT:
		  rtDecimal->setChecked(true);
		  break;
		}
	    }
	}
    }
}

/*================================================================*/
void KWParagDia::slotDoubleClicked( QListBoxItem * )
{
  if(lTabs->currentItem()!=-1)
    {
      eTabPos->setText(lTabs->currentText());
      double value=lTabs->currentText().toDouble();
      bDel->setEnabled(true);
      //bModify->setEnabled(true);
      setActifItem(value);
    }
}



/*================================================================*/
void KWParagDia::clearAligns()
{
    rLeft->setChecked( false );
    rCenter->setChecked( false );
    rRight->setChecked( false );
    rJustify->setChecked( false );
}

/*================================================================*/
void KWParagDia::updateBorders()
{
    if ( m_leftBorder.ptWidth == 0 )
        bLeft->setOn( false );
    else
        bLeft->setOn( true );

    if ( m_rightBorder.ptWidth == 0 )
        bRight->setOn( false );
    else
        bRight->setOn( true );

    if ( m_topBorder.ptWidth == 0 )
        bTop->setOn( false );
    else
        bTop->setOn( true );

    if ( m_bottomBorder.ptWidth == 0 )
        bBottom->setOn( false );
    else
        bBottom->setOn( true );
    prev3->setLeftBorder( m_leftBorder );
    prev3->setRightBorder( m_rightBorder );
    prev3->setTopBorder( m_topBorder );
    prev3->setBottomBorder( m_bottomBorder );
}

/*================================================================*/
void KWParagDia::leftChanged( const QString & _text )
{
    prev1->setLeft( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::rightChanged( const QString & _text )
{
  prev1->setRight( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::firstChanged( const QString & _text )
{
    prev1->setFirst( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::spacingActivated( int _index )
{
    if ( _index == 4 ) {
        eSpacing->setEnabled( true );
        eSpacing->setText( "12.0" );
        eSpacing->setFocus();
    } else {
        eSpacing->setEnabled( false );
        switch ( _index ) {
        case 0: eSpacing->setText( "14.0" );
            break;
        case 1: eSpacing->setText( "28.0" );
            break;
        case 2: eSpacing->setText( "42.0" );
            break;
        case 3: eSpacing->setText( "56.0" );
            break;
        }
    }
    prev1->setSpacing( eSpacing->text().toDouble() );
}

/*================================================================*/
void KWParagDia::spacingChanged( const QString & _text )
{
  prev1->setSpacing( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::beforeChanged( const QString & _text )
{
    prev1->setBefore( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::afterChanged( const QString & _text )
{
    prev1->setAfter( _text.toDouble() );
}

/*================================================================*/
void KWParagDia::alignLeft()
{
    prev2->setAlign( Qt::AlignLeft );
    clearAligns();
    rLeft->setChecked( true );
}

/*================================================================*/
void KWParagDia::alignCenter()
{
    prev2->setAlign( Qt::AlignCenter );
    clearAligns();
    rCenter->setChecked( true );
}

/*================================================================*/
void KWParagDia::alignRight()
{
    prev2->setAlign( Qt::AlignRight );
    clearAligns();
    rRight->setChecked( true );
}

/*================================================================*/
void KWParagDia::alignJustify()
{
    prev2->setAlign( Qt3::AlignJustify );
    clearAligns();
    rJustify->setChecked( true );
}

/*================================================================*/
void KWParagDia::brdLeftToggled( bool _on )
{
    if ( !_on )
        m_leftBorder.ptWidth = 0;
    else {
      if(m_bAfterInitBorder)
	{
	  m_leftBorder.ptWidth = cWidth->currentText().toInt();
	  m_leftBorder.color = QColor( bColor->color() );
	  m_leftBorder.style= Border::getStyle( cStyle->currentText() );
	}
    }
    prev3->setLeftBorder( m_leftBorder );
}

/*================================================================*/
void KWParagDia::brdRightToggled( bool _on )
{
    if ( !_on )
        m_rightBorder.ptWidth = 0;
    else {
      if(m_bAfterInitBorder)
	{
	  m_rightBorder.ptWidth = cWidth->currentText().toInt();
	  m_rightBorder.color = QColor( bColor->color() );
	  m_rightBorder.style= Border::getStyle( cStyle->currentText() );
	}
    }
    prev3->setRightBorder( m_rightBorder );
}

/*================================================================*/
void KWParagDia::brdTopToggled( bool _on )
{
    if ( !_on )
        m_topBorder.ptWidth = 0;
    else {
      if(m_bAfterInitBorder)
	{

	  m_topBorder.ptWidth = cWidth->currentText().toInt();
	  m_topBorder.color = QColor( bColor->color() );
	  m_topBorder.style= Border::getStyle( cStyle->currentText() );
	}
    }
    prev3->setTopBorder( m_topBorder );
}

/*================================================================*/
void KWParagDia::brdBottomToggled( bool _on )
{
    if ( !_on )
        m_bottomBorder.ptWidth = 0;
    else {
      if(m_bAfterInitBorder)
	{
	  m_bottomBorder.ptWidth = cWidth->currentText().toInt();
	  m_bottomBorder.color = QColor( bColor->color() );
	  m_bottomBorder.style=Border::getStyle(cStyle->currentText());
	}
    }
    prev3->setBottomBorder( m_bottomBorder );
}

/*================================================================*/
void KWParagDia::brdStyleChanged( const QString & )
{
}

/*================================================================*/
void KWParagDia::brdWidthChanged( const QString & )
{
}

/*================================================================*/
void KWParagDia::brdColorChanged( const QColor & )
{
}

/*================================================================*/
void KWParagDia::changeBullet()
{
  rBullets->setChecked(true);
  typeChanged( g1->id(rBullets) );//activate CT_CUSTOMBULLET
  QString f = m_counter.bulletFont;
  QChar c = m_counter.counterBullet;

  if ( KCharSelectDia::selectChar( f, c ) ) {
    m_counter.bulletFont = f;
    m_counter.counterBullet = c;
    bBullets->setText( c );
    bBullets->setFont( QFont( m_counter.bulletFont ) );
    prev4->setCounter( m_counter );
  }
}

/*================================================================*/
void KWParagDia::typeChanged( int _type )
{
  static const int buttongroup2CounterType[] = {
      Counter::CT_NONE, Counter::CT_NUM, Counter::CT_ALPHAB_L, Counter::CT_ALPHAB_U,
      Counter::CT_ROM_NUM_L, Counter::CT_ROM_NUM_U, Counter::CT_DISCBULLET,
      Counter::CT_SQUAREBULLET, Counter::CT_CIRCLEBULLET, Counter::CT_CUSTOMBULLET,
      Counter::CT_CUSTOM };

  m_counter.counterType = static_cast<Counter::CounterType>(buttongroup2CounterType[_type]);
  enableUIForCounterType();
}

void KWParagDia::enableUIForCounterType()
{
  switch( m_counter.counterType )
  {
      case Counter::CT_NONE:
      case Counter::CT_DISCBULLET:
      case Counter::CT_SQUAREBULLET:
      case Counter::CT_CIRCLEBULLET:
      case Counter::CT_CUSTOMBULLET:
          gText->setEnabled( false );
          gOther->setEnabled( false );
          lDepth->setEnabled( false );
          sDepth->setEnabled( false );
          break;
      default:
          gText->setEnabled( true );
          gOther->setEnabled( true );
          lDepth->setEnabled( m_counter.numberingType == Counter::NT_CHAPTER );
          sDepth->setEnabled( m_counter.numberingType == Counter::NT_CHAPTER );
  }

}

/*================================================================*/
void KWParagDia::counterDefChanged( const QString& _cd )
{
    m_counter.customCounterDef = _cd;
}

/*================================================================*/
void KWParagDia::numTypeChanged( int _ntype )
{
    m_counter.numberingType = static_cast<Counter::NumType>( _ntype );
    lDepth->setEnabled( m_counter.numberingType == Counter::NT_CHAPTER );
    sDepth->setEnabled( m_counter.numberingType == Counter::NT_CHAPTER );
}

/*================================================================*/
void KWParagDia::leftTextChanged( const QString & _c )
{
    m_counter.counterLeftText = _c;
}

/*================================================================*/
void KWParagDia::rightTextChanged( const QString & _c )
{
    m_counter.counterRightText = _c;
}

/*================================================================*/
void KWParagDia::startChanged( const QString & _c )
{
    m_counter.startCounter = _c.toInt(); // HACK
}

/*================================================================*/
void KWParagDia::depthChanged( int _val )
{
    m_counter.counterDepth = _val;
}

/*================================================================*/
void KWParagDia::setCounter( Counter _counter )
{
    prev4->setCounter( _counter );
    m_counter = _counter;

    switch ( m_counter.counterType ) {
    case Counter::CT_NONE: rNone->setChecked( true );
        break;
    case Counter::CT_NUM: rANums->setChecked( true );
        break;
    case Counter::CT_ALPHAB_L: rLAlph->setChecked( true );
        break;
    case Counter::CT_ALPHAB_U: rUAlph->setChecked( true );
        break;
    case Counter::CT_ROM_NUM_L: rLRNums->setChecked( true );
        break;
    case Counter::CT_ROM_NUM_U: rURNums->setChecked( true );
        break;
    case Counter::CT_CIRCLEBULLET: rCircleBullet->setChecked( true );
        break;
    case Counter::CT_SQUAREBULLET: rSquareBullet->setChecked( true );
        break;
    case Counter::CT_DISCBULLET: rDiscBullet->setChecked( true );
        break;
    case Counter::CT_CUSTOMBULLET: rBullets->setChecked( true );
        break;
    case Counter::CT_CUSTOM: rCustom->setChecked( true );
        break;
    }

    switch ( m_counter.numberingType ) {
    case Counter::NT_LIST: rList->setChecked( true );
        break;
    case Counter::NT_CHAPTER: rChapter->setChecked( true );
        break;
    }

    eCustomNum->setText( m_counter.customCounterDef );

    bBullets->setText( m_counter.counterBullet );
    bBullets->setFont( QFont( m_counter.bulletFont ) );

    ecLeft->setText( m_counter.counterLeftText );
    ecRight->setText( m_counter.counterRightText );

    sDepth->setValue( m_counter.counterDepth );
    // What we really need is a combobox filled with values depending on
    // the type of numbering - or a spinbox. (DF)
    eStart->setText( QString::number(m_counter.startCounter) ); // HACK

    enableUIForCounterType();
}

/*================================================================*/
/*void KWParagDia::setTabList( const QList<KoTabulator> *tabList )
{
    lTabs->clear();
    QListIterator<KoTabulator> it(*tabList);
    for ( ; it.current(); ++it )
        lTabs->insertItem(QString::number((*it)->ptPos));
}*/

/*================================================================*/
KWUnit KWParagDia::leftIndent() const
{
    return KWUnit::createUnit( QMAX(eLeft->text().toDouble(),0), unit );
}

/*================================================================*/
KWUnit KWParagDia::rightIndent() const
{
    return KWUnit::createUnit( QMAX(eRight->text().toDouble(),0), unit );
}

/*================================================================*/
KWUnit KWParagDia::firstLineIndent() const
{
  return KWUnit::createUnit( eFirstLine->text().toDouble(), unit );
}

/*================================================================*/
KWUnit KWParagDia::spaceBeforeParag() const
{
    return KWUnit::createUnit( QMAX(eBefore->text().toDouble(),0), unit );
}

/*================================================================*/
KWUnit KWParagDia::spaceAfterParag() const
{
    return KWUnit::createUnit( QMAX(eAfter->text().toDouble(),0), unit );
}

/*================================================================*/
KWUnit KWParagDia::lineSpacing() const
{
    return KWUnit::createUnit( QMAX(eSpacing->text().toDouble(),0), unit );
}

void KWParagDia::setParagLayout( const KWParagLayout & lay )
{
    setAlign( lay.alignment );
    setFirstLineIndent( lay.margins[QStyleSheetItem::MarginFirstLine] );
    setLeftIndent( lay.margins[QStyleSheetItem::MarginLeft] );
    setRightIndent( lay.margins[QStyleSheetItem::MarginRight] );
    setSpaceBeforeParag( lay.margins[QStyleSheetItem::MarginTop] );
    setSpaceAfterParag( lay.margins[QStyleSheetItem::MarginBottom] );
    setCounter( lay.counter );
    setLineSpacing( lay.lineSpacing );
    setLeftBorder( lay.leftBorder );
    setRightBorder( lay.rightBorder );
    setTopBorder( lay.topBorder );
    setBottomBorder( lay.bottomBorder );
    oldLayout=lay;
    //setTabList( lay.ParagLayout->getTabList );
    //border init it's necessary to allow left border works
    m_bAfterInitBorder=true;
}
#include "paragdia.moc"
