/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <qpainter.h>
#include <qdrawutil.h>
#include <qkeycode.h>
#include <qregexp.h>
#include <qpoint.h>
#include <qprinter.h>
#include <qcursor.h>
#include <qstack.h>
#include <qbuffer.h>
#include <qmessagebox.h>
#include <qclipboard.h>
#include <qpicture.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qdragobject.h>
#include <qmime.h>

#include <klocale.h>
#include <kglobal.h>

#include "kspread_table.h"
#include "kspread_undo.h"
#include "kspread_map.h"
#include "kspread_doc.h"
#include "kspread_util.h"
#include "kspread_canvas.h"

#include "KSpreadTableIface.h"

#include <koStream.h>

#include <strstream.h>
#include <kdebug.h>

#include "../kchart/kchart_part.h"

/* include the kauto_array class here since this is the only place
 * where it is used */
template <class X>
class kauto_array {
  X *ptr;

public:
  typedef X element_type;
  explicit kauto_array(size_t n) { ptr = new X[n]; }
  ~kauto_array() { delete [] ptr; }

  X& operator[](int n) { return ptr[n]; }
  operator X*() { return ptr; }
  X* operator +(int offset) { return ptr + offset; }

private:
  kauto_array& operator=(const kauto_array& a);
  kauto_array(const kauto_array& a);
};


/* ############ Torben
   There is this m_lstChildren in KSpreadTable. Make sure to keep in addition
   the m_lstChildren of ContainerPart up to date.
*/

/*****************************************************************************
 *
 * CellBinding
 *
 *****************************************************************************/

CellBinding::CellBinding( KSpreadTable *_table, const QRect& _area )
{
  m_rctDataArea = _area;

  m_pTable = _table;
  m_pTable->addCellBinding( this );

  m_bIgnoreChanges = false;
}

CellBinding::~CellBinding()
{
  m_pTable->removeCellBinding( this );
}

void CellBinding::cellChanged( KSpreadCell *_cell )
{
  if ( m_bIgnoreChanges )
    return;

  emit changed( _cell );
}

bool CellBinding::contains( int _x, int _y )
{
  return m_rctDataArea.contains( QPoint( _x, _y ) );
}

/*****************************************************************************
 *
 * ChartBinding
 *
 *****************************************************************************/

ChartBinding::ChartBinding( KSpreadTable *_table, const QRect& _area, ChartChild *_child )
    : CellBinding( _table, _area )
{
  m_child = _child;
}

ChartBinding::~ChartBinding()
{
}

void ChartBinding::cellChanged( KSpreadCell* )
{
  kdDebug(36001) << "######### void ChartBinding::cellChanged( KSpreadCell* )" << endl;

  if ( m_bIgnoreChanges )
    return;

  kdDebug(36001) << "with=" << m_rctDataArea.width() << "  height=" << m_rctDataArea.height() << endl;

  KChartData matrix( m_rctDataArea.height(), m_rctDataArea.width() );

  //matrix.matrix.length( ( m_rctDataArea.width() - 1 ) * ( m_rctDataArea.height() - 1 ) );
  for ( int y = 0; y < m_rctDataArea.height(); y++ )
    for ( int x = 0; x < m_rctDataArea.width(); x++ )
    {
	KSpreadCell* cell = m_pTable->cellAt( m_rctDataArea.left() + x, m_rctDataArea.top() + y );
	matrix.cell( y, x ).exists = TRUE;
	if ( cell && cell->isValue() )
	    matrix.cell( y, x ).value = cell->valueDouble();
	else if ( cell )
	    matrix.cell( y, x ).value = cell->valueString();
	else
	    matrix.cell( y, x ).exists = FALSE;
    }

  // ######### Kalle may be interested in that, too
  /* Chart::Range range;
  range.top = m_rctDataArea.top();
  range.left = m_rctDataArea.left();
  range.right = m_rctDataArea.right();
  range.bottom = m_rctDataArea.bottom();
  range.table = m_pTable->name(); */

  m_child->chart()->setPart( matrix );
}

/*****************************************************************************
 *
 * KSpreadTable
 *
 *****************************************************************************/

int KSpreadTable::s_id = 0L;
QIntDict<KSpreadTable>* KSpreadTable::s_mapTables;

KSpreadTable* KSpreadTable::find( int _id )
{
  if ( !s_mapTables )
    return 0L;

  return (*s_mapTables)[ _id ];
}

KSpreadTable::KSpreadTable( KSpreadMap *_map, const char *_name )
    : QObject( _map, _name )
{
  if ( s_mapTables == 0L )
    s_mapTables = new QIntDict<KSpreadTable>;
  m_id = s_id++;
  s_mapTables->insert( m_id, this );

  m_pMap = _map;
  m_pDoc = _map->doc();
  m_dcop = 0;
  m_bShowPageBorders = FALSE;

  m_lstCellBindings.setAutoDelete( FALSE );

  m_strName = _name;

  m_lstChildren.setAutoDelete( true );

  m_dctCells.setAutoDelete( true );
  m_dctRows.setAutoDelete( true );
  m_dctColumns.setAutoDelete( true );

  m_pDefaultCell = new KSpreadCell( this, 0, 0 );
  m_pDefaultRowLayout = new RowLayout( this, 0 );
  m_pDefaultRowLayout->setDefault();
  m_pDefaultColumnLayout = new ColumnLayout( this, 0 );
  m_pDefaultColumnLayout->setDefault();

  // No selection is active
  m_rctSelection.setCoords( 0, 0, 0, 0 );

  m_pWidget = new QWidget();
  m_pPainter = new QPainter;
  m_pPainter->begin( m_pWidget );

  m_iMaxColumn = 256;
  m_iMaxRow = 256;
  m_bScrollbarUpdates = true;

  setHidden(false);
  m_bShowGrid=true;
  m_bShowFormular=false;
  m_bLcMode=false;
  m_bShowColumnNumber=false;
  // Get a unique name so that we can offer scripting
  if ( !_name )
  {
      QCString s;
      s.sprintf("Table%i", s_id );
      QObject::setName( s.data() );
  }
}

bool KSpreadTable::isEmpty( unsigned long int x, unsigned long int y )
{
  KSpreadCell* c = cellAt( x, y );
  if ( !c || c->isEmpty() )
    return true;

  return false;
}

ColumnLayout* KSpreadTable::columnLayout( int _column )
{
    ColumnLayout *p = m_dctColumns[ _column ];
    if ( p != 0L )
	return p;

    return m_pDefaultColumnLayout;
}

RowLayout* KSpreadTable::rowLayout( int _row )
{
    RowLayout *p = m_dctRows[ _row ];
    if ( p != 0L )
	return p;

    return m_pDefaultRowLayout;
}

int KSpreadTable::leftColumn( int _xpos, int &_left, KSpreadCanvas *_canvas )
{
    if ( _canvas )
    {
	_xpos += _canvas->xOffset();
	_left = -_canvas->xOffset();
    }
    else
	_left = 0;

    int col = 1;
    int x = columnLayout( col )->width( _canvas );
    while ( x < _xpos )
    {
	// Should never happen
	if ( col == 0x10000 )
	    return 1;
	_left += columnLayout( col )->width( _canvas );
	col++;
	x += columnLayout( col )->width( _canvas );
    }

    return col;
}

int KSpreadTable::rightColumn( int _xpos, KSpreadCanvas *_canvas )
{
    if ( _canvas )
	_xpos += _canvas->xOffset();

    int col = 1;
    int x = 0;
    while ( x < _xpos )
    {
	// Should never happen
	if ( col == 0x10000 )
	    return 0x10000;
	x += columnLayout( col )->width( _canvas );
	col++;
    }

    return col;
}

int KSpreadTable::topRow( int _ypos, int & _top, KSpreadCanvas *_canvas )
{
    if ( _canvas )
    {
	_ypos += _canvas->yOffset();
	_top = -_canvas->yOffset();
    }
    else
	_top = 0;

    int row = 1;
    int y = rowLayout( row )->height( _canvas );
    while ( y < _ypos )
    {
	// Should never happen
	if ( row == 0x10000 )
	    return 1;
	_top += rowLayout( row )->height( _canvas );
	row++;
	y += rowLayout( row )->height( _canvas);
    }

    return row;
}

int KSpreadTable::bottomRow( int _ypos, KSpreadCanvas *_canvas )
{
    if ( _canvas )
	_ypos += _canvas->yOffset();

    int row = 1;
    int y = 0;
    while ( y < _ypos )
    {
	// Should never happen
	if ( row == 0x10000 )
	    return 0x10000;
	y += rowLayout( row )->height( _canvas );
	row++;
    }

    return row;
}

int KSpreadTable::columnPos( int _col, KSpreadCanvas *_canvas )
{
    int x = 0;
    if ( _canvas )
      x -= _canvas->xOffset();
    for ( int col = 1; col < _col; col++ )
    {
	// Should never happen
	if ( col == 0x10000 )
	    return x;
	
	x += columnLayout( col )->width( _canvas );
    }

    return x;
}

int KSpreadTable::rowPos( int _row, KSpreadCanvas *_canvas )
{
    int y = 0;
    if ( _canvas )
      y -= _canvas->yOffset();
    for ( int row = 1 ; row < _row ; row++ )
    {
	// Should never happen
	if ( row == 0x10000 )
	    return y;
	
	y += rowLayout( row )->height( _canvas );
    }

    return y;
}

KSpreadCell* KSpreadTable::visibleCellAt( int _column, int _row, bool _no_scrollbar_update )
{
  KSpreadCell* cell = cellAt( _column, _row, _no_scrollbar_update );
  if ( cell->isObscured() )
    return cellAt( cell->obscuringCellsColumn(), cell->obscuringCellsRow(), _no_scrollbar_update );

  return cell;
}

KSpreadCell* KSpreadTable::cellAt( int _column, int _row, bool _no_scrollbar_update )
{
  if ( !_no_scrollbar_update && m_bScrollbarUpdates )
  {
    if ( _column > m_iMaxColumn )
    {
      m_iMaxColumn = _column;
      emit sig_maxColumn( _column );
    }
    if ( _row > m_iMaxRow )
    {
      m_iMaxRow = _row;
      emit sig_maxRow( _row );
    }
  }

  int i = _row + ( _column * 0x10000 );

  KSpreadCell *p = m_dctCells[ i ];
  if ( p != 0L )
    return p;

  return m_pDefaultCell;
}

ColumnLayout* KSpreadTable::nonDefaultColumnLayout( int _column )
{
    ColumnLayout *p = m_dctColumns[ _column ];
    if ( p != 0L )
	return p;
	
    p = new ColumnLayout( this, _column );
    p->setWidth( m_pDefaultColumnLayout->width() );
    m_dctColumns.insert( _column, p );

    return p;
}

RowLayout* KSpreadTable::nonDefaultRowLayout( int _row )
{
    RowLayout *p = m_dctRows[ _row ];
    if ( p != 0L )
	return p;
	
    p = new RowLayout( this, _row );
    // TODO: copy the default RowLayout here!!
    p->setHeight( m_pDefaultRowLayout->height() );
    m_dctRows.insert( _row, p );

    return p;
}

KSpreadCell* KSpreadTable::nonDefaultCell( int _column, int _row,
					   bool _no_scrollbar_update )
{
  if ( !_no_scrollbar_update && m_bScrollbarUpdates )
  {
    if ( _column > m_iMaxColumn )
    {
      m_iMaxColumn = _column;
      emit sig_maxColumn( _column );
    }
    if ( _row > m_iMaxRow )
    {
      m_iMaxRow = _row;
      emit sig_maxRow( _row );
    }
  }

  int key = _row + ( _column * 0x10000 );

  KSpreadCell *p = m_dctCells[ key ];
  if ( p != 0L )
    return p;

  KSpreadCell *cell = new KSpreadCell( this, _column, _row );
  m_dctCells.insert( key, cell );

  return cell;
}

void KSpreadTable::setText( int _row, int _column, const QString& _text, bool updateDepends )
{
    m_pDoc->setModified( true );

    KSpreadCell *cell = nonDefaultCell( _column, _row );

    KSpreadUndoSetText *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	undo = new KSpreadUndoSetText( m_pDoc, this, cell->text(), _column, _row );
	m_pDoc->undoBuffer()->appendUndo( undo );
    }

    // The cell will force a display refresh itself, so we dont have to care here.
    cell->setCellText( _text, updateDepends );
}

void KSpreadTable::setLayoutDirtyFlag()
{
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
	it.current()->setLayoutDirtyFlag();
}

void KSpreadTable::setCalcDirtyFlag()
{
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
      it.current()->setCalcDirtyFlag();
	
}

void KSpreadTable::recalc(bool m_depend)
{
    kdDebug(36001) << "KSpreadTable::recalc(" << m_depend << ") STARTING" << endl;
    // First set all cells as dirty
    setCalcDirtyFlag();
    // Now recalc cells - it is important to do it AFTER, so that when
    // calculating one cell calculates many others, those are not done again.
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
	it.current()->calc(m_depend);
    kdDebug(36001) << "KSpreadTable::recalc(" << m_depend << ") DONE" << endl;
}

void KSpreadTable::setChooseRect( const QRect &_sel )
{
    if ( _sel == m_chooseRect )
	return;

    QRect old( m_chooseRect );
    m_chooseRect = _sel;

    emit sig_changeChooseSelection( this, old, m_chooseRect );
}

void KSpreadTable::unselect()
{
    if ( m_rctSelection.left() == 0 )
	return;

    QRect r = m_rctSelection;
    // Discard the selection
    m_rctSelection.setCoords( 0, 0, 0, 0 );

    emit sig_unselect( this, r );
}

