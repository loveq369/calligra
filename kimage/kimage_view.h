#ifndef __kimage_gui_h__
#define __kimage_gui_h__

class KImageView;
class KImageDoc;
class KImageShell;

#include <koFrame.h>
#include <koView.h>
#include <opMenu.h>
#include <opToolBar.h>
#include <openparts_ui.h>

#include <qpixmap.h>
#include <qwidget.h>

#include "kimage.h"

/**
 */
class KImageView : public QWidget,
		   virtual public KoViewIf,
		   virtual public KImage::View_skel
{
    Q_OBJECT
public:
    KImageView( QWidget *_parent, const char *_name, KImageDoc *_doc );
    ~KImageView();

    KImageDoc* doc() { return m_pDoc; }
  
    /**
     * ToolBar
     */
    void fitToView();
    /**
     * ToolBar
     */
    void fitWithProportions();
    /**
     * ToolBar
     */
    void originalSize();
    /**
     * ToolBar
     */
    void editImage();
  
    /**
     * MenuBar
     */
    void pageLayout();
    /**
     * MenuBar
     */
    void importImage();
    /**
     * MenuBar
     */
    void exportImage();
  
    virtual void cleanUp();

    CORBA::Boolean printDlg();
  
public slots:
    // Document signals
    void slotUpdateView();

protected:
  // C++
  virtual void init();
  // IDL
  virtual bool event( const char* _event, const CORBA::Any& _value );
  // C++
  virtual bool mappingCreateMenubar( OpenPartsUI::MenuBar_ptr _menubar );
  virtual bool mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr _factory );
  
  virtual void newView();
  virtual void helpUsing();

  virtual void resizeEvent( QResizeEvent *_ev );
  virtual void paintEvent( QPaintEvent *_ev );
  
  OpenPartsUI::ToolBar_var m_vToolBarEdit;
  CORBA::Long m_idButtonEdit_Lines;
  CORBA::Long m_idButtonEdit_Areas;
  CORBA::Long m_idButtonEdit_Bars;
  CORBA::Long m_idButtonEdit_Cakes;

  OpenPartsUI::Menu_var m_vMenuEdit;
  CORBA::Long m_idMenuEdit_FitToView;
  CORBA::Long m_idMenuEdit_FitWithProps;
  CORBA::Long m_idMenuEdit_Original;
  CORBA::Long m_idMenuEdit_Edit;
  CORBA::Long m_idMenuEdit_Import;
  CORBA::Long m_idMenuEdit_Export;
  CORBA::Long m_idMenuEdit_Page;
  OpenPartsUI::Menu_var m_vMenuHelp;
  CORBA::Long m_idMenuHelp_About;
  CORBA::Long m_idMenuHelp_Using;
    
  KImageDoc *m_pDoc;  

  QPixmap m_pixmap;
  
  int m_iXOffset;
  int m_iYOffset;
};

#endif