void KSpreadTable::setSelection( const QRect &_sel, KSpreadCanvas *_canvas )
{
  if ( _sel == m_rctSelection )
    return;

  // We want to see whether a single cell was clicked like a button.
  // This is only of interest if no cell was selected before
  if ( _sel.left() == 0 )
  {
    // So we test first whether only a single cell was selected
    KSpreadCell *cell = cellAt( m_rctSelection.left(), m_rctSelection.top() );
    // Did we mark only a single cell ?
    // Take care: One cell may obscure other cells ( extra size! ).
    if ( m_rctSelection.left() + cell->extraXCells() == m_rctSelection.right() &&
	 m_rctSelection.top() + cell->extraYCells() == m_rctSelection.bottom() )
      cell->clicked( _canvas );
  }

  QRect old( m_rctSelection );
  m_rctSelection = _sel;

  emit sig_changeSelection( this, old, m_rctSelection );
}

void KSpreadTable::setSelectionFont( const QPoint &_marker, const char *_font, int _size,
				     signed char _bold, signed char _italic,signed char _underline,
                                     signed char _strike )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  if ( _font )
	    it.current()->setTextFontFamily( _font );
	  if ( _size > 0 )
	    it.current()->setTextFontSize( _size );
	  if ( _italic >= 0 )
	    	it.current()->setTextFontItalic( (bool)_italic );
	  if ( _bold >= 0 )
	    	it.current()->setTextFontBold( (bool)_bold );
	  if ( _underline >= 0 )
	    	it.current()->setTextFontUnderline( (bool)_underline );
         if ( _strike >= 0 )
	    	it.current()->setTextFontStrike( (bool)_strike );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  if ( _font )
	    it.current()->setTextFontFamily( _font );
	  if ( _size > 0 )
	    it.current()->setTextFontSize( _size );
	  if ( _italic >= 0 )
	    	it.current()->setTextFontItalic( (bool)_italic );
	  if ( _bold >= 0 )
	    	it.current()->setTextFontBold( (bool)_bold );
	  if ( _underline >= 0 )
	    	it.current()->setTextFontUnderline( (bool)_underline );
          if ( _strike >= 0 )
	    	it.current()->setTextFontStrike( (bool)_strike );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );

	      if ( cell == m_pDefaultCell )
	      {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	      }

	      cell->setDisplayDirtyFlag();

	      if ( _font )
		cell->setTextFontFamily( _font );
	      if ( _size > 0 )
		cell->setTextFontSize( _size );
	      if ( _italic >= 0 )
		  cell->setTextFontItalic( (bool)_italic );
	      if ( _bold >= 0 )
		  cell->setTextFontBold( (bool)_bold );
	      if ( _underline >= 0 )
	    	  cell->setTextFontUnderline( (bool)_underline );
              if ( _strike >= 0 )
	    	  cell->setTextFontStrike( (bool)_strike );
	      cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionSize( const QPoint &_marker,int _size )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setTextFontSize( (it.current()->textFontSize()+ _size));
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setTextFontSize( (it.current()->textFontSize()+ _size) );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );

	      if ( cell == m_pDefaultCell )
	      {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	      }

	      cell->setDisplayDirtyFlag();
	      cell->setTextFontSize( (cell->textFontSize()+ _size) );
	      cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionUpperLower( const QPoint &_marker,int _type )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() && !it.current()->isDefault()&& !it.current()->text().isEmpty()&&(it.current()->text().find('*')!=0)&&(it.current()->text().find('!')!=0))
	  	{
	  	it.current()->setDisplayDirtyFlag();
	  	if(_type==-1)
                  it.current()->setCellText( (it.current()->text().lower()));
	  	else if(_type==1)
                  it.current()->setCellText( (it.current()->text().upper()));
	  	it.current()->clearDisplayDirtyFlag();
	  	}
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() &&!it.current()->isDefault()&&!it.current()->text().isEmpty()	  &&(it.current()->text().find('*')!=0)&&(it.current()->text().find('!')!=0))
		{
	  	it.current()->setDisplayDirtyFlag();
	  	if(_type==-1)
	  		it.current()->setCellText( (it.current()->text().lower()));
	  	else if(_type==1)
	  		it.current()->setCellText( (it.current()->text().upper()));
	  	it.current()->clearDisplayDirtyFlag();
	  	}
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {		
	      KSpreadCell *cell = cellAt( x, y );
       	      if(!cell->isValue() && !cell->isBool() &&!cell->isFormular() &&!cell->isDefault()&&!cell->text().isEmpty()&&(cell->text().find('*')!=0)&&(cell->text().find('!')!=0))
		{

	      	cell->setDisplayDirtyFlag();
	      	if(_type==-1)
	  		cell->setCellText( (cell->text().lower()));
	  	else if(_type==1)
	  		cell->setCellText( (cell->text().upper()));
	      	cell->clearDisplayDirtyFlag();
	       	}
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionfirstLetterUpper( const QPoint &_marker)
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() && !it.current()->isDefault()&& !it.current()->text().isEmpty()&&(it.current()->text().find('*')!=0)&&(it.current()->text().find('!')!=0))
	  	{
	  	it.current()->setDisplayDirtyFlag();
                QString tmp=it.current()->text();
                int len=tmp.length();
                it.current()->setCellText( (tmp.at(0).upper()+tmp.right(len-1)));
	  	it.current()->clearDisplayDirtyFlag();
	  	}
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() &&!it.current()->isDefault()&&!it.current()->text().isEmpty()	  &&(it.current()->text().find('*')!=0)&&(it.current()->text().find('!')!=0))
		{
	  	it.current()->setDisplayDirtyFlag();
                QString tmp=it.current()->text();
                int len=tmp.length();
                it.current()->setCellText( (tmp.at(0).upper()+tmp.right(len-1)));
	  	it.current()->clearDisplayDirtyFlag();
	  	}
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );
       	      if(!cell->isValue() && !cell->isBool() &&!cell->isFormular() &&!cell->isDefault()&&!cell->text().isEmpty()&&(cell->text().find('*')!=0)&&(cell->text().find('!')!=0))
		{

	      	cell->setDisplayDirtyFlag();
                QString tmp=cell->text();
                int len=tmp.length();
                cell->setCellText( (tmp.at(0).upper()+tmp.right(len-1)));
	      	cell->clearDisplayDirtyFlag();
	       	}
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionVerticalText( const QPoint &_marker,bool _b)
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  	it.current()->setDisplayDirtyFlag();
                it.current()->setVerticalText(_b);
                it.current()->setMultiRow( false );
                it.current()->setAngle(0);
	  	it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	        it.current()->setDisplayDirtyFlag();
                it.current()->setVerticalText(_b);
                it.current()->setMultiRow( false );
                it.current()->setAngle(0);
                it.current()->clearDisplayDirtyFlag();

	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

        KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

        for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );
              if ( cell == m_pDefaultCell )
	        {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	        }
	       cell->setDisplayDirtyFlag();
               cell->setVerticalText(_b);
               cell->setMultiRow( false );
               cell->setAngle(0);
	       cell->clearDisplayDirtyFlag();

	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionComment( const QPoint &_marker,QString _comment)
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  	it.current()->setDisplayDirtyFlag();
                it.current()->setComment(_comment);
	  	it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	        it.current()->setDisplayDirtyFlag();
                it.current()->setComment(_comment);
                it.current()->clearDisplayDirtyFlag();

	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

        KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

        for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );
              if ( cell == m_pDefaultCell )
	        {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	        }
	       cell->setDisplayDirtyFlag();
               cell->setComment(_comment);
	       cell->clearDisplayDirtyFlag();

	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionAngle( const QPoint &_marker,int _value)
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  	it.current()->setDisplayDirtyFlag();
                it.current()->setAngle(_value);
                it.current()->setVerticalText(false);
                it.current()->setMultiRow( false );
	  	it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	        it.current()->setDisplayDirtyFlag();
                it.current()->setAngle(_value);
                it.current()->setVerticalText(false);
                it.current()->setMultiRow( false );
                it.current()->clearDisplayDirtyFlag();

	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

        KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

        for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );
              if ( cell == m_pDefaultCell )
	        {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	        }
	       cell->setDisplayDirtyFlag();
               cell->setAngle(_value);
               cell->setVerticalText(false);
               cell->setMultiRow( false );
	       cell->clearDisplayDirtyFlag();
	    }
	emit sig_updateView( this, r );
    }
}


void KSpreadTable::setSelectionRemoveComment( const QPoint &_marker)
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  	it.current()->setDisplayDirtyFlag();
                it.current()->setComment("");
	  	it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	        it.current()->setDisplayDirtyFlag();
                it.current()->setComment("");
                it.current()->clearDisplayDirtyFlag();

	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

        KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

        for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
	      KSpreadCell *cell = cellAt( x, y );
              if ( cell != m_pDefaultCell )
	        {
                cell->setDisplayDirtyFlag();
                cell->setComment("");
	        cell->clearDisplayDirtyFlag();
	        }


	    }

	emit sig_updateView( this, r );
    }
}


void KSpreadTable::setSelectionTextColor( const QPoint &_marker, QColor tb_Color )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setTextColor(tb_Color);
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setTextColor(tb_Color);
	
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
	
	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );
		
		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
		cell-> setTextColor(tb_Color);
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionbgColor( const QPoint &_marker, QColor bg_Color )
{
m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setBgColor(bg_Color);
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setBgColor(bg_Color);
	
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
		cell-> setBgColor(bg_Color);
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionBorderColor( const QPoint &_marker, QColor bd_Color )
{
    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  int it_Row=it.current()->row();
	  int it_Col=it.current()->column();	  		  if(it.current()->topBorderStyle(it_Row,it_Col)!=Qt::NoPen )
   	   	it.current()->setTopBorderColor( bd_Color );
   	  if(it.current()->leftBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setLeftBorderColor(bd_Color);
   	  if(it.current()->fallDiagonalStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setFallDiagonalColor(bd_Color);
   	  if(it.current()->goUpDiagonalStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setGoUpDiagonalColor(bd_Color);    	
	  if(it.current()->bottomBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setBottomBorderColor(bd_Color);
   	  if(it.current()->rightBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setRightBorderColor(bd_Color);
   	   	
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  int it_Row=it.current()->row();
	  int it_Col=it.current()->column();
	  if(it.current()->topBorderStyle(it_Row,it_Col)!=Qt::NoPen )
   	   	it.current()->setTopBorderColor( bd_Color );
   	  if(it.current()->leftBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setLeftBorderColor(bd_Color);
   	  if(it.current()->fallDiagonalStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setFallDiagonalColor(bd_Color);
   	  if(it.current()->goUpDiagonalStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setGoUpDiagonalColor(bd_Color);    	
	  if(it.current()->bottomBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setBottomBorderColor(bd_Color);
   	  if(it.current()->rightBorderStyle(it_Row,it_Col)!=Qt::NoPen)
   	   	it.current()->setRightBorderColor(bd_Color);
	
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
	
	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {		
		KSpreadCell *cell = cellAt( x, y );

		if ( cell != m_pDefaultCell )
           	{
			cell->setDisplayDirtyFlag();
           		if(cell->topBorderStyle(x,y)!=Qt::NoPen )
   	   			cell->setTopBorderColor( bd_Color );
   	   		if(cell->leftBorderStyle(x,y)!=Qt::NoPen)
   	   			cell->setLeftBorderColor(bd_Color);
   	   		if(cell->fallDiagonalStyle(x,y)!=Qt::NoPen)
   	   			cell->setFallDiagonalColor(bd_Color);
   	   		if(cell->goUpDiagonalStyle(x,y)!=Qt::NoPen)
   	   			cell->setGoUpDiagonalColor(bd_Color);    	
	   		if(cell->bottomBorderStyle(x,y)!=Qt::NoPen)
   	   			cell->setBottomBorderColor(bd_Color);
   	   		if(cell->rightBorderStyle(x,y)!=Qt::NoPen)
   	   			cell->setRightBorderColor(bd_Color);
    	   		cell->clearDisplayDirtyFlag();
    	   	}
		
		
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSeries( const QPoint &_marker,int start,int end,int step,Series mode,Series type)
{
m_pDoc->setModified( true );
QRect r(_marker.x(), _marker.y(), _marker.x(), _marker.y() );

int y = r.top();
int x = r.left();
int posx=0;
int posy=0;

for ( int incr=start;incr<=end; )
        {
	KSpreadCell *cell = cellAt( x+posx, y+posy );

	if ( cell == m_pDefaultCell )
		{
		cell = new KSpreadCell( this, x+posx, y+posy );
		int key = y+posy + ( (x+posx) * 0x10000 );
		m_dctCells.insert( key, cell );
		}
	QString tmp;
	cell->setCellText(tmp.setNum(incr));

	if(mode==Column)
	    posy++;
	else if(mode==Row)
	    posx++;
        else
            kdDebug(36001) << "Error in Series::mode" << endl;

        if(type==Linear)
	    incr=incr+step;
	else if(type==Geometric)
	    incr=incr*step;
        else
            kdDebug(36001) << "Error in Series::type" << endl;
	}

}



void KSpreadTable::setSelectionPercent( const QPoint &_marker ,bool b )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  if(!b)
	  	{
	  	it.current()->setFaktor( 1.0 );
	  	it.current()->setPrecision( 0 );
                it.current()->setFormatNumber(KSpreadCell::Number);
	  	}
	  else
		{
		it.current()->setFaktor( 100.0 );
	  	it.current()->setPrecision( 0 );
                it.current()->setFormatNumber(KSpreadCell::Percentage);
	  	}

	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  if(!b)
	  	{
	  	it.current()->setFaktor( 1.0 );
	  	it.current()->setPrecision( 0 );
                it.current()->setFormatNumber(KSpreadCell::Number);
	  	}
	  else
		{
		it.current()->setFaktor( 100.0 );
	  	it.current()->setPrecision( 0 );
                it.current()->setFormatNumber(KSpreadCell::Percentage);
	  	}
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();

		if(!b )
			{
			cell->setFaktor( 1.0 );
			cell->setPrecision( 0 );
                        cell->setFormatNumber(KSpreadCell::Number);
                        }
		else
			{
			cell->setFaktor( 100.0 );
			cell->setPrecision( 0 );
                        cell->setFormatNumber(KSpreadCell::Percentage);
			}
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::changeCellTabName(QString old_name,QString new_name)
{
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
    {
        if(it.current()->isFormular() || it.current()->content()==KSpreadCell::RichText)
        {
            if(it.current()->text().find(old_name)!=-1)
            {
                int nb = it.current()->text().contains(old_name+"!");
                QString tmp=old_name+"!";
                int len=tmp.length();
                tmp=it.current()->text();

                for(int i=0;i<nb;i++)
                {
                    int pos= tmp.find(old_name+"!");
                    tmp.replace(pos,len,new_name+"!");
                }
                it.current()->setCellText(tmp);
            }
        }
    }
}

void KSpreadTable::insertRightCell( const QPoint &_marker )
{
    m_dctCells.setAutoDelete( FALSE );

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    int count = 0;
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    // Determine right most column
    int max_column = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->column() > max_column )
	max_column = it.current()->column();
    }

    for ( int i = max_column; i >= _marker.x(); i-- )
    {
      for( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->column() == i && list[k]->row()==_marker.y() && !list[ k ]->isDefault() )
	{
	  kdDebug(36001) << "Moving Cell " << list[k]->column() << " " << list[k]->row() << endl;
	  int key = list[ k ]->row() | ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );
	
	  list[ k ]->setColumn( list[ k ]->column() + 1 );
		
	  key = list[ k ]->row() | ( list[ k ]->column() * 0x10000 );
	
	  m_dctCells.insert( key, list[ k ] );
	  list[ k ]->offsetAlign(list[k]->column(),list[k]->row());
	}
      }
    }


    m_dctCells.setAutoDelete( TRUE );

    m_pDoc->setModified( true );
    emit sig_updateView( this );
}

void KSpreadTable::insertBottomCell(const QPoint &_marker)
{
    m_dctCells.setAutoDelete( FALSE );

    kauto_array<KSpreadCell*> list(m_dctCells.count());
    int count = 0;
    // Find the last row
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    int max_row = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->row() > (int)max_row )
	max_row = it.current()->row();
    }

    for ( int i = max_row; i >= _marker.y(); i-- )
    {
      for( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->row() == i && list[ k ]->column()==_marker.x() && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );
	
	  list[ k ]->setRow( list[ k ]->row() + 1 );
		
	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	  list[ k ]->offsetAlign(list[k]->column(),list[k]->row());
	}
      }
    }

    m_dctCells.setAutoDelete( TRUE );

    emit sig_updateView( this );
}

void KSpreadTable::removeLeftCell(const QPoint &_marker)
{
    m_dctCells.setAutoDelete( FALSE );
    // Delete column
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    while( it.current() )
    {
	int key = it.current()->row() + ( it.current()->column() * 0x10000 );
	if ( it.current()->column() == _marker.x() && it.current()->row()==_marker.y() && !it.current()->isDefault() )
	{
	    KSpreadCell *cell = it.current();
	    m_dctCells.remove( key );
	    delete cell;
	
	}
	else
	{
	    ++it;
	}
    }

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    int count = 0;
    // Find right most cell
    it.toFirst();
    int max_column = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->column() > max_column )
	max_column = it.current()->column();
    }

    // Move cells
    for ( int i = _marker.x() + 1; i <= max_column; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->column() == i && list[k]->row()==_marker.y() && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );

	  list[ k ]->setColumn( list[ k ]->column() - 1 );
		
	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	}
      }
    }


    m_dctCells.setAutoDelete( TRUE );

    emit sig_updateView( this );
}

void KSpreadTable::removeTopCell(const QPoint &_marker)
{
    m_dctCells.setAutoDelete( FALSE );

    // Remove row
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    while( it.current() )
    {
	int key = it.current()->row() + ( it.current()->column() * 0x10000 );

	if ( it.current()->row() == _marker.y() && it.current()->column()==_marker.x() && !it.current()->isDefault() )
        {
	    KSpreadCell *cell = it.current();
	    m_dctCells.remove( key );
	    delete cell;
	}
	else
        {
	    ++it;
	}
    }

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    int count = 0;
    // Find last row
    it.toFirst();
    int max_row = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->row() > max_row )
	max_row = it.current()->row();
    }

    // Move rows below the deleted one upwards
    for ( int i = _marker.y() + 1; i <= max_row; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->row() == i && list[k]->column()==_marker.x() && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );

	  list[ k ]->setRow( list[ k ]->row() - 1 );

	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	}
      }
    }
    m_dctCells.setAutoDelete( true );

    emit sig_updateView( this );
}

void KSpreadTable::changeNameCellRef(const QPoint & pos, bool fullRowOrColumn, ChangeRef ref, QString tabname)
{
  bool correctDefaultTableName = (tabname == name()); // for cells without table ref (eg "A1")
  QIntDictIterator<KSpreadCell> it( m_dctCells );
  for ( ; it.current(); ++it )
  {
    if(it.current()->isFormular())
    {
      QString origText = it.current()->text();
      unsigned int i = 0;
      QString newText;

      bool correctTableName = correctDefaultTableName;
      //bool previousCorrectTableName = false;
      for ( ; i < origText.length(); ++i )
      {
        QChar origCh = origText[i];
	if ( origCh != ':' && origCh != '$' && !origCh.isLetter() )
	{
          newText += origCh;
          // Reset the "correct table indicator"
          correctTableName = correctDefaultTableName;
	}
	else // Letter or dollar : maybe start of cell name/range
          // (or even ':', like in a range - note that correctTable is kept in this case)
	{
          // Collect everything that forms a name (cell name or table name)
          QString str;
          for( ; i < origText.length() &&
                  (origText[i].isLetter() || origText[i].isDigit()
                   || origText[i].isSpace() || origText[i] == '$')
                   ; ++i )
            str += origText[i];

          // Was it a table name ?
          if ( origText[i] == '!' )
          {
            newText += str + '!'; // Copy it (and the '!')
            // Look for the table name right before that '!'
            correctTableName = ( newText.right( tabname.length()+1 ) == tabname+"!" );
          }
          else // It must be a cell identifier
          {
            // Parse it
            KSpreadPoint point( str );
            if (point.isValid())
            {
              int col = point.pos.x();
              int row = point.pos.y();
              // Update column
              if ( point.columnFixed )
                newText += '$' + util_columnLabel( col );
              else
              {
                if(ref==ColumnInsert
                   && correctTableName
                   && col>=pos.x()     // Column after the new one : +1
                   && ( fullRowOrColumn || row == pos.y() ) ) // All rows or just one
                {
                  newText += util_columnLabel(col+1);
                }
                else if(ref==ColumnRemove
                        && correctTableName
                        && col > pos.x() // Column after the deleted one : -1
                        && ( fullRowOrColumn || row == pos.y() ) ) // All rows or just one
                {
                  newText += util_columnLabel(col-1);
                }
                else
                  newText += util_columnLabel(col);
              }
              // Update row
              if ( point.rowFixed )
                newText += '$' + QString::number( row );
              else
              {
                if(ref==RowInsert
                   && correctTableName
                   && row >= pos.y() // Row after the new one : +1
                   && ( fullRowOrColumn || col == pos.x() ) ) // All columns or just one
                {
                  newText += QString::number( row+1 );
                }
                else if(ref==RowRemove
                        && correctTableName
                        && row > pos.y() // Column after the deleted one : -1
                        && ( fullRowOrColumn || col == pos.x() ) ) // All columns or just one
                {
                  newText += QString::number( row-1 );
                }
                else
                  newText += QString::number( row );
              }
            }
            else // Not a cell ref
            {
              //kdDebug(36001) << "Copying (unchanged) : " << str << endl;
              newText += str;
            }
            // Copy the char that got us to stop
            newText += origText[i];
          }
        }
      }
      it.current()->setCellText(newText, false /* no recalc deps for each, done independently */ );
    }
  }
}

#if 0
// Merged into the one above. Remove if it works.
void KSpreadTable::changeNameCellRef2(const QPoint & pos, ChangeRef ref,QString tabname)
{
  QIntDictIterator<KSpreadCell> it( m_dctCells );
  for ( ; it.current(); ++it )
  {
    if(it.current()->isFormular())
    {
      QString newText = "";

      // Should this be rewritten to use QString instead of char *
      // or is local8Bit ok ? (David)
      const char *origText = it.current()->text().local8Bit();
      char buf[ 2 ];
      buf[ 1 ] = 0;

      bool fixedRef1 = FALSE;
      bool fixedRef2 = FALSE;
      bool correctTableName;
      bool old_value=false;
      while ( *origText != 0 )
      {
	if ( *origText != '$' && !isalpha( *origText ) )
	{
          buf[0] = *origText++;
          newText += buf;
          correctTableName = (tabname==name());

          if(newText.right(1)=="!")
          {
            int pos=newText.findRev(tabname);
            if(pos!=-1)
            {
              int pos2=newText.length()-tabname.length()-1;
              if( newText.find(tabname,pos2)!=-1)
                correctTableName=true;
              else
                correctTableName=false;

            }
            else
              correctTableName=false;
            //when you write Table1!A1:A2
            //=>don't forget if it's the good name
            old_value=correctTableName;
          }
          else if(newText.right(1)==":")
          {
            correctTableName=old_value;
          }
          else
            old_value=correctTableName;
          fixedRef1 = fixedRef2 = FALSE;
	}
	else
	{
          QString newRef = "";
          if ( *origText == '$' )
          {
            newRef = "$";
            origText++;
            fixedRef1 = TRUE;
          }
          if ( isalpha( *origText ) )
          {
            char buffer[ 1024 ];
            while ( *origText && isalpha( *origText ) )
            {
              buf[ 0 ] = *origText;
              newRef += buf;
            }
            if ( *origText == '$' )
            {
              newRef += "$";
              origText++;
              fixedRef2 = TRUE;
            }
            if ( isdigit( *origText ) )
            {
              const char *p3 = origText;
              int row = atoi( origText );
              while ( *origText != 0 && isdigit( *origText ) ) origText++;
              // Is it a table
              if ( *origText == '!' )
              {
                newText += newRef;
                fixedRef1 = fixedRef2 = FALSE;
                origText = p3;

              }
              else // It must be a cell identifier
              {
                int col = 0;
                if ( strlen( buffer ) >= 2 )
                {
                  col += 26 * ( buffer[0] - 'A' + 1 );
                  col += buffer[1] - 'A' + 1;
                }
                else
                  col += buffer[0] - 'A' + 1;
                if ( fixedRef1 )
                  newText+="$"+util_columnLabel(col);
                else
                {

                  if(ref==ColumnInsert && correctTableName==true)
                  {
                    if(col >=pos.x()&&row==pos.y())
                      newText+=util_columnLabel(col+1);
                    else
                      newText+=util_columnLabel(col);
                  }
                  else if(ref==ColumnRemove && correctTableName==true)
                  {
                    if(col >pos.x() &&row==pos.y())
                      newText+=util_columnLabel(col-1);
                    else
                      newText+=util_columnLabel(col);
                  }
                  else
                  {
                    newText+=util_columnLabel(col);
                  }
                }
                if ( fixedRef2 )
                {
                  sprintf( buffer, "$%i", row );
                }
                else
                {
                  if(ref==RowInsert && correctTableName==true)
                  {
                    if(row >=pos.y()&&col==pos.x())
                      sprintf( buffer, "%i", (row+1)  );
                    else
                      sprintf( buffer, "%i", row  );
                  }
                  else if(ref==RowRemove && correctTableName==true)
                  {
                    if(row >pos.y()&&col==pos.x())
                      sprintf( buffer, "%i", (row-1)  );
                    else
                      sprintf( buffer, "%i", row  );
                  }
                  else
                  {
                    sprintf( buffer, "%i", row  );
                  }
                  newText += buffer;
                }
              }
            }
            else
            {
              newText += newRef;
              fixedRef1 = fixedRef2 = FALSE;
            }
          }
          else
          {
            newText += newRef;
            fixedRef1 = FALSE;
          }
        }
      }
      it.current()->setCellText(newText, false /* no recalc deps for each, done independently */ );
    }

  }
}
#endif


bool KSpreadTable::replace( const QPoint &_marker,QString _find,QString _replace, bool b_sensitive, bool b_whole )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );
    bool b_replace=false;
    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() &&!it.current()->isDefault()&&!it.current()->text().isEmpty())
		{
                QString text;
                if((text=replaceText(it.current()->text(), _find, _replace,b_sensitive,b_whole))!=it.current()->text())
                        {
                        it.current()->setDisplayDirtyFlag();
                        it.current()->setCellText(text);
                        it.current()->clearDisplayDirtyFlag();
                        b_replace=true;
                        }
		}
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return b_replace;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	   if(!it.current()->isValue() && !it.current()->isBool() &&!it.current()->isFormular() &&!it.current()->isDefault()&&!it.current()->text().isEmpty())
		{
                QString text;
                if((text=replaceText(it.current()->text(), _find, _replace,b_sensitive,b_whole))!=it.current()->text())
                        {
                        it.current()->setDisplayDirtyFlag();
                        it.current()->setCellText(text);
                        it.current()->clearDisplayDirtyFlag();
                        b_replace=true;
                        }
                }
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return b_replace;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}
                if(!cell->isValue() && !cell->isBool() &&!cell->isFormular() &&!cell->isDefault()&&!cell->text().isEmpty())
		{
                QString text;
                if((text=replaceText(cell->text(), _find, _replace,b_sensitive,b_whole))!=cell->text())
                        {
                        cell->setDisplayDirtyFlag();
                        cell->setCellText(text);
                        cell->clearDisplayDirtyFlag();
                        b_replace=true;
                        }
		}
	    }

	emit sig_updateView( this, r );
	return b_replace;
    }
}

QString KSpreadTable::replaceText(QString _cellText,QString _find,QString _replace, bool b_sensitive,bool b_whole)
{
QString findText;
QString replaceText;
QString realCellText;
int index=0;
int lenreplace=0;
int lenfind=0;
if(b_sensitive)
        {
        realCellText=_cellText;
        findText=_find;
        replaceText=_replace;
        if( realCellText.find(findText) >=0)
                {
                do
                        {
                        index=realCellText.find(findText,index+lenreplace);
                        int len=realCellText.length();
                        lenfind=findText.length();
                        lenreplace=replaceText.length();
                        if(!b_whole)
                                realCellText=realCellText.left(index)+replaceText+realCellText.right(len-index-lenfind);
                        else if(b_whole)
                                {
                                if (((index==0 || realCellText.mid(index-1,1)==" ")
                                        && (realCellText.mid(index+lenfind,1)==" "||(index+lenfind)==len)))
                                        realCellText=realCellText.left(index)+replaceText+realCellText.right(len-index-lenfind);
                                }
                        }
                while( realCellText.find(findText,index+lenreplace) >=0);
                }
        return realCellText;
        }
else
        {
        realCellText=_cellText;
        findText=_find.lower();
        replaceText=_replace;
        if( realCellText.lower().find(findText) >=0)
                {
                do
                        {
                        index=realCellText.lower().find(findText,index+lenreplace);
                        int len=realCellText.length();
                        lenfind=findText.length();
                        lenreplace=replaceText.length();
                        if(!b_whole)
                                realCellText=realCellText.left(index)+replaceText+realCellText.right(len-index-lenfind);
                        else if(b_whole)
                                {
                                if (((index==0 || realCellText.mid(index-1,1)==" ")
                                        && (realCellText.mid(index+lenfind,1)==" "||(index+lenfind)==len)))
                                        realCellText=realCellText.left(index)+replaceText+realCellText.right(len-index-lenfind);
                                }
                        }
                while( realCellText.lower().find(findText,index+lenreplace) >=0);
                }
        return realCellText;
        }
}

void KSpreadTable::borderBottom( const QPoint &_marker,QColor _color )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
    for ( int x = r.left(); x <= r.right(); x++ )
    {
	int y = r.bottom();
	KSpreadCell *cell = cellAt( x, y );
	if ( cell == m_pDefaultCell )
        {
	    cell = new KSpreadCell( this, x, y );
	    int key = y + ( x * 0x10000 );
	    m_dctCells.insert( key, cell );
	}
        cell->setBottomBorderStyle( SolidLine );
        cell->setBottomBorderColor( _color );
        cell->setBottomBorderWidth( 2 );
    }
    emit sig_updateView( this, r );
}

void KSpreadTable::borderRight( const QPoint &_marker,QColor _color )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
    for ( int y = r.top(); y <= r.bottom(); y++ )
    {
	int x = r.right();
	KSpreadCell *cell = cellAt( x, y );
	if ( cell == m_pDefaultCell )
        {
	    cell = new KSpreadCell( this, x, y );
	    int key = y + ( x * 0x10000 );
	    m_dctCells.insert( key, cell );
	}

  	cell->setRightBorderStyle( SolidLine );
   	cell->setRightBorderColor( _color );
    	cell->setRightBorderWidth( 2 );
    }
    emit sig_updateView( this, r );
}

void KSpreadTable::borderLeft( const QPoint &_marker,QColor _color )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
    for ( int y = r.top(); y <= r.bottom(); y++ )
    {
	int x = r.left();
	KSpreadCell *cell = cellAt( x, y );
	if ( cell == m_pDefaultCell )
        {
	    cell = new KSpreadCell( this, x, y );
	    int key = y + ( x * 0x10000 );
	    m_dctCells.insert( key, cell );
	}
  	cell->setLeftBorderStyle( SolidLine );
   	cell->setLeftBorderColor( _color );
    	cell->setLeftBorderWidth( 2 );
    }
    emit sig_updateView( this, r );
}

void KSpreadTable::borderTop( const QPoint &_marker,QColor _color )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
    for ( int x = r.left(); x <= r.right(); x++ )
    {
	int y = r.top();
	KSpreadCell *cell = cellAt( x, y );
	if ( cell == m_pDefaultCell )
        {
	    cell = new KSpreadCell( this, x, y );
	    int key = y + ( x * 0x10000 );
	    m_dctCells.insert( key, cell );
	}
  	cell->setTopBorderStyle( SolidLine );
   	cell->setTopBorderColor( _color );
    	cell->setTopBorderWidth( 2 );
    }
    emit sig_updateView( this, r );
}

void KSpreadTable::borderOutline( const QPoint &_marker,QColor _color )
{
    borderRight( _marker,_color);
    borderLeft(_marker,_color);
    borderTop(_marker,_color);
    borderBottom(_marker,_color);
}

void KSpreadTable::borderAll( const QPoint &_marker,QColor _color )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
	
    for ( int x = r.left(); x <= r.right(); x++ )
    {
	for(int y=r.top();y<=r.bottom();y++)
	{
	    KSpreadCell *cell = cellAt( x, y );
	    if ( cell == m_pDefaultCell )
	    {
		cell = new KSpreadCell( this, x, y );
		int key = y + ( x * 0x10000 );
		m_dctCells.insert( key, cell );
	    }
	    cell->setBottomBorderStyle( SolidLine );
	    cell->setBottomBorderColor( _color );
	    cell->setBottomBorderWidth( 2 );
	    cell->setRightBorderStyle( SolidLine );
	    cell->setRightBorderColor( _color );
	    cell->setRightBorderWidth( 2 );
	    cell->setLeftBorderStyle( SolidLine );
	    cell->setLeftBorderColor( _color );
	    cell->setLeftBorderWidth( 2 );
	    cell->setTopBorderStyle( SolidLine );
	    cell->setTopBorderColor( _color );
	    cell->setTopBorderWidth( 2 );
     	}
    }
    emit sig_updateView( this, r );
}

void KSpreadTable::borderRemove( const QPoint &_marker )
{
    QRect r( m_rctSelection );
    if ( m_rctSelection.left()==0 )
	r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    KSpreadUndoCellLayout *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

    for ( int x = r.left(); x <= r.right(); x++ )
    {
	for(int y=r.top();y<=r.bottom();y++)
        {
	    KSpreadCell *cell = cellAt( x, y );
	    cell->setBottomBorderStyle( NoPen );
	    cell->setBottomBorderColor( black );
	    cell->setBottomBorderWidth( 1 );
	    cell->setRightBorderStyle( NoPen );
	    cell->setRightBorderColor( black );
	    cell->setRightBorderWidth( 1 );
	    cell->setLeftBorderStyle( NoPen );
	    cell->setLeftBorderColor( black );
	    cell->setLeftBorderWidth( 1 );
	    cell->setTopBorderStyle( NoPen );
	    cell->setTopBorderColor( black );
	    cell->setTopBorderWidth( 1 );
	    cell->setFallDiagonalStyle( NoPen );
	    cell->setFallDiagonalColor( black );
	    cell->setFallDiagonalWidth( 1 );
	    cell->setGoUpDiagonalStyle( NoPen );
	    cell->setGoUpDiagonalColor( black );
	    cell->setGoUpDiagonalWidth( 1 );
	}

    }
    emit sig_updateView( this, r );
}


void KSpreadTable::sortByRow( int ref_row, SortingOrder mode )
{
    QRect r( selectionRect() );
    ASSERT( mode == Increase || mode == Decrease );

    // Sorting algorithm: David's :). Well, I guess it's called minmax or so.
    // For each row, we look for all columns under it and we find the one to swap with it.
    // Much faster than the awful bubbleSort...
    for ( int d = r.left();  d<= r.right(); d++ )
    {
	KSpreadCell *cell1 = cellAt( d,ref_row  );
        // Look for which column we want to swap with the one number d
        KSpreadCell * bestCell = cell1;
        int bestX = d;

        for ( int x = d + 1 ; x <= r.right(); x++ )
        {
          KSpreadCell *cell2 = cellAt( x,ref_row );

          // Here we use the operators < and > for cells, which do it all.
          if ( (mode==Increase && *cell2 < *bestCell) ||
               (mode==Decrease && *cell2 > *bestCell) )
          {
            bestCell = cell2;
            bestX = x;
          }
        }

        // Swap columns cell1 and bestCell (i.e. d and bestX)
        if ( d != bestX )
        {
          for(int y=r.top();y<=r.bottom();y++)
            swapCells( d,y,bestX,y );
        }

    }

    emit sig_updateView( this, r );
}

void KSpreadTable::sortByColumn(int ref_column,SortingOrder mode)
{
    //kdDebug() << "KSpreadTable::sortByColumn Ref_column=" << ref_column << endl;
    ASSERT( mode == Increase || mode == Decrease );
    QRect r( selectionRect() );

    // Sorting algorithm: David's :). Well, I guess it's called minmax or so.
    // For each row, we look for all rows under it and we find the one to swap with it.
    // Much faster than the awful bubbleSort...
    for ( int d = r.top(); d <= r.bottom(); d++ )
    {
        // Look for which row we want to swap with the one number d
	KSpreadCell *cell1 = cellAt( ref_column, d );
        //kdDebug() << "New ref row " << d << endl;

        KSpreadCell * bestCell = cell1;
        int bestY = d;

        for ( int y = d + 1 ; y <= r.bottom(); y++ )
        {
          KSpreadCell *cell2 = cellAt( ref_column, y );

          // Here we use the operators < and > for cells, which do it all.
          if ( (mode==Increase && *cell2 < *bestCell) ||
               (mode==Decrease && *cell2 > *bestCell) )
          {
            bestCell = cell2;
            bestY = y;
            //kdDebug() << "Best y now " << bestY << endl;
          }
        }

        // Swap rows cell1 and bestCell (i.e. d and bestY)
        if ( d != bestY )
        {
          //kdDebug() << "Swapping rows " << d << " and " << bestY << endl;
          for(int x=r.left();x<=r.right();x++)
            swapCells( x, d, x, bestY );
        }
    }

    emit sig_updateView( this, r );
}

void KSpreadTable::swapCells( int x1, int y1, int x2, int y2 )
{
  KSpreadCell *ref1 = cellAt( x1, y1 );
  KSpreadCell *ref2 = cellAt( x2, y2 );
  if ( ref1->isDefault() )
  {
    if ( !ref2->isDefault() )
    {
      ref1 = nonDefaultCell( x1, y1 );
      // TODO : make ref2 default instead of copying a default cell into it
    }
    else
      return; // nothing to do
  }
  else
    if ( ref2->isDefault() )
    {
      ref2 = nonDefaultCell( x2, y2 );
      // TODO : make ref1 default instead of copying a default cell into it
    }

  // Dummy cell used for swapping cells
  KSpreadCell *tmp = new KSpreadCell( this, -1, -1 );
  tmp->copyAll(ref1);
  ref1->copyAll(ref2);
  ref2->copyAll(tmp);
  delete tmp;
}

void KSpreadTable::setSelectionMultiRow( const QPoint &_marker, bool enable )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
          it.current()->setMultiRow( enable );
          it.current()->setVerticalText( false );
          it.current()->setAngle(0);
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
          it.current()->setMultiRow( enable );
          it.current()->setVerticalText( false );
          it.current()->setAngle(0);
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
                cell->setMultiRow( enable );
                cell->setVerticalText( false );
                cell->setAngle(0);
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionAlign( const QPoint &_marker, KSpreadLayout::Align _align )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setAlign( _align );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setAlign( _align );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
		cell->setAlign( _align );
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionAlignY( const QPoint &_marker, KSpreadLayout::AlignY _alignY )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setAlignY( _alignY );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  it.current()->setAlignY( _alignY );
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {		
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
		cell->setAlignY( _alignY );
		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}


void KSpreadTable::setSelectionPrecision( const QPoint &_marker, int _delta )
{
    m_pDoc->setModified( true );

    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
	  if ( _delta == 1 )
	    it.current()->incPrecision();
	  else
	    it.current()->decPrecision();
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
	  if ( _delta == 1 )
	    it.current()->incPrecision();
	  else
	    it.current()->decPrecision();
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
	
	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {		
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();

		if ( _delta == 1 )
		  cell->incPrecision();
		else
		  cell->decPrecision();

		cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::setSelectionMoneyFormat( const QPoint &_marker,bool b )
{
    m_pDoc->setModified( true );
    bool selected = ( m_rctSelection.left() != 0 );
    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setDisplayDirtyFlag();
          if(b)
                {
                it.current()->setFormatNumber(KSpreadCell::Money);
                it.current()->setFaktor( 1.0 );
                it.current()->setPrecision( KGlobal::locale()->fracDigits() );
                }
          else
                {
                it.current()->setFormatNumber(KSpreadCell::Number);
                it.current()->setFaktor( 1.0 );
                it.current()->setPrecision( 0 );
                }
	  it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();
          if(b)
                {
                it.current()->setFormatNumber(KSpreadCell::Money);
                it.current()->setFaktor( 1.0 );
                it.current()->setPrecision( KGlobal::locale()->fracDigits() );
                }
          else
                {
                it.current()->setFormatNumber(KSpreadCell::Number);
                it.current()->setFaktor( 1.0 );
                it.current()->setPrecision( 0 );
                }
          it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
                if(b)
                        {
                        cell->setFormatNumber(KSpreadCell::Money);
                        cell->setFaktor( 1.0 );
                        cell->setPrecision( KGlobal::locale()->fracDigits() );
                        }
                else
                        {
                        cell->setFormatNumber(KSpreadCell::Number);
                        cell->setFaktor( 1.0 );
                        cell->setPrecision( 0 );
                        }
                cell->clearDisplayDirtyFlag();
	    	}

	emit sig_updateView( this, r );
    }
}

int KSpreadTable::adjustColumn( const QPoint& _marker, int _col )
{
    int long_max=0;
    if( _col == -1 )
    {
	if ( m_rctSelection.left() != 0 && m_rctSelection.bottom() == 0x7FFF )
        {
	    QIntDictIterator<KSpreadCell> it( m_dctCells );
	    for ( ; it.current(); ++it )
      	    {
		long l = it.currentKey();
		int col = l >> 16;
		if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
		{
		    if( !it.current()->isEmpty() )
		    {

	                if( it.current()->textWidth() > long_max )
                                {
                                long_max = it.current()->textWidth() +
				       it.current()->leftBorderWidth(it.current()->column(),it.current()->row() ) +
				       it.current()->rightBorderWidth(it.current()->column(),it.current()->row() );
                                }

		    }
		}
	    }
	}

    }
    else
    {
	QRect r( m_rctSelection );
	if( r.left() == 0 || r.right() == 0 || r.top() == 0 || r.bottom() == 0 )
	{
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
	}
	int x = _col;
	for ( int y = r.top(); y <= r.bottom(); y++ )
	{
	    KSpreadCell *cell = cellAt( x, y );
	    if( cell != m_pDefaultCell && !cell->isEmpty() )
	    {

                   if(cell->textWidth() > long_max )
                                {
                                long_max = cell->textWidth() +
			        cell->leftBorderWidth(cell->column(),cell->row() ) +
			        cell->rightBorderWidth(cell->column(),cell->row() );
                                }

	    }
	}


    }
    //add 4 because long_max is the long of the text
    //but column has borders
    if( long_max == 0 )
	return -1;
    else
	return ( long_max + 4 );
}

int KSpreadTable::adjustRow(const QPoint &_marker,int _row)
{
    int long_max=0;
    if(_row==-1)
    {
	if ( m_rctSelection.left() != 0 && m_rctSelection.right() == 0x7FFF )
        {
	    QIntDictIterator<KSpreadCell> it( m_dctCells );
	    for ( ; it.current(); ++it )
	    {
		long l = it.currentKey();
		int row = l & 0xFFFF;
		if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
		{
		    if(!it.current()->isEmpty() )
		    {

	  		if(it.current()->textHeight()>long_max)
			    long_max = it.current()->textHeight() +
				       it.current()->topBorderWidth(it.current()->column(),it.current()->row() ) +
				       it.current()->bottomBorderWidth(it.current()->column(),it.current()->row() );

		    }
		}
	    }
	}
    }
    else
    {
	QRect r( m_rctSelection );
	if( r.left() == 0 || r.right() == 0 || r.top() == 0 || r.bottom() == 0 )
	{
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
	}
	int y=_row;
	for ( int x = r.left(); x <= r.right(); x++ )
	{
	    KSpreadCell *cell = cellAt( x, y );
	    if(cell != m_pDefaultCell && !cell->isEmpty())
	    {

	 	if(cell->textHeight()>long_max)
		    long_max = cell->textHeight() +
			       cell->topBorderWidth(cell->column(),cell->row() ) +
			       cell->bottomBorderWidth(cell->column(),cell->row() );


	    }
	}
    }

    //add 4 because long_max is the long of the text
    //but column has borders
    if( long_max == 0 )
	return -1;
    else
	return ( long_max + 4 );
}

void KSpreadTable::clearSelection( const QPoint &_marker )
{
    m_pDoc->setModified( true );
    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->setCellText("");
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setCellText("");
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	
	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setCellText("");
            }

	emit sig_updateView( this, r );
    }
}

void KSpreadTable::defaultSelection( const QPoint &_marker )
{
    m_pDoc->setModified( true );
    bool selected = ( m_rctSelection.left() != 0 );

    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
	  it.current()->defaultStyle();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->defaultStyle();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );

	KSpreadUndoCellLayout *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    undo = new KSpreadUndoCellLayout( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}
	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->defaultStyle();
            }

	emit sig_updateView( this, r );
    }
}


void KSpreadTable::insertRow( unsigned long int _row )
{
    KSpreadUndoInsertRow *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	undo = new KSpreadUndoInsertRow( m_pDoc, this, _row );
	m_pDoc->undoBuffer()->appendUndo( undo );
    }

    // We want to remove cells without deleting them
    m_dctCells.setAutoDelete( FALSE );
    m_dctRows.setAutoDelete( FALSE );

    /**
     * Shift the cells to the right
     */

    // Find the last row and create a list
    // of all cells
    kauto_array<KSpreadCell*> list(m_dctCells.count());
    unsigned long int count = 0;
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    unsigned long int max_row = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->row() > (int)max_row )
	max_row = it.current()->row();
    }

    // Go from the right most row in left direction ....
    for ( unsigned long int i = max_row; i >= _row; i-- )
    {
      // Iterate over all cells and move cells which are in row i
      // one row to the right
      for( unsigned long int k = 0; k < count; k++ )
      {
	if ( list[ k ]->row() == (int)i && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );
	
	  list[ k ]->setRow( list[ k ]->row() + 1 );
		
	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	}
      }
    }

    /**
     * Shift the row layouts to the right
     */
    kauto_array<RowLayout*> list2(m_dctRows.count());
    count = 0;
    QIntDictIterator<RowLayout> it2( m_dctRows );
    max_row = 1;
    for ( ; it2.current(); ++it2 )
    {
      list2[ count++ ] = it2.current();
      if ( it2.current()->row() > (int)max_row )
	max_row = it2.current()->row();
    }

    for ( unsigned long int i = max_row; i >= _row; i-- )
    {
      for( unsigned long int k = 0; k < count; k++ )
      {
	if ( list2[ k ]->row() == (int)i )
	{
	  int key = list2[ k ]->row();
	  m_dctRows.remove( key );

	  list2[ k ]->setRow( list2[ k ]->row() + 1 );
		
	  key = list2[ k ]->row();
	  m_dctRows.insert( key, list2[ k ] );
	}
      }
    }
    m_pDoc->setModified( true );

    // Reset to normal behaviour
    m_dctCells.setAutoDelete( TRUE );
    m_dctRows.setAutoDelete( TRUE );

    // Update the view and borders
    emit sig_updateView( this );
    emit sig_updateHBorder( this );
    emit sig_updateVBorder( this );
}

void KSpreadTable::deleteRow( unsigned long int _row )
{
    KSpreadUndoDeleteRow *undo = 0L;
    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	undo = new KSpreadUndoDeleteRow( m_pDoc, this, _row );
	m_pDoc->undoBuffer()->appendUndo( undo  );
    }

    m_dctCells.setAutoDelete( FALSE );
    m_dctRows.setAutoDelete( FALSE );

    // Remove row
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); )
    {
	int key = it.current()->row() + ( it.current()->column() * 0x10000 );

	if ( it.current()->row() == (int)_row && !it.current()->isDefault() )
	{
	    KSpreadCell *cell = it.current();
	    m_dctCells.remove( key );
	    if ( undo )
	      undo->appendCell( cell );
	    else
	      delete cell;
	}
	else
	{
	++it;
	}
    }

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    int count = 0;
    // Find last row
    it.toFirst();
    int max_row = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->row() > max_row )
	max_row = it.current()->row();
    }

    // Move rows below the deleted one upwards
    for ( int i = _row + 1; i <= max_row; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->row() == i && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );
		
	  list[ k ]->setRow( list[ k ]->row() - 1 );
		
	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	}
      }
    }


    // Delete RowLayout
    QIntDictIterator<RowLayout> it2( m_dctRows );
    for ( ; it2.current(); ++it2 )
    {
	int key = it2.current()->row();
	if ( it2.current()->row() == (int)_row && !it2.current()->isDefault() )
	{
	    RowLayout *l = it2.current();
	    m_dctRows.remove( key );
	    if ( undo )
	      undo->setRowLayout( l );
	    else
	      delete l;
	}
    }

    kauto_array<RowLayout*> list2( m_dctRows.count());
    count = 0;
    // Find last RowLayout
    it2.toFirst();
    max_row = 1;
    for ( ; it2.current(); ++it2 )
    {
      list2[ count++ ] = it2.current();
      if ( it2.current()->row() > max_row )
	max_row = it2.current()->row();
    }

    for ( int i = _row + 1; i <= max_row; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list2[ k ]->row() == i && !list2[ k ]->isDefault() )
	{
	  int key = list2[ k ]->row();
	  m_dctRows.remove( key );

	  list2[ k ]->setRow( list2[ k ]->row() - 1 );
		
	  key = list2[ k ]->row();
	  m_dctRows.insert( key, list2[ k ] );
	}
      }
    }

    m_pDoc->setModified( true );

    m_dctCells.setAutoDelete( true );
    m_dctRows.setAutoDelete( true );

    emit sig_updateView( this );
    emit sig_updateHBorder( this );
    emit sig_updateVBorder( this );
}

void KSpreadTable::insertColumn( unsigned long int _column )
{
    KSpreadUndoInsertColumn *undo;
    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	undo = new KSpreadUndoInsertColumn( m_pDoc, this, _column );
	m_pDoc->undoBuffer()->appendUndo( undo  );
    }

    m_dctCells.setAutoDelete( FALSE );
    m_dctColumns.setAutoDelete( FALSE );

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    unsigned long int count = 0;
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    // Determine right most column
    unsigned long int max_column = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->column() > (int)max_column )
	max_column = it.current()->column();
    }

    for ( unsigned long int i = max_column; i >= _column; i-- )
    {
      for( unsigned long int k = 0; k < count; k++ )
      {
	if ( list[ k ]->column() == (int)i && !list[ k ]->isDefault() )
	{
	
	kdDebug(36001) << "Moving Cell " << list[k]->column() << " " << list[k]->row() << endl;
	int key = list[ k ]->row() | ( list[ k ]->column() * 0x10000 );
	m_dctCells.remove( key );
	
 	list[ k ]->setColumn( list[ k ]->column() + 1 );

	key = list[ k ]->row() | ( list[ k ]->column() * 0x10000 );
	m_dctCells.insert( key, list[ k ] );

	}
      }
    }

    kauto_array<ColumnLayout*> list2(m_dctColumns.count());
    count = 0;
    // Find right most ColumnLayout
    QIntDictIterator<ColumnLayout> it2( m_dctColumns );
    max_column = 1;
    for ( ; it2.current(); ++it2 )
    {
      list2[ count++ ] = it2.current();
      if ( it2.current()->column() > (int)max_column )
	max_column = it2.current()->column();
    }

    for ( unsigned long int i = max_column; i >= _column; i-- )
    {
      for( unsigned long int k = 0; k < count; k++ )
      {
	if ( list2[ k ]->column() == (int)i )
	{
	  int key = list2[ k ]->column();
	  m_dctColumns.remove( key );
	  list2[k]->setColumn( list2[ k ]->column() + 1 );
		
	  key = list2[k]->column();
	  m_dctColumns.insert( key, list2[k] );

	}
      }
    }

    m_pDoc->setModified( true );

    m_dctCells.setAutoDelete( TRUE );
    m_dctColumns.setAutoDelete( TRUE );

    emit sig_updateView( this );
    emit sig_updateHBorder( this );
    emit sig_updateVBorder( this );
}

void KSpreadTable::deleteColumn( unsigned long int _column )
{
    KSpreadUndoDeleteColumn *undo = 0L;
    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	undo = new KSpreadUndoDeleteColumn( m_pDoc, this, _column );
	m_pDoc->undoBuffer()->appendUndo( undo  );
    }

    m_dctCells.setAutoDelete( FALSE );
    m_dctColumns.setAutoDelete( FALSE );

    // Delete column
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current();  )
    {
	int key = it.current()->row() + ( it.current()->column() * 0x10000 );

	if ( it.current()->column() == (int)_column && !it.current()->isDefault() )
	{
	    KSpreadCell *cell = it.current();
	    m_dctCells.remove( key );
	    if ( undo )
	      undo->appendCell( cell );
	    else
	      delete cell;
	}
	else
	{
	++it;
	}
    }

    kauto_array<KSpreadCell*> list( m_dctCells.count());
    int count = 0;
    // Find right most cell
    it.toFirst();
    int max_column = 1;
    for ( ; it.current(); ++it )
    {
      list[ count++ ] = it.current();
      if ( it.current()->column() > max_column )
	max_column = it.current()->column();
    }

    // Move cells
    for ( int i = _column + 1; i <= max_column; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list[ k ]->column() == i && !list[ k ]->isDefault() )
	{
	  int key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.remove( key );
	
	  list[ k ]->setColumn( list[ k ]->column() - 1 );

	  key = list[ k ]->row() + ( list[ k ]->column() * 0x10000 );
	  m_dctCells.insert( key, list[ k ] );
	}
      }
    }

    // Delete ColumnLayout
    QIntDictIterator<ColumnLayout> it2( m_dctColumns );
    for ( ; it2.current(); ++it2 )
    {
	int key = it2.current()->column();
	if ( it2.current()->column() == (int)_column && !it2.current()->isDefault() )
	{
	    ColumnLayout *l = it2.current();
	    m_dctColumns.remove( key );
	    if ( undo )
	      undo->setColumnLayout( l );
	    else
	      delete l;
	}
    }

    kauto_array<ColumnLayout*> list2(m_dctColumns.count());
    count = 0;
    // Move ColumnLayouts
    it2.toFirst();
    max_column = 1;
    for ( ; it2.current(); ++it2 )
    {
      list2[ count++ ] = it2.current();	
      if ( it2.current()->column() > max_column )
	max_column = it2.current()->column();
    }

    for ( int i = _column + 1; i <= max_column; i++ )
    {
      for ( int k = 0; k < count; k++ )
      {
	if ( list2[ k ]->column() == i && !list2[ k ]->isDefault() )
	{
	  int key = list2[ k ]->column();
	  m_dctColumns.remove( key );

	  list2[ k ]->setColumn( list2[ k ]->column() - 1 );
		
	  key = list2[ k ]->column();
	  m_dctColumns.insert( key, list2[ k ] );
	}
      }
    }

    m_pDoc->setModified( true );

    m_dctCells.setAutoDelete( TRUE );
    m_dctColumns.setAutoDelete( TRUE );

    emit sig_updateView( this );
    emit sig_updateHBorder( this );
    emit sig_updateVBorder( this );
}

void KSpreadTable::setConditional( const QPoint &_marker,KSpreadConditional tmp[3] )
{
m_pDoc->setModified( true );
KSpreadConditional *tmpCondition=0;

    bool selected = ( m_rctSelection.left() != 0 );
    // Complete rows selected ?
    if ( selected && m_rctSelection.right() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
          it.current()->setDisplayDirtyFlag();
            for(int i=0;i<3;i++)
                {
                 switch(i)
				{
				case 0:
					
					if(tmp[i].m_cond==None)
						it.current()->removeFirstCondition();
					else
						{
						tmpCondition=it.current()->getFirstCondition();
                				tmpCondition->val1=tmp[i].val1;
                				tmpCondition->val2=tmp[i].val2;
                				tmpCondition->colorcond=tmp[i].colorcond;
                				tmpCondition->fontcond=tmp[i].fontcond;
                				tmpCondition->m_cond=tmp[i].m_cond;
						}
					break;
				case 1:
					
					if(tmp[i].m_cond==None)
						it.current()->removeSecondCondition();
					else
						{
						tmpCondition=it.current()->getSecondCondition();
                				tmpCondition->val1=tmp[i].val1;
                				tmpCondition->val2=tmp[i].val2;
                				tmpCondition->colorcond=tmp[i].colorcond;
                				tmpCondition->fontcond=tmp[i].fontcond;
                				tmpCondition->m_cond=tmp[i].m_cond;
						}

					break;
				case 2:
					
					if(tmp[i].m_cond==None)
						it.current()->removeThirdCondition();
					else
						{
						tmpCondition=it.current()->getThirdCondition();
                				tmpCondition->val1=tmp[i].val1;
                				tmpCondition->val2=tmp[i].val2;
                				tmpCondition->colorcond=tmp[i].colorcond;
                				tmpCondition->fontcond=tmp[i].fontcond;
                				tmpCondition->m_cond=tmp[i].m_cond;
						}

					break;
				}
		 }
	  it.current()->clearDisplayDirtyFlag();
	
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    // Complete columns selected ?
    else if ( selected && m_rctSelection.bottom() == 0x7FFF )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
	  it.current()->setDisplayDirtyFlag();

          for(int i=0;i<3;i++)
                {
                switch(i)
				{	
   	             	case 0:

					if(tmp[i].m_cond==None)
						it.current()->removeFirstCondition();
					else
						{
						tmpCondition=it.current()->getFirstCondition();
    	           				tmpCondition->val1=tmp[i].val1;
     	          				tmpCondition->val2=tmp[i].val2;
      	         				tmpCondition->colorcond=tmp[i].colorcond;
       	        				tmpCondition->fontcond=tmp[i].fontcond;
        	       			tmpCondition->m_cond=tmp[i].m_cond;
						}
					break;
				case 1:
					
					if(tmp[i].m_cond==None)
						it.current()->removeSecondCondition();
					else
						{
					  	tmpCondition=it.current()->getSecondCondition();
  	          				tmpCondition->val1=tmp[i].val1;
   	             			tmpCondition->val2=tmp[i].val2;
    	            			tmpCondition->colorcond=tmp[i].colorcond;
    	            			tmpCondition->fontcond=tmp[i].fontcond;
     	          				tmpCondition->m_cond=tmp[i].m_cond;
						}	
					break;
				case 2:

					if(tmp[i].m_cond==None)
						it.current()->removeThirdCondition();
					else
						{
				    	tmpCondition=it.current()->getThirdCondition();
   	           				tmpCondition->val1=tmp[i].val1;
    	            			tmpCondition->val2=tmp[i].val2;
     		           		tmpCondition->colorcond=tmp[i].colorcond;
     	 	         		tmpCondition->fontcond=tmp[i].fontcond;
        	       			tmpCondition->m_cond=tmp[i].m_cond;
						}	

					break;
  			}	
                }
          it.current()->clearDisplayDirtyFlag();
	}
      }

      emit sig_updateView( this, m_rctSelection );
      return;
    }
    else
    {
	QRect r( m_rctSelection );
	if ( !selected )
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );


	for ( int x = r.left(); x <= r.right(); x++ )
	    for ( int y = r.top(); y <= r.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell == m_pDefaultCell )
		{
		    cell = new KSpreadCell( this, x, y );
		    int key = y + ( x * 0x10000 );
		    m_dctCells.insert( key, cell );
		}

		cell->setDisplayDirtyFlag();
                for(int i=0;i<3;i++)
                        {
                	switch(i)
				{	
   	             	case 0:
					
					if(tmp[i].m_cond==None)
						cell->removeFirstCondition();
					else
						{
    						tmpCondition=cell->getFirstCondition();
    	           				tmpCondition->val1=tmp[i].val1;
     	          				tmpCondition->val2=tmp[i].val2;
      	         				tmpCondition->colorcond=tmp[i].colorcond;
       	        				tmpCondition->fontcond=tmp[i].fontcond;
        	       			tmpCondition->m_cond=tmp[i].m_cond;
						}
					break;
				case 1:

					if(tmp[i].m_cond==None)
						cell->removeSecondCondition();
					else
						{
						tmpCondition=cell->getSecondCondition();
  	          				tmpCondition->val1=tmp[i].val1;
   	             			tmpCondition->val2=tmp[i].val2;
    	            			tmpCondition->colorcond=tmp[i].colorcond;
    	            			tmpCondition->fontcond=tmp[i].fontcond;
     	          				tmpCondition->m_cond=tmp[i].m_cond;
						}	
					break;
				case 2:
					
					if(tmp[i].m_cond==None)
						cell->removeThirdCondition();
					else
						{
						tmpCondition=cell->getThirdCondition();
   	           				tmpCondition->val1=tmp[i].val1;
    	            			tmpCondition->val2=tmp[i].val2;
     		           		tmpCondition->colorcond=tmp[i].colorcond;
     	 	         		tmpCondition->fontcond=tmp[i].fontcond;
        	       			tmpCondition->m_cond=tmp[i].m_cond;
						}	

					break;
                        }
                    }
                    cell->clearDisplayDirtyFlag();
	    }

	emit sig_updateView( this, r );
    }
}


void KSpreadTable::copySelection( const QPoint &_marker )
{
    QRect rct;

    // No selection ? => copy active cell
    if ( m_rctSelection.left() == 0 )
	rct.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    else
	rct = selectionRect();
	
    // Save to buffer
    QDomDocument doc = saveCellRect( rct );

    QBuffer buffer;
    buffer.open( IO_WriteOnly );
    QTextStream str( &buffer );
    str << doc;
    buffer.close();

    QStoredDrag* data = new QStoredDrag( "application/x-kspread-snippet" );
    data->setEncodedData( buffer.buffer() );

    QApplication::clipboard()->setData( data );
}

void KSpreadTable::cutSelection( const QPoint &_marker )
{
    m_pDoc->setModified( true );

    copySelection( _marker );
    deleteSelection( _marker );
}

void KSpreadTable::paste( const QPoint &_marker, PasteMode sp, Operation op )
{
    QMimeSource* mime = QApplication::clipboard()->data();
    if ( !mime || !mime->provides( "application/x-kspread-snippet" ) )
	return;

    QByteArray b = mime->encodedData( "application/x-kspread-snippet" );

    kdDebug(36001) << "Parsing " << b.size() << " bytes" << endl;

    QBuffer buffer( b );
    buffer.open( IO_ReadOnly );
    QDomDocument doc;
    doc.setContent( &buffer );
    buffer.close();

    // TODO: Test for parsing errors

    loadSelection( doc, _marker.x() - 1, _marker.y() - 1, sp, op );
    m_pDoc->setModified( true );
    emit sig_updateView( this );
}

bool KSpreadTable::loadSelection( const QDomDocument& doc, int _xshift, int _yshift, PasteMode sp, Operation op )
{
    QDomElement e = doc.documentElement();

    QDomElement c = e.firstChild().toElement();
    for( ; !c.isNull(); c = c.nextSibling().toElement() )
    {
	if ( c.tagName() == "cell" )
        {
	    int row = c.attribute( "row" ).toInt() + _yshift;
	    int col = c.attribute( "column" ).toInt() + _xshift;

	    bool needInsert = FALSE;
	    KSpreadCell* cell = cellAt( col, row );
	    if ( ( op == OverWrite && sp == Normal ) || cell->isDefault() )
	    {
		cell = new KSpreadCell( this, 0, 0 );
		needInsert = TRUE;
	    }
	    if ( !cell->load( c, _xshift, _yshift, sp, op ) )
            {
                if ( needInsert )
                  delete cell;
            }
	    else
              if ( needInsert )
		insertCell( cell );
	}
        else if ( (c.tagName() == "right-most-border")&& ( (sp == Normal) || (sp == Format)) )
        {
	    int row = c.attribute( "row" ).toInt() + _yshift;
	    int col = c.attribute( "column" ).toInt() + _xshift;

	    bool needInsert = FALSE;
	    KSpreadCell* cell = nonDefaultCell( col, row );
	    if ( !cell )
	    {
		cell = new KSpreadCell( this, 0, 0 );
		needInsert = TRUE;
	    }
	    if ( !cell->loadRightMostBorder( c, _xshift, _yshift ) )
            {
                if ( needInsert )
                  delete cell;
            }
	    else
              if ( needInsert )
		insertCell( cell );
	}
	else if ( (c.tagName() == "bottom-most-border")&& ((sp == Normal) || (sp == Format)))
        {
	    int row = c.attribute( "row" ).toInt() + _yshift;
	    int col = c.attribute( "column" ).toInt() + _xshift;

	    bool needInsert = FALSE;
	    KSpreadCell* cell = nonDefaultCell( col, row );
	    if ( !cell )
	    {
		cell = new KSpreadCell( this, 0, 0 );
		needInsert = TRUE;
	    }

            if ( !cell->loadBottomMostBorder( c, _xshift, _yshift ) )
            {
              if ( needInsert )
                delete cell;
            }
	    else
              if ( needInsert )
		insertCell( cell );
	}
    }

    m_pDoc->setModified( true );

    return true;
}

void KSpreadTable::deleteCells( int _left, int _top, int _right, int _bottom )
{
    // A list of all cells we want to delete.
    QStack<KSpreadCell> cellStack;
    // cellStack.setAutoDelete( TRUE );

    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
    {
	if ( !it.current()->isDefault() && it.current()->row() >= _top &&
	     it.current()->row() <= _bottom && it.current()->column() >= _left &&
	     it.current()->column() <= _right )
	  cellStack.push( it.current() );
    }

    m_dctCells.setAutoDelete( false );
    // Remove the cells from the table
    while ( !cellStack.isEmpty() )
    {
	KSpreadCell *cell = cellStack.pop();

	int key = cell->row() + ( cell->column() * 0x10000 );
	m_dctCells.remove( key );
	cell->updateDepending();

	delete cell;
    }
    m_dctCells.setAutoDelete( true );

    setLayoutDirtyFlag();

    QIntDictIterator<KSpreadCell> it2( m_dctCells );
    for ( ; it2.current(); ++it2 )
      if ( it2.current()->isForceExtraCells() && !it2.current()->isDefault() )
	it2.current()->forceExtraCells( it2.current()->column(), it2.current()->row(),
					it2.current()->extraXCells(), it2.current()->extraYCells() );
}

void KSpreadTable::deleteSelection( const QPoint &_marker )
{
    m_pDoc->setModified( true );

    if ( m_rctSelection.left() == 0 )
    {
	KSpreadUndoDelete *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    QRect r;
	    r.setCoords( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
	    undo = new KSpreadUndoDelete( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	deleteCells( _marker.x(), _marker.y(), _marker.x(), _marker.y() );
    }
    else if ( m_rctSelection.right() == 0x7fff )
    {
      // TODO
    }
    else if ( m_rctSelection.bottom() == 0x7fff )
    {
      // TODO
    }
    else
    {
	KSpreadUndoDelete *undo;
	if ( !m_pDoc->undoBuffer()->isLocked() )
	{
	    QRect r;
	    r.setCoords( m_rctSelection.left(), m_rctSelection.top(),
			 m_rctSelection.right(), m_rctSelection.bottom() );
	    undo = new KSpreadUndoDelete( m_pDoc, this, r );
	    m_pDoc->undoBuffer()->appendUndo( undo );
	}

	deleteCells( m_rctSelection.left(), m_rctSelection.top(),
		     m_rctSelection.right(), m_rctSelection.bottom() );
    }

    emit sig_updateView( this );
}


void KSpreadTable::mergeCell( const QPoint &_marker)
{
if(m_rctSelection.left() == 0)
        return;
int x=_marker.x();
int y=_marker.y();
if(_marker.x()>m_rctSelection.left())
        x=m_rctSelection.left();
if(_marker.y()>m_rctSelection.top())
        y=m_rctSelection.top();
KSpreadCell *cell = nonDefaultCell(x ,y  );

cell->forceExtraCells( x ,y,
                           abs(m_rctSelection.right() -m_rctSelection.left()),
                           abs(m_rctSelection.bottom() - m_rctSelection.top()));
emit sig_updateView( this, m_rctSelection );
}

void KSpreadTable::dissociateCell( const QPoint &_marker)
{
KSpreadCell *cell = nonDefaultCell(_marker.x() ,_marker.y()  );
int x=cell->extraXCells();
if(x==0)
        x=1;
int y=cell->extraYCells();
if(y==0)
        y=1;
cell->forceExtraCells( _marker.x() ,_marker.y(),0,0);
QRect selection(_marker.x() ,_marker.y(),x,y);

emit sig_updateView( this, selection );
}


QRect KSpreadTable::refreshArea( const QRect &_rect )
{
    QRect area(_rect);
    int nbColumn=area.right()-area.left()+1;
    int nbRow=area.bottom()-area.top()+1;
    bool selected = ( area.left() != 0 );

    // Complete rows selected ?
    if (  area.right() >= 0x7FFF && selected)
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int row = l & 0xFFFF;
	if ( m_rctSelection.top() <= row && m_rctSelection.bottom() >= row )
	{
        if(it.current()->extraYCells()!=0)
                nbRow=QMAX(nbRow,it.current()->extraYCells());
	}
      }
      if(nbRow==0)
        nbRow=1;
      area.setCoords(area.left(),area.top(),area.right(),nbRow+area.top());
      return  area;
    }
    // Complete columns selected ?
    else if (  area.bottom() >= 0x7FFF && selected )
    {
      QIntDictIterator<KSpreadCell> it( m_dctCells );
      for ( ; it.current(); ++it )
      {
	long l = it.currentKey();
	int col = l >> 16;
	if ( m_rctSelection.left() <= col && m_rctSelection.right() >= col )
	{
        if(it.current()->extraXCells())
                nbColumn=QMAX(nbColumn,it.current()->extraXCells());
	}
      }
      if(nbColumn==0)
        nbColumn=1;
      area.setCoords(area.left(),area.top(),nbColumn+area.left(),area.bottom());
      return area;
    }
    else
    {
	for ( int x = area.left(); x <= area.right(); x++ )
	    for ( int y = area.top(); y <= area.bottom(); y++ )
	    {
		KSpreadCell *cell = cellAt( x, y );

		if ( cell != m_pDefaultCell )
		{
                if(cell->extraXCells())
                        nbColumn=QMAX(nbColumn,cell->extraXCells());
                if(cell->extraYCells())
                        nbRow=QMAX(nbRow,cell->extraYCells());
                }
	    }

        area.setCoords(area.left(),area.top(),nbColumn+area.left(),nbRow+area.top());
        return area;

    }
}


void KSpreadTable::draw( QPaintDevice* _dev, long int _width, long int _height,
			 float _scale )
{
  QRect page_range;
  page_range.setLeft( 1 );
  page_range.setTop( 1 );

  QRect rect( 1, 1, _width, _height );

  int col = 1;
  int x = columnLayout( col )->width();
  bool bend = false;
  while ( !bend )
  {
    col++;
    int w = columnLayout( col )->width();
    if ( x + w > rect.width() )
    {
      bend = true;
      col--;
    }
    else
      x += w;
  }
  page_range.setRight( col );
	
  int row = 1;
  int y = rowLayout( row )->height();
  bend = false;
  while ( !bend )
  {
    row++;
    int h = rowLayout( row )->height();
    if ( y + h > rect.height() )
    {
      row--;
      bend = true;
    }
    else
      y += h;
  }
  page_range.setBottom( row );

  QPainter painter;
  painter.begin( _dev );

  if ( _scale != 1.0 )
    painter.scale( _scale, _scale );

  printPage( painter, &page_range, NoPen);//doc()->defaultGridPen() );

  painter.end();
}

void KSpreadTable::print( QPainter &painter, QPrinter *_printer )
{
    QPen gridPen;
    gridPen.setStyle( NoPen );

    unsigned int pages = 1;

    QRect cell_range;
    cell_range.setCoords( 1, 1, 1, 1 );

    // Find maximum right/bottom cell with content
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
    {
	if ( it.current()->column() > cell_range.right() )
	    cell_range.setRight( it.current()->column() );
	if ( it.current()->row() > cell_range.bottom() )
	    cell_range.setBottom( it.current()->row() );
    }

    QList<QRect> page_list;
    page_list.setAutoDelete( TRUE );

    QRect rect;
    rect.setCoords( 0, 0, (int)( MM_TO_POINT * m_pDoc->printableWidth() ),
		    (int)( MM_TO_POINT * m_pDoc->printableHeight() ) );

    // Up to this row everything is already printed
    int bottom = 0;
    // Start of the next page
    int top = 1;
    // Calculate all pages, but if we are embedded, print only the first one
    while ( bottom < cell_range.bottom() /* && page_list.count() == 0 */ )
    {
	kdDebug(36001) << "bottom=" << bottom << " bottom_range=" << cell_range.bottom() << endl;
	
	// Up to this column everything is already printed
	int right = 0;
	// Start of the next page
	int left = 1;
	while ( right < cell_range.right() )
        {
	    kdDebug(36001) << "right=" << right << " right_range=" << cell_range.right() << endl;
		
	    QRect *page_range = new QRect;
	    page_list.append( page_range );
	    page_range->setLeft( left );
	    page_range->setTop( top );

	    int col = left;
	    int x = columnLayout( col )->width();
	    while ( x < rect.width() )
            {
		col++;
		x += columnLayout( col )->width();
	    }
	    // We want to print at least one column
	    if ( col == left )
		col = left + 1;
	    page_range->setRight( col - 1 );

	    int row = top;
	    int y = rowLayout( row )->height();
	    while ( y < rect.height() )
            {
		row++;
		y += rowLayout( row )->height();
	    }
	    // We want to print at least one row
	    if ( row == top )
		row = top + 1;
	    page_range->setBottom( row - 1 );

	    right = page_range->right();
	    left = page_range->right() + 1;
	    bottom = page_range->bottom();
	}

	top = bottom + 1;
    }

    int pagenr = 1;

    // Print all pages in the list
    QRect *p;
    for ( p = page_list.first(); p != 0L; p = page_list.next() )
    {
	// print head line
	QFont font( "Times", 10 );
	painter.setFont( font );
	QFontMetrics fm = painter.fontMetrics();
	int w = fm.width( m_pDoc->headLeft( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() ),
			      (int)( MM_TO_POINT * 10.0 ), m_pDoc->headLeft( pagenr, m_strName ) );
	w = fm.width( m_pDoc->headMid( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() +
				     ( MM_TO_POINT * m_pDoc->printableWidth() - (float)w ) / 2.0 ),
			      (int)( MM_TO_POINT * 10.0 ), m_pDoc->headMid( pagenr, m_strName ) );
	w = fm.width( m_pDoc->headRight( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() +
				     MM_TO_POINT * m_pDoc->printableWidth() - (float)w ),
			      (int)( MM_TO_POINT * 10.0 ), m_pDoc->headRight( pagenr, m_strName ) );

	// print foot line
	w = fm.width( m_pDoc->footLeft( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() ),
			      (int)( MM_TO_POINT * ( m_pDoc->paperHeight() - 10.0 ) ),
			      m_pDoc->footLeft( pagenr, m_strName ) );
	w = fm.width( m_pDoc->footMid( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() +
				     ( MM_TO_POINT * m_pDoc->printableWidth() - (float)w ) / 2.0 ),
			      (int)( MM_TO_POINT * ( m_pDoc->paperHeight() - 10.0 ) ),
			      m_pDoc->footMid( pagenr, m_strName ) );
	w = fm.width( m_pDoc->footRight( pagenr, m_strName ) );
	if ( w > 0 )
	    painter.drawText( (int)( MM_TO_POINT * m_pDoc->leftBorder() +
				     MM_TO_POINT * m_pDoc->printableWidth() - (float)w ),
			      (int)( MM_TO_POINT * ( m_pDoc->paperHeight() - 10.0 ) ),
			      m_pDoc->footRight( pagenr, m_strName ) );
	
	painter.translate( MM_TO_POINT * m_pDoc->leftBorder(),
			   MM_TO_POINT * m_pDoc->topBorder() );
	// Print the page
	printPage( painter, p, gridPen );
	painter.translate( - MM_TO_POINT * m_pDoc->leftBorder(),
			   - MM_TO_POINT * m_pDoc->topBorder() );

	if ( pages < page_list.count() )
	    _printer->newPage();
	pagenr++;
    }
}

void KSpreadTable::printPage( QPainter &_painter, QRect *page_range, const QPen& _grid_pen )
{
  int ypos = 0;

  kdDebug(36001) << "Rect x=" << page_range->left() << " y=" << page_range->top() << ", w="
		 << page_range->width() << " h="  << page_range->height() << endl;
  for ( int y = page_range->top(); y <= page_range->bottom() + 1; y++ )
  {
    RowLayout *row_lay = rowLayout( y );
    int xpos = 0;

    for ( int x = page_range->left(); x <= page_range->right() + 1; x++ )
    {
      ColumnLayout *col_lay = columnLayout( x );

      // painter.window();	
      KSpreadCell *cell = cellAt( x, y );
      if ( y > page_range->bottom() && x > page_range->right() )
	{ /* Do nothing */ }
      else if ( y > page_range->bottom() )
	cell->print( _painter, xpos, ypos, x, y, col_lay, row_lay, FALSE, TRUE, _grid_pen );
      else if ( x > page_range->right() )
	cell->print( _painter, xpos, ypos, x, y, col_lay, row_lay, TRUE, FALSE, _grid_pen );
      else
	cell->print( _painter, xpos, ypos, x, y, col_lay, row_lay,
		     FALSE, FALSE, _grid_pen );

      xpos += col_lay->width();
    }

    ypos += row_lay->height();
  }

  // ########## Torben: Need to print children here.
  /**
  // Draw the children
  QListIterator<KSpreadChild> chl( m_lstChildren );
  for( ; chl.current(); ++chl )
  {
    kdDebug(36001) << "Printing child ....." << endl;
    // HACK, dont display images that reside outside the paper
    _painter.translate( chl.current()->geometry().left(),
			chl.current()->geometry().top() );
    QPicture* pic;
    pic = chl.current()->draw();
    kdDebug(36001) << "Fetched picture data" << endl;
    _painter.drawPicture( *pic );
    kdDebug(36001) << "Played" << endl;
    _painter.translate( - chl.current()->geometry().left(),
			- chl.current()->geometry().top() );
  }
  */
}

QDomDocument KSpreadTable::saveCellRect( const QRect &_rect )
{
    QDomDocument doc( "spreadsheet-snippet" );
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
    QDomElement spread = doc.createElement( "spreadsheet-snippet" );
    doc.appendChild( spread );

    QRect rightMost( _rect.x(), _rect.y(), _rect.width() + 1, _rect.height() );
    QRect bottomMost( _rect.x(), _rect.y(), _rect.width(), _rect.height() + 1 );

    // Save all cells.
    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
    {
	if ( !it.current()->isDefault() )
        {
	    QPoint p( it.current()->column(), it.current()->row() );
	    if ( _rect.contains( p ) )
		spread.appendChild( it.current()->save( doc, _rect.left() - 1, _rect.top() - 1 ) );
	    else if ( rightMost.contains( p ) )
		spread.appendChild( it.current()->saveRightMostBorder( doc, _rect.left() - 1, _rect.top() - 1 ) );
	    else if ( bottomMost.contains( p ) )
		spread.appendChild( it.current()->saveBottomMostBorder( doc, _rect.left() - 1, _rect.top() - 1 ) );
	}
    }

    return doc;
}

QDomElement KSpreadTable::save( QDomDocument& doc )
{
  QDomElement table = doc.createElement( "table" );
  table.setAttribute( "name", m_strName );
  table.setAttribute( "grid", (int)m_bShowGrid);
  table.setAttribute( "hide", (int)m_bTableHide);
  table.setAttribute( "formular", (int)m_bShowFormular);
  table.setAttribute( "borders", (int)m_bShowPageBorders);
  table.setAttribute( "lcmode", (int)m_bLcMode);
  table.setAttribute( "columnnumber", (int)m_bShowColumnNumber);
  // Save all cells.
  QIntDictIterator<KSpreadCell> it( m_dctCells );
  for ( ; it.current(); ++it )
  {
    if ( !it.current()->isDefault() )
    {
      QDomElement e = it.current()->save( doc );
      if ( e.isNull() )
	return QDomElement();
      table.appendChild( e );
    }
  }

  // Save all RowLayout objects.
  QIntDictIterator<RowLayout> rl( m_dctRows );
  for ( ; rl.current(); ++rl )
  {
    if ( !rl.current()->isDefault() )
    {
      QDomElement e = rl.current()->save( doc );
      if ( e.isNull() )
	return QDomElement();
      table.appendChild( e );
    }
  }

  // Save all ColumnLayout objects.
  QIntDictIterator<ColumnLayout> cl( m_dctColumns );
  for ( ; cl.current(); ++cl )
  {
    if ( !cl.current()->isDefault() )
    {
      QDomElement e = cl.current()->save( doc );
      if ( e.isNull() )
	return QDomElement();
      table.appendChild( e );
    }
  }

  QListIterator<KSpreadChild> chl( m_lstChildren );
  for( ; chl.current(); ++chl )
  {
    QDomElement e = chl.current()->save( doc );
    if ( e.isNull() )
      return QDomElement();
    table.appendChild( e );
  }

  return table;
}

bool KSpreadTable::isLoading()
{
  return m_pDoc->isLoading();
}

bool KSpreadTable::loadXML( const QDomElement& table )
{
  bool ok=false;
  m_strName = table.attribute( "name" );
  if ( m_strName.isEmpty() )
    return false;
  if(table.hasAttribute("grid"))
  {
    m_bShowGrid = (int)table.attribute("grid").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }
  if(table.hasAttribute("hide"))
  {
    m_bTableHide = (int)table.attribute("hide").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }
  if(table.hasAttribute("formular"))
  {
    m_bShowFormular = (int)table.attribute("formular").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }
  if(table.hasAttribute("borders"))
  {
    m_bShowPageBorders = (int)table.attribute("borders").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }
  if(table.hasAttribute("lcmode"))
  {
    m_bLcMode = (int)table.attribute("lcmode").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }
  if(table.hasAttribute("columnnumber"))
  {
    m_bShowColumnNumber = (int)table.attribute("columnnumber").toInt( &ok );
    // we just ignore 'ok' - if it didn't work, go on
  }

  QDomNode n = table.firstChild();
  while( !n.isNull() )
  {
    QDomElement e = n.toElement();
    if ( !e.isNull() && e.tagName() == "cell" )
    {
      KSpreadCell *cell = new KSpreadCell( this, 0, 0 );
      if ( cell->load( e, 0, 0 ) )
          insertCell( cell );
      else
          delete cell; // Allow error handling: just skip invalid cells
    }
    else if ( !e.isNull() && e.tagName() == "row" )
    {
      RowLayout *rl = new RowLayout( this, 0 );
      if ( rl->load( e ) )
          insertRowLayout( rl );
      else
          delete rl;
    }
    else if ( !e.isNull() && e.tagName() == "column" )
    {
      ColumnLayout *cl = new ColumnLayout( this, 0 );
      if ( cl->load( e ) )
          insertColumnLayout( cl );
      else
          delete cl;
    }
    else if ( !e.isNull() && e.tagName() == "object" )
    {
      KSpreadChild *ch = new KSpreadChild( m_pDoc, this );
      if ( ch->load( e ) )
          insertChild( ch );
      else
          delete ch;
    }
    else if ( !e.isNull() && e.tagName() == "chart" )
    {
	// ############ Torben
	/*
      ChartChild *ch = new ChartChild( m_pDoc, this );
      if ( ch->load( e ) )
	insertChild( ch );
      else
        delete ch;*/
    }
    n = n.nextSibling();
  }

  return true;
}

void KSpreadTable::update()
{
  kdDebug(36001) << "KSpreadTable::update()" << endl;
  QIntDictIterator<KSpreadCell> it( m_dctCells );
  for ( ; it.current(); ++it )
  {
      if ( it.current()->isFormular() )
	  it.current()->makeFormular();
      if ( it.current()->calcDirtyFlag() )
	  it.current()->update();
  }
}

bool KSpreadTable::loadChildren( KoStore* _store )
{
  QListIterator<KSpreadChild> it( m_lstChildren );
  for( ; it.current(); ++it )
    if ( !it.current()->loadDocument( _store ) )
      return false;

  return true;
}

void KSpreadTable::setShowPageBorders( bool b )
{
    if ( b == m_bShowPageBorders )
	return;

    m_bShowPageBorders = b;
    emit sig_updateView( this );
}

bool KSpreadTable::isOnNewPageX( int _column )
{
    int col = 1;
    float x = columnLayout( col )->mmWidth();
    while ( col <= _column )
    {
	// Should never happen
	if ( col == 0x10000 )
	    return FALSE;

	if ( x > m_pDoc->printableWidth() )
	{
	    if ( col == _column )
		return TRUE;
	    else
		x = columnLayout( col )->mmWidth();
	}
	
	col++;
	x += columnLayout( col )->mmWidth();
    }

    return FALSE;
}

bool KSpreadTable::isOnNewPageY( int _row )
{
    int row = 1;
    float y = rowLayout( row )->mmHeight();
    while ( row <= _row )
    {
	// Should never happen
	if ( row == 0x10000 )
	    return FALSE;

	if ( y > m_pDoc->printableHeight() )
	{
	    if ( row == _row )
		return TRUE;
	    else
		y = rowLayout( row )->mmHeight();
	}	
	row++;
	y += rowLayout( row )->mmHeight();
    }

    return FALSE;
}

void KSpreadTable::addCellBinding( CellBinding *_bind )
{
  m_lstCellBindings.append( _bind );

  m_pDoc->setModified( true );
}

void KSpreadTable::removeCellBinding( CellBinding *_bind )
{
  m_lstCellBindings.removeRef( _bind );

  m_pDoc->setModified( true );
}

KSpreadTable* KSpreadTable::findTable( const QString & _name )
{
  if ( !m_pMap )
    return 0L;

  return m_pMap->findTable( _name );
}

void KSpreadTable::insertCell( KSpreadCell *_cell )
{
  int key = _cell->row() + ( _cell->column() * 0x10000 );
  m_dctCells.replace( key, _cell );

  if ( m_bScrollbarUpdates )
  {
    if ( _cell->column() > m_iMaxColumn )
    {
      m_iMaxColumn = _cell->column();
      emit sig_maxColumn( _cell->column() );
    }
    if ( _cell->row() > m_iMaxRow )
    {
      m_iMaxRow = _cell->row();
      emit sig_maxRow( _cell->row() );
    }
  }
}

void KSpreadTable::insertColumnLayout( ColumnLayout *_l )
{
  m_dctColumns.replace( _l->column(), _l );
}

void KSpreadTable::insertRowLayout( RowLayout *_l )
{
  m_dctRows.replace( _l->row(), _l );
}

void KSpreadTable::updateCell( KSpreadCell *cell, int _column, int _row )
{
    if ( doc()->isLoading() )
	return;

    // Get the size
    int left = columnPos( _column );
    int top = rowPos( _row );
    int right = left + cell->extraWidth();
    int bottom = top + cell->extraHeight();

    // Need to calculate ?
    if ( cell->calcDirtyFlag() )
	cell->calc();

    // Need to make layout ?
    if ( cell->layoutDirtyFlag() )
	cell->makeLayout( painter(), _column, _row );

    // Perhaps the size changed now ?
    right = QMAX( right, left + cell->extraWidth() );
    bottom = QMAX( bottom, top + cell->extraHeight() );

    // Force redraw
    QPointArray arr( 4 );
    arr.setPoint( 0, left, top );
    arr.setPoint( 1, right, top );
    arr.setPoint( 2, right, bottom );
    arr.setPoint( 3, left, bottom );

    emit sig_polygonInvalidated( arr );

    cell->clearDisplayDirtyFlag();
}

void KSpreadTable::emit_updateRow( RowLayout *_layout, int _row )
{
    if ( doc()->isLoading() )
	return;

    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
      if ( it.current()->row() == _row )
	  it.current()->setLayoutDirtyFlag();

    emit sig_updateVBorder( this );
    emit sig_updateView( this );
    _layout->clearDisplayDirtyFlag();
}

void KSpreadTable::emit_updateColumn( ColumnLayout *_layout, int _column )
{
    if ( doc()->isLoading() )
	return;

    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for ( ; it.current(); ++it )
	if ( it.current()->column() == _column )
	    it.current()->setLayoutDirtyFlag();

    emit sig_updateHBorder( this );
    emit sig_updateView( this );
    _layout->clearDisplayDirtyFlag();
}

void KSpreadTable::insertChart( const QRect& _rect, KoDocumentEntry& _e, const QRect& _data )
{
    kdDebug(36001) << "Creating document" << endl;
    KoDocument* doc = _e.createDoc();
    kdDebug(36001) << "Created" << endl;
    if ( !doc )
	// Error message is already displayed, so just return
	return;

   kdDebug(36001) << "NOW FETCHING INTERFACE" << endl;

   if ( !doc->initDoc() )
     return;

   ChartChild* ch = new ChartChild( m_pDoc, this, doc, _rect );
   ch->setDataArea( _data );
   ch->update();

   m_pDoc->insertChild( ch );
   insertChild( ch );
}

void KSpreadTable::insertChild( const QRect& _rect, KoDocumentEntry& _e )
{
    KoDocument* doc = _e.createDoc( m_pDoc );
    doc->initDoc();

    KSpreadChild* ch = new KSpreadChild( m_pDoc, this, doc, _rect );
    // m_lstChildren.append( _child );
    m_pDoc->insertChild( ch );

    insertChild( ch );
}

void KSpreadTable::insertChild( KSpreadChild *_child )
{
  m_lstChildren.append( _child );

  emit sig_polygonInvalidated( _child->framePointArray() );
}

void KSpreadTable::changeChildGeometry( KSpreadChild *_child, const QRect& _rect )
{
  _child->setGeometry( _rect );

  emit sig_updateChildGeometry( _child );
}

QListIterator<KSpreadChild> KSpreadTable::childIterator()
{
  return QListIterator<KSpreadChild> ( m_lstChildren );
}

bool KSpreadTable::saveChildren( KoStore* _store, const char *_path )
{
  int i = 0;

  QListIterator<KSpreadChild> it( m_lstChildren );
  for( ; it.current(); ++it )
  {
    QString path = QString( "%1/%2" ).arg( _path ).arg( i++ );
    if ( !it.current()->document()->saveToStore( _store, "", path ) )
      return false;
  }
  return true;
}

KSpreadTable::~KSpreadTable()
{
    s_mapTables->remove( m_id );

    QIntDictIterator<KSpreadCell> it( m_dctCells );
    for (; it.current(); ++it )
	it.current()->tableDies();

    m_dctCells.clear(); // cells destructor needs table to still exist

    m_pPainter->end();
    delete m_pPainter;
    delete m_pWidget;
}

void KSpreadTable::enableScrollBarUpdates( bool _enable )
{
  m_bScrollbarUpdates = _enable;
}

DCOPObject* KSpreadTable::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KSpreadTableIface( this );

    return m_dcop;
}


bool KSpreadTable::setTableName( const QString& name, bool init )
{
    if ( map()->findTable( name ) )
	return FALSE;

    if ( m_strName == name )
	return TRUE;

    QString old_name = m_strName;
    m_strName = name;

    if ( init )
	return TRUE;

    QListIterator<KSpreadTable> it( map()->tableList() );
    for( ; it.current(); ++it )
	it.current()->changeCellTabName( old_name, name );

    if ( !m_pDoc->undoBuffer()->isLocked() )
    {
	KSpreadUndoAction* undo = new KSpreadUndoSetTableName( doc(), this, old_name );
	m_pDoc->undoBuffer()->appendUndo( undo );
    }

    m_pDoc->changeAreaTableName(old_name,name);
    emit sig_nameChanged( this, old_name );

    return TRUE;
}

/**********************************************************
 *
 * KSpreadChild
 *
 **********************************************************/

KSpreadChild::KSpreadChild( KSpreadDoc *parent, KSpreadTable *_table, KoDocument* doc, const QRect& geometry )
  : KoDocumentChild( parent, doc, geometry )
{
  m_pTable = _table;
}

KSpreadChild::KSpreadChild( KSpreadDoc *parent, KSpreadTable *_table ) : KoDocumentChild( parent )
{
  m_pTable = _table;
}


KSpreadChild::~KSpreadChild()
{
}

/**********************************************************
 *
 * ChartChild
 *
 **********************************************************/

ChartChild::ChartChild( KSpreadDoc *_spread, KSpreadTable *_table, KoDocument* doc, const QRect& _rect )
  : KSpreadChild( _spread, _table, doc, _rect )
{
  m_pBinding = 0;
  m_table = _table;
}

/* ChartChild::ChartChild( KSpreadDoc *_spread, KSpreadTable *_table ) :
  KSpreadChild( _spread, _table )
{
  m_pBinding = 0;
  } */

ChartChild::~ChartChild()
{
  if ( m_pBinding )
    delete m_pBinding;
}

void ChartChild::setDataArea( const QRect& _data )
{
  if ( m_pBinding == 0L )
    m_pBinding = new ChartBinding( m_pTable, _data, this );
  else
    m_pBinding->setDataArea( _data );
}

void ChartChild::update()
{
    if ( m_pBinding )
	m_pBinding->cellChanged( 0 );
}

bool ChartChild::save( ostream& out )
{
    QString u = document()->url().url();
    QString mime = document()->mimeType();

    out << indent << "<CHART url=\"" << u.utf8().data() << "\" mime=\"" << mime.utf8().data() << "\">"
	<< geometry();
    if ( m_pBinding )
	out << "<BINDING>" << m_pBinding->dataArea() << "</BINDING>";
    out << "</CHART>" << endl;

    return true;
}

// ############### Is this KOML stuff really needed ?
bool ChartChild::loadTag( KOMLParser& parser, const string& tag, vector<KOMLAttrib>& /* lst */ )
{
    if ( tag == "BINDING" )
    {
	string tag2;
	vector<KOMLAttrib> lst2;
	string name2;
	// RECT
	while( parser.open( 0L, tag2 ) )
	{
	    KOMLParser::parseTag( tag2.c_str(), name2, lst2 );

	    if ( name2 == "RECT" )
		m_pBinding = new ChartBinding( m_table, tagToRect( lst2 ), this );
	    else
	    {
		kdDebug(36001) << "Unknown tag '" << tag2.c_str() << "' in BINDING" << endl;
		return FALSE;
	    }
	}
	if ( !parser.close( (string &) tag ) )
	{
	    kdDebug(36001) << "ERR: Closing BINDING" << endl;
	    return false;
	}
	
	return TRUE;
    }

    return FALSE;
}

bool ChartChild::loadDocument( KoStore* _store )
{
    // Did we see the BINDING tag ?
    if ( !m_pBinding )
	return FALSE;

    bool res = KSpreadChild::loadDocument( _store );
    if ( !res )
	return res;

  // #### Torben: Check wether the document really supports
  //      the chart interface
  /* CORBA::Object_var obj = m_rDoc->getInterface( "IDL:Chart/SimpleChart:1.0" );
  Chart::SimpleChart_var chart = Chart::SimpleChart::_narrow( obj );
  if ( CORBA::is_nil( chart ) )
  {
    KMessageBox::error( 0L, i18n("Chart does not support the required interface"));
    return false;
    } */

    update();

    return true;
}

KChartPart* ChartChild::chart()
{
    return (KChartPart*)document();
}

#include "kspread_table.moc"
