/*
 * KPlato Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 * Copyright (C) 2010, 2011, 2012 by Dag Andersen <danders@get2net.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "reportview.h"
#include "reportview_p.h"

#include "report.h"
#include "reportdata.h"
#include "reportsourceeditor.h"
#include "reportscripts.h"
#include "ui_reportsectionswidget.h"
#include "ui_reporttoolswidget.h"

#include <KReportPage>
#include <KReportPreRenderer>
#include <KReportRenderObjects>
#include <KReportDesigner>
#include <KReportDesignerSection>
#include <KReportDesignerSectionDetail>
#include <KReportDesignerSectionDetailGroup>
#include <KPropertyEditorView>

#include "kptglobal.h"
#include "kptaccountsmodel.h"
#include "kptflatproxymodel.h"
#include "kptnodeitemmodel.h"
#include "kpttaskstatusmodel.h"
#include "kptresourcemodel.h"
#include "kptresourceappointmentsmodel.h"
#include "kptschedule.h"
#include "kptnodechartmodel.h"
#include "kptdebug.h"

#include "KoPageLayout.h"
#include "KoDocument.h"
#include "KoIcon.h"
#include <KoXmlReader.h>

#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardguiitem.h>
#include <kguiitem.h>
#include <kmessagebox.h>
#include <ktoolbar.h>

#include <QCloseEvent>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLayout>
#include <QDockWidget>
#include <QModelIndex>
#include <QModelIndexList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QActionGroup>
#include <QStackedWidget>
#include <QAction>
#include <qpushbutton.h>
#include <QMimeDatabase>
#include <QFileDialog>

namespace KPlato
{

//----------------
ReportPrintingDialog::ReportPrintingDialog( ViewBase *view, ORODocument *reportDocument )
    : KoPrintingDialog( view ),
    m_reportDocument( reportDocument )
{
    printer().setFromTo( documentFirstPage(), documentLastPage() );
    m_context.printer = &printer();
    m_context.painter = 0;
    KReportRendererFactory factory;
    m_renderer = factory.createInstance( "print" );

    //FIXME: This should be done by KReportPrintRender but setupPrinter() is private
    QPrinter *pPrinter = &printer();
    pPrinter->setCreator("Plan");
    pPrinter->setDocName(reportDocument->title());
    pPrinter->setFullPage(true);
    pPrinter->setOrientation((reportDocument->pageOptions().isPortrait() ? QPrinter::Portrait : QPrinter::Landscape));
    pPrinter->setPageOrder(QPrinter::FirstPageFirst);

    if (reportDocument->pageOptions().getPageSize().isEmpty())
        pPrinter->setPageSize(QPrinter::Custom);
    else
        pPrinter->setPageSize(KoPageFormat::printerPageSize(KoPageFormat::formatFromString(reportDocument->pageOptions().getPageSize())));

    //FIXME: There is something wrong with KReport margins
    qreal left = reportDocument->pageOptions().getMarginLeft();
    qreal top = reportDocument->pageOptions().getMarginTop();
    qreal right = reportDocument->pageOptions().getMarginRight();
    qreal bottom = reportDocument->pageOptions().getMarginBottom();

    pPrinter->setPageMargins( left, top, right, bottom, QPrinter::Point );
}

ReportPrintingDialog::~ReportPrintingDialog()
{
    delete m_renderer;
}

void ReportPrintingDialog::startPrinting( RemovePolicy removePolicy )
{
    debugPlan;
    QPainter p( &printer() );
    printPage( 1,  p );
    if ( removePolicy == DeleteWhenDone ) {
        deleteLater();
    }
}

int ReportPrintingDialog::documentLastPage() const
{
    return m_reportDocument->pages();
}


void ReportPrintingDialog::printPage( int page, QPainter &painter )
{
    m_context.painter = &painter;
    m_renderer->render( m_context, m_reportDocument, page );
}

QAbstractPrintDialog::PrintDialogOptions ReportPrintingDialog::printDialogOptions() const
{
    return QAbstractPrintDialog::PrintToFile |
           QAbstractPrintDialog::PrintPageRange |
           QAbstractPrintDialog::PrintCollateCopies |
           QAbstractPrintDialog::DontUseSheet;
}

//---------------------
ReportView::ReportView(KoPart *part, KoDocument *doc, QWidget *parent )
    : ViewBase(part, doc, parent )
{
//    debugPlan<<"--------------- ReportView ------------------";
    setObjectName("ReportView");

    QLayout *l = new QHBoxLayout( this );
    l->setMargin(0);
    m_stack = new QStackedWidget( this );
    l->addWidget( m_stack );

    ReportWidget *v = new ReportWidget(part, doc, m_stack);
    m_stack->addWidget( v );
    connect(v, SIGNAL(editReportDesign()),SLOT(slotEditReport()));
    connect(v, SIGNAL(guiActivated(ViewBase*,bool)), SIGNAL(guiActivated(ViewBase*,bool)));

    ReportDesigner *d = new ReportDesigner(part, doc, m_stack);
    m_stack->addWidget( d );
    connect(d, SIGNAL(viewReport()), SLOT(slotViewReport()));
    connect(d, SIGNAL(guiActivated(ViewBase*,bool)), SIGNAL(guiActivated(ViewBase*,bool)));
    connect(d, SIGNAL(optionsModified()), SIGNAL(optionsModified()));

    m_stack->setCurrentIndex( 0 );
}

void ReportView::slotEditReport()
{
    reportWidget()->setGuiActive( false );
    m_stack->setCurrentIndex( 1 );
    reportDesigner()->setGuiActive( true );
}

void ReportView::slotViewReport()
{
    reportDesigner()->setGuiActive( false );
    if ( reportWidget()->documentIsNull() || reportDesigner()->isModified() ) {
        reportWidget()->loadXML( reportDesigner()->document() );
    }
    if ( reportDesigner()->isModified() ) {
        emit optionsModified();
        reportDesigner()->setModified( false );
    }
    m_stack->setCurrentIndex( 0 );
    reportWidget()->setGuiActive( true );
}

void ReportView::setProject( Project *project )
{
    reportWidget()->setProject( project );
    reportDesigner()->setProject( project );
}

void ReportView::setScheduleManager( ScheduleManager *sm )
{
    reportWidget()->setScheduleManager( sm );
    reportDesigner()->setScheduleManager( sm );
}

KoPrintJob *ReportView::createPrintJob()
{
    return static_cast<ViewBase*>( m_stack->currentWidget() )->createPrintJob();
}

void ReportView::setGuiActive( bool active )
{
    return static_cast<ViewBase*>( m_stack->currentWidget() )->setGuiActive( active );
}

bool ReportView::loadXML( const QDomDocument &doc )
{
    reportDesigner()->setData( doc );
    return reportWidget()->loadXML( doc );
}

bool ReportView::loadContext( const KoXmlElement &context )
{
    bool res = true;
    // designer first, widget uses it's data
    res = reportDesigner()->loadContext( context );
    res &= reportWidget()->loadContext( context );

    reportWidget()->loadXML( reportDesigner()->document() );

    return res;
}

void ReportView::saveContext( QDomElement &context ) const
{
    QDomElement e = context.ownerDocument().createElement( "view" );
    context.appendChild( e );
    e.setAttribute( "current-view", QString::number(m_stack->currentIndex()) );

    reportDesigner()->saveContext( context );
    reportWidget()->saveContext( context );
}

ReportWidget *ReportView::reportWidget() const
{
    return static_cast<ReportWidget*>( m_stack->widget( 0 ) );
}

ReportDesigner *ReportView::reportDesigner() const
{
    return static_cast<ReportDesigner*>( m_stack->widget( 1 ) );
}

QDomDocument ReportView::document() const
{
    return reportDesigner()->document();
}

QList< ReportData* > ReportView::reportDataModels() const
{
    return reportWidget()->reportDataModels();
}


//---------------------
ReportWidget::ReportWidget(KoPart *part, KoDocument *doc, QWidget *parent )
    : ViewBase(part, doc, parent ),
    m_reportdatamodels( Report::createBaseReportDataModels() )
{
//    debugPlan<<"--------------- ReportWidget ------------------";

    m_preRenderer = 0;
    setObjectName("ReportWidget");

    m_reportView = new QGraphicsView(this);
    m_reportScene = new QGraphicsScene(this);
    m_reportScene->setSceneRect(0,0,1000,2000);
    m_reportView->setScene(m_reportScene);
    m_reportScene->setBackgroundBrush(palette().brush(QPalette::Dark));

    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin(0);
    l->addWidget( m_reportView );
    m_pageSelector = new ReportNavigator( this );
    l->addWidget( m_pageSelector );

    setupGui();

    connect(m_pageSelector->ui_next, SIGNAL(clicked()), this, SLOT(nextPage()));
    connect(m_pageSelector->ui_prev, SIGNAL(clicked()), this, SLOT(prevPage()));
    connect(m_pageSelector->ui_first, SIGNAL(clicked()), this, SLOT(firstPage()));
    connect(m_pageSelector->ui_last, SIGNAL(clicked()), this, SLOT(lastPage()));
    connect(m_pageSelector->ui_selector, SIGNAL(valueChanged(int)), SLOT(renderPage(int)));

    slotRefreshView();
}

//-----------------

void ReportWidget::renderPage( int page )
{
    m_reportPage->renderPage( page );
}

void ReportWidget::nextPage()
{
    m_pageSelector->ui_selector->setValue( m_pageSelector->ui_selector->value() + 1 );
}

void ReportWidget::prevPage()
{
    m_pageSelector->ui_selector->setValue( m_pageSelector->ui_selector->value() - 1 );
}

void ReportWidget::firstPage()
{
    m_pageSelector->ui_selector->setValue( 1 );
}

void ReportWidget::lastPage()
{
    m_pageSelector->ui_selector->setValue( m_pageSelector->ui_max->value() );
}

KoPrintJob *ReportWidget::createPrintJob()
{
    return new ReportPrintingDialog( this, m_reportDocument );
}

KoPageLayout ReportWidget::pageLayout() const
{
    KoPageLayout p = ViewBase::pageLayout();
    KReportPageOptions opt = m_reportDocument->pageOptions();
    p.orientation = opt.isPortrait() ? KoPageFormat::Portrait : KoPageFormat::Landscape;

    if (opt.getPageSize().isEmpty()) {
        p.format = KoPageFormat::CustomSize;
        p.width = opt.getCustomWidth();
        p.height = opt.getCustomHeight();
    } else {
        p.format = KoPageFormat::formatFromString(opt.getPageSize());
    }
    p.topMargin = opt.getMarginTop();
    p.bottomMargin = opt.getMarginBottom();
    p.leftMargin = opt.getMarginLeft();
    p.rightMargin = opt.getMarginRight();

    p.pageEdge = 0.0;
    p.bindingSide = 0.0;
    return p;
}

QUrl ReportWidget::getExportFileName(const QString &mimetype)
{
    const QString filterString = QMimeDatabase().mimeTypeForName(mimetype).filterString();

    const QString result = QFileDialog::getSaveFileName(this, i18nc("@title:window", "Export Report"), QString(), filterString);
    return result.isEmpty() ? QUrl() : QUrl::fromLocalFile(result);
}

void ReportWidget::exportAsTextDocument()
{
    KReportRendererContext context;
    context.destinationUrl = getExportFileName(QStringLiteral("application/vnd.oasis.opendocument.text"));
    if (!context.destinationUrl.isValid()) {
        return;
    }

    debugPlan<<"Export to odt:"<<context.destinationUrl;
    KReportRendererBase *renderer = m_factory.createInstance("odtframes");
    if ( renderer == 0 ) {
        errorPlan<<"Cannot create odt (frames) renderer";
        return;
    }
    if (!renderer->render(context, m_reportDocument)) {
        KMessageBox::error(this, i18nc( "@info", "Failed to export to <filename>%1</filename>", context.destinationUrl.toDisplayString()) , i18n("Export to text document failed"));
    }
}

void ReportWidget::exportAsSpreadsheet()
{
    KReportRendererContext context;
    context.destinationUrl = getExportFileName(QStringLiteral("application/vnd.oasis.opendocument.spreadsheet"));
    if (!context.destinationUrl.isValid()) {
        return;
    }

    debugPlan<<"Export to ods:"<<context.destinationUrl;
    KReportRendererBase *renderer;
    renderer = m_factory.createInstance("ods");
    if ( renderer == 0 ) {
        errorPlan<<"Cannot create ods renderer";
        return;
    }
    if (!renderer->render(context, m_reportDocument)) {
        KMessageBox::error(this, i18nc( "@info", "Failed to export to <filename>%1</filename>", context.destinationUrl.toDisplayString()) , i18n("Export to spreadsheet failed"));
    }
}

void ReportWidget::exportAsWebPage()
{
    KReportRendererContext context;
    context.destinationUrl = getExportFileName(QStringLiteral("text/html"));
    if (!context.destinationUrl.isValid()) {
        return;
    }

    debugPlan<<"Export to html:"<<context.destinationUrl;
    KReportRendererBase *renderer;
    //QT5TODO: perhaps also support "htmltable", with similar question switch like kexi
    //Though plugins should rather not be hardcoded at all in the future.
    renderer = m_factory.createInstance("htmlcss");
    if ( renderer == 0 ) {
        errorPlan<<"Cannot create html renderer";
        return;
    }
    if (!renderer->render(context, m_reportDocument)) {
        KMessageBox::error(this, i18nc( "@info", "Failed to export to <filename>%1</filename>", context.destinationUrl.toDisplayString()) , i18n("Export to HTML failed"));
    }
}

void ReportWidget::setupGui()
{
    /*KActionCollection *coll = actionCollection();*/
    QAction *a = 0;
    QString name = "reportview_list";

    a = new QAction(koIcon("go-next-view"), i18n("Edit Report"), this);
    a->setToolTip( i18nc( "@info:tooltip", "Edit the report definition" ) );
    a->setWhatsThis( i18nc( "@info:whatsthis", "Opens the report design in the report design dialog." ) );
    connect(a, SIGNAL(triggered()), this, SIGNAL(editReportDesign()));
    addAction( name, a );

    KActionMenu *exportMenu = new KActionMenu(koIcon("document-export"), i18nc("@title:menu","E&xport As"), this);
    exportMenu->setToolTip( i18nc( "@info:tooltip", "Export to file" ) );
    exportMenu->setDelayed(false);

    a = new QAction(koIcon("application-vnd.oasis.opendocument.text"), i18n("Text Document..."), this);
    a->setToolTip(i18n("Export the report as a text document (in OpenDocument Text format)"));
    connect(a, SIGNAL(triggered()), this, SLOT(exportAsTextDocument()));
    exportMenu->addAction(a);

    a = new QAction(koIcon("application-vnd.oasis.opendocument.spreadsheet"), i18n("Spreadsheet..."), this);
    a->setToolTip(i18n("Export the report as a spreadsheet (in OpenDocument Spreadsheet format)"));
    connect(a, SIGNAL(triggered()), this, SLOT(exportAsSpreadsheet()));
    exportMenu->addAction(a);

    a = new QAction(koIcon("text-html"), i18n("Web Page..."), this);
    a->setObjectName("export_as_web_page");
    a->setToolTip(i18n("Export the report as a web page (in HTML format)"));
    connect(a, SIGNAL(triggered()), this, SLOT(exportAsWebPage()));
    exportMenu->addAction(a);

    addAction( name, exportMenu );
}

void ReportWidget::setGuiActive( bool active ) // virtual slot
{
    if ( active ) {
        slotRefreshView();
    }
    ViewBase::setGuiActive( active );
}

void ReportWidget::slotRefreshView()
{
    if ( ! isVisible() ) {
        debugPlan<<"Not visible";
        return;
    }
    delete m_preRenderer;
    QDomElement e = m_design.documentElement();
    m_preRenderer = new KReportPreRenderer( e.firstChildElement( "report:content" ) );
    if ( ! m_preRenderer->isValid()) {
        debugPlan<<"Invalid design document";
        return;
    }
    ReportData *rd = createReportData( e );
    m_preRenderer->setSourceData( rd );
    //QT5TODO: see how to make this depend on scripting availability
//     m_preRenderer->registerScriptObject(new ProjectAccess( rd ), "project");

    if (! m_preRenderer->generateDocument()) {
        debugPlan << "Could not generate report document";
        return;
    }

    m_reportDocument = m_preRenderer->document();
    m_pageSelector->setMaximum( m_reportDocument ? m_reportDocument->pages() : 1 );
    m_pageSelector->setCurrentPage( 1 );

    m_reportPage = new KReportPage(this, m_reportDocument);
    m_reportPage->setObjectName("ReportPage");

    m_reportScene->setSceneRect(0,0,m_reportPage->rect().width() + 40, m_reportPage->rect().height() + 40);
    m_reportScene->addItem(m_reportPage);
    m_reportPage->setPos(20,20);
    m_reportView->centerOn(0,0);

    return;
}

void ReportWidget::setReportDataModels( const QList<ReportData*> &models )
{
    m_reportdatamodels = models;
}

ReportData *ReportWidget::createReportData( const QDomElement &element )
{
    // get the data source
    QDomElement e = element.firstChildElement( "data-source" );
    QString modelname = e.attribute( "select-from" );

    return createReportData( modelname );
}

ReportData *ReportWidget::createReportData( const QString &type )
{
    ReportData *r = Report::findReportData( m_reportdatamodels, type );
    Q_ASSERT( r );
    if ( r ) {
        r = r->clone();
        r->setParent( this );
        r->setProject( project() );
        r->setScheduleManager( m_schedulemanager );
    }
    return r;
}

bool ReportWidget::loadXML( const QDomDocument &doc )
{
    m_design = doc;
    slotRefreshView();
    return true;
}

bool ReportWidget::loadContext( const KoXmlElement &/*context*/ )
{
    return true;
}

void ReportWidget::saveContext( QDomElement &/*context*/ ) const
{
}

bool ReportWidget::documentIsNull() const
{
    return m_design.isNull();
}

//------------------
ReportNavigator::ReportNavigator( QWidget *parent )
    : QWidget( parent )
{
    setupUi( this );
    ui_first->setIcon(koIcon("go-first-view"));
    ui_last->setIcon(koIcon("go-last-view"));
    ui_prev->setIcon(koIcon("go-previous-view"));
    ui_next->setIcon(koIcon("go-next-view"));

    connect( ui_max, SIGNAL(valueChanged(int)), SLOT(slotMaxChanged(int)));

    connect( ui_selector, SIGNAL(valueChanged(int)), SLOT(setButtonsEnabled()) );

    ui_max->setValue( 1 );
}

void ReportNavigator::setMaximum( int value )
{
    ui_max->setMaximum( value );
    ui_max->setValue( value );
}

void ReportNavigator::setCurrentPage( int page )
{
    ui_selector->setValue( page );
}


void ReportNavigator::slotMaxChanged( int value )
{
    ui_selector->setMaximum( value );
    setButtonsEnabled();
}

void ReportNavigator::setButtonsEnabled()
{
    bool backw = ui_selector->value() > ui_selector->minimum();
    ui_first->setEnabled( backw );
    ui_prev->setEnabled( backw );

    bool forw = ui_selector->value() < ui_selector->maximum();
    ui_last->setEnabled( forw );
    ui_next->setEnabled( forw );
}

//----------------------------
ReportDesignDialog::ReportDesignDialog( QWidget *parent )
    : KoDialog( parent ),
    m_view( 0 )
{
    setCaption( i18nc( "@title:window", "Report Designer" ) );
    m_panel = new ReportDesignPanel( this );

    setMainWidget( m_panel );
}

ReportDesignDialog::ReportDesignDialog( const QDomElement &element, const QList<ReportData*> &models, QWidget *parent )
    : KoDialog( parent ),
    m_view( 0 )
{
    setCaption( i18nc( "@title:window", "Report Designer" ) );
    setButtons( KoDialog::Close | KoDialog::User1 | KoDialog::User2 );
    setButtonText( KoDialog::User1, i18n( "Save To View" ) );
    setButtonIcon(KoDialog::User1, koIcon("window-new"));
    setButtonText( KoDialog::User2, i18n( "Save To File" ) );
    setButtonIcon(KoDialog::User2, koIcon("document-save-as"));

    m_panel = new ReportDesignPanel( element, models, this );

    setMainWidget( m_panel );

    connect( this, SIGNAL(user1Clicked()), SLOT(slotSaveToView()) );
    connect( this, SIGNAL(user2Clicked()), SLOT(slotSaveToFile()) );
}

void ReportDesignDialog::closeEvent ( QCloseEvent * e )
{
    if ( m_panel->m_modified ) {
        //NOTE: When the close (x) button in the window frame is clicked, QWidget automatically hides us if we don't handle it
        QPushButton *b = button( KoDialog::Close );
        if ( b ) {
            b->animateClick();
            e->ignore();
            return;
        }
    }
    KoDialog::closeEvent ( e );
}

void ReportDesignDialog::slotButtonClicked( int button )
{
    if ( button == KoDialog::Close ) {
        if ( m_panel->m_modified ) {
            int res = KMessageBox::warningContinueCancel( this,
                    i18nc( "@info", "The report definition has been modified.<br/>"
                    "<emphasis>If you continue, the modifications will be lost.</emphasis>" ) );

            if ( res == KMessageBox::Cancel ) {
                return;
            }
        }
        hide();
        reject();
        return;
    }
    KoDialog::slotButtonClicked( button );
}

void ReportDesignDialog::slotSaveToFile()
{
    const QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty()) {
        return;
    }

    QFile file( fileName );
    if ( ! file.open( QIODevice::WriteOnly ) ) {
        KMessageBox::sorry( this, i18nc( "@info", "Cannot open file:<br/><filename>%1</filename>", file.fileName() ) );
        return;
    }
    QTextStream out( &file );
    out << document().toString();
    m_panel->m_modified = false;
    file.close();
}

void ReportDesignDialog::slotSaveToView()
{
    if ( m_view == 0 ) {
        emit createReportView( this );
        return;
    }
    if ( m_panel->m_modified ) {
        saveToView();
    }
}

void ReportDesignDialog::slotViewCreated( ViewBase *view )
{
    ReportView *v = dynamic_cast<ReportView*>( view );
    if ( v ) {
        m_view = v;
        saveToView(); // always save
    }
}

void ReportDesignDialog::saveToView()
{
    if ( m_view == 0 ) {
        return;
    }
    KUndo2Command *cmd = new ModifyReportDefinitionCmd( m_view, document(), kundo2_i18n( "Modify report definition" ) );
    emit modifyReportDefinition( cmd );
    m_panel->m_modified = false;
}

QDomDocument ReportDesignDialog::document() const
{
    return m_panel->document();
}

//----
ReportDesignPanel::ReportDesignPanel( QWidget *parent )
    : QWidget( parent ),
    m_modified( false ),
    m_reportdatamodels( Report::createBaseReportDataModels( this ) )
{
    QVBoxLayout *l = new QVBoxLayout( this );

    KToolBar *tb = new KToolBar( this );
    l->addWidget( tb );


    QSplitter *sp1 = new QSplitter( this );
    l->addWidget( sp1 );

    QFrame *frame = new QFrame( sp1 );
    frame->setFrameShadow( QFrame::Sunken );
    frame->setFrameShape( QFrame::StyledPanel );

    l = new QVBoxLayout( frame );

    m_sourceeditor = new ReportSourceEditor( frame );
    l->addWidget( m_sourceeditor );
    QStandardItemModel *model = createSourceModel( m_sourceeditor );
    m_sourceeditor->setModel( model );

    m_propertyeditor = new KPropertyEditorView( frame );
    l->addWidget( m_propertyeditor );

    QScrollArea *sa = new QScrollArea( sp1 );
    m_designer = new KReportDesigner( sa );
    sa->setWidget( m_designer );

    m_designer->setReportData( createReportData( m_sourceeditor->selectFromTag() ) );
    slotPropertySetChanged();

    connect( m_sourceeditor, SIGNAL(selectFromChanged(QString)), SLOT(setReportData(QString)) );

    connect( this, SIGNAL(insertItem(QString)), m_designer, SLOT(slotItem(QString)) );

    connect( m_designer, SIGNAL(propertySetChanged()), SLOT(slotPropertySetChanged()) );
    connect( m_designer, SIGNAL(dirty()), SLOT(setModified()) );
    connect( m_designer, SIGNAL(itemInserted(QString)), this, SLOT(slotItemInserted(QString)));

    populateToolbar( tb );
}

ReportDesignPanel::ReportDesignPanel( const QDomElement &element, const QList<ReportData*> &models, QWidget *parent )
    : QWidget( parent ),
    m_modified( false ),
    m_reportdatamodels( models )
{
    QVBoxLayout *l = new QVBoxLayout( this );

    KToolBar *tb = new KToolBar( this );
    l->addWidget( tb );


    QSplitter *sp1 = new QSplitter( this );
    l->addWidget( sp1 );

    QFrame *frame = new QFrame( sp1 );
    frame->setFrameShadow( QFrame::Sunken );
    frame->setFrameShape( QFrame::StyledPanel );

    l = new QVBoxLayout( frame );

    m_sourceeditor = new ReportSourceEditor( frame );
    l->addWidget( m_sourceeditor );
    QStandardItemModel *model = createSourceModel( m_sourceeditor );
    m_sourceeditor->setModel( model );
    m_sourceeditor->setSourceData( element.firstChildElement( "data-source" ) );

    m_propertyeditor = new KPropertyEditorView( frame );
    l->addWidget( m_propertyeditor );

    QScrollArea *sa = new QScrollArea( sp1 );
    QDomElement e = element.firstChildElement( "report:content" );
    if ( e.isNull() ) {
        m_designer = new KReportDesigner( sa );
    } else {
        m_designer = new KReportDesigner( sa, e );
    }
    sa->setWidget( m_designer );

    m_designer->setReportData( createReportData( m_sourceeditor->selectFromTag() ) );
    slotPropertySetChanged();

    connect( m_sourceeditor, SIGNAL(selectFromChanged(QString)), SLOT(setReportData(QString)) );

    connect( this, SIGNAL(insertItem(QString)), m_designer, SLOT(slotItem(QString)) );

    connect( m_designer, SIGNAL(propertySetChanged()), SLOT(slotPropertySetChanged()) );
    connect( m_designer, SIGNAL(dirty()), SLOT(setModified()) );
    connect( m_designer, SIGNAL(itemInserted(QString)), this, SLOT(slotItemInserted(QString)));

    populateToolbar( tb );
}

void ReportDesignPanel::populateToolbar( KToolBar *tb )
{
    tb->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    QAction *a = 0;

    a =  KStandardAction::cut( this );
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotEditCut()));
    tb->addAction( a );

    a =  KStandardAction::copy( this );
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotEditCopy()));
    tb->addAction( a );

    a =  KStandardAction::paste( this );
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotEditPaste()));
    tb->addAction( a );

    const KGuiItem del = KStandardGuiItem::del();
    a = new QAction( del.icon(), del.text(), this );
    a->setToolTip( del.toolTip() );
    a->setShortcut( QKeySequence::Delete );
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotEditDelete()));
    tb->addAction( a );

    tb->addSeparator();

    a = new QAction(koIcon("arrow-up"), i18n("Raise"), this);
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotRaiseSelected()));
    tb->addAction( a );
    a = new QAction(koIcon("arrow-down"), i18n("Lower"), this);
    connect(a, SIGNAL(triggered(bool)), m_designer, SLOT(slotLowerSelected()));
    tb->addAction( a );

    tb->addSeparator();

    a = new QAction(koIcon("document-properties"), i18n("Section Editor"), this);
    a->setObjectName("sectionedit");
    tb->addAction( a );

    tb->addSeparator();

    m_actionGroup = new QActionGroup(tb);
    // allow only the following item types, there is not appropriate data for others
    const QStringList itemtypes = QStringList()
        << QLatin1String("org.kde.kreport.line")
        << QLatin1String("org.kde.kreport.label")
        << QLatin1String("org.kde.kreport.field")
        << QLatin1String("org.kde.kreport.text")
        << QLatin1String("org.kde.kreport.check")
        << QLatin1String("org.kde.kreport.chart")
        << QLatin1String("org.kde.kreport.web")
        << QLatin1String(""); //separator
    foreach( QAction *a, m_designer->itemActions(m_actionGroup) ) {
        if ( ! itemtypes.contains( a->objectName() ) ) {
            m_actionGroup->removeAction( a );
            continue;
        }
        tb->addAction( a );
        connect( a, SIGNAL(triggered(bool)), SLOT(slotInsertAction()) );
    }
}

void ReportDesignPanel::slotPropertySetChanged()
{
    debugPlan<<m_propertyeditor;
    if ( m_propertyeditor ) {
        m_propertyeditor->changeSet( m_designer->itemPropertySet() );
    }
}

void ReportDesignPanel::slotInsertAction()
{
    emit insertItem( sender()->objectName() );
}

void ReportDesignPanel::slotItemInserted(const QString &)
{
    if (m_actionGroup->checkedAction())  {
        m_actionGroup->checkedAction()->setChecked(false);
    }
}

void ReportDesignPanel::setReportData( const QString &tag )
{
    m_designer->setReportData( createReportData( tag ) );
}

QDomDocument ReportDesignPanel::document() const
{
    QDomDocument document( "planreportdefinition" );
    document.appendChild( document.createProcessingInstruction(
                              "xml",
                              "version=\"1.0\" encoding=\"UTF-8\"" ) );

    QDomElement e = document.createElement( "planreportdefinition" );
    e.setAttribute( "editor", "Plan" );
    e.setAttribute( "mime", "application/x-vnd.kde.plan.report.definition" );
    e.setAttribute( "version", "1.0" );
    document.appendChild( e );

    if ( m_sourceeditor ) {
        m_sourceeditor->sourceData( e );
    }
    e.appendChild( m_designer->document() );
/*    debugPlan<<"ReportDesignerView::document:";
    debugPlan<<document.toString();*/
    return document;
}

ReportData *ReportDesignPanel::createReportData( const QString &type )
{
    ReportData *r = Report::findReportData( m_reportdatamodels, type );
    Q_ASSERT( r );
    return r;
}

QStandardItemModel *ReportDesignPanel::createSourceModel( QObject *parent ) const
{
    QStandardItemModel *m = new QStandardItemModel( parent );

    QStandardItem *item = new QStandardItem( i18n( "Tasks" ) );
    item->setData( "tasks", Reports::TagRole );
    item->setEditable( false );
    m->appendRow( item );

    item = new QStandardItem( i18n( "Task status" ) );
    item->setData( "taskstatus", Reports::TagRole );
    item->setEditable( false );
    m->appendRow( item );

    item = new QStandardItem( i18n( "Resource assignments" ) );
    item->setData( "resourceassignments", Reports::TagRole );
    item->setEditable( false );
    m->appendRow( item );

    item = new QStandardItem( i18n( "Resources" ) );
    item->setData( "resources", Reports::TagRole );
    item->setEditable( false );
    m->appendRow( item );

    return m;
}


//-------------------
ModifyReportDefinitionCmd ::ModifyReportDefinitionCmd( ReportView *view, const QDomDocument &value, const KUndo2MagicString& name )
    : NamedCommand( name ),
    m_view( view ),
    m_newvalue( value.cloneNode().toDocument() ),
    m_oldvalue( m_view->document().cloneNode().toDocument() )
{
}
void ModifyReportDefinitionCmd ::execute()
{
    m_view->loadXML( m_newvalue );
}
void ModifyReportDefinitionCmd ::unexecute()
{
    m_view->loadXML( m_oldvalue );
}

//--------------------------

ReportDesigner::ReportDesigner(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_designer( 0 ),
    m_reportdatamodels( Report::createBaseReportDataModels() ),
    m_groupsectioneditor( new GroupSectionEditor( this ) )
{
    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin(0);
    m_scrollarea = new QScrollArea( this );
    l->addWidget( m_scrollarea );

    setupGui();
    QDomDocument domdoc;
    domdoc.setContent( QString( "<planreportdefinition version=\"1.0\" mime=\"application/x-vnd.kde.plan.report.definition\" editor=\"Plan<\">"
        "<data-source select-from=\"tasks\"/>"
        "<report:content xmlns:report=\"http://kexi-project.org/report/2.0\" "
        "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" "
        "xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\">"
        "<report:title>Report</report:title>"
        "</report:content>"
        "</planreportdefinition>" ) );
    setData( domdoc );
}

void ReportDesigner::setupGui()
{
    /*KActionCollection *coll = actionCollection();*/
    QAction *a = 0;
    QString name = "edit_copypaste";

    a =  KStandardAction::cut( this );
    connect(a, SIGNAL(triggered(bool)), this, SIGNAL(cutActivated()));
    addAction( name, a );

    a =  KStandardAction::copy( this );
    connect(a, SIGNAL(triggered(bool)), this, SIGNAL(copyActivated()));
    addAction( name, a );

    a =  KStandardAction::paste( this );
    connect(a, SIGNAL(triggered(bool)), this, SIGNAL(pasteActivated()));
    addAction( name, a );

    const KGuiItem del = KStandardGuiItem::del();
    a = new QAction(del.icon(), del.text(), this);
    a->setObjectName( "edit_delete" );
    a->setToolTip(del.toolTip());
    a->setShortcut( QKeySequence::Delete );
    connect(a, SIGNAL(triggered(bool)), this, SIGNAL(deleteActivated()));
    addAction( name, a );

    name = "reportdesigner_list";
    a = new QAction(koIcon("go-previous-view"), i18n("View report"), this);
    a->setObjectName( "view_report" );
    connect(a, SIGNAL(triggered(bool)), SIGNAL(viewReport()));
    addAction( name, a );

    m_undoaction = new QAction(koIcon("edit-undo"), i18n("Undo all changes"), this);
    m_undoaction->setObjectName( "undo_all_changes" );
    m_undoaction->setEnabled( false );
    connect(m_undoaction, SIGNAL(triggered(bool)), SLOT(undoAllChanges()));
    addAction( name, m_undoaction );
    createDockers();
}

void ReportDesigner::undoAllChanges()
{
    if ( isModified() ) {
        setData();
    }
}

void ReportDesigner::slotModified()
{
    m_undoaction->setEnabled( isModified() );
}

bool ReportDesigner::isModified() const
{
    return m_designer->isModified();
}

void ReportDesigner::setModified( bool on )
{
    m_designer->setModified( on );
    m_undoaction->setEnabled( on );
}


void ReportDesigner::setData( const QDomDocument &doc )
{
    m_original = doc.cloneNode().toDocument();
    setData();
}

void ReportDesigner::setData()
{
    delete m_designer;
    QDomElement e = m_original.documentElement().firstChildElement( "report:content" );
    if ( e.isNull() ) {
        m_designer = new KReportDesigner( m_scrollarea );
    } else {
        m_designer = new KReportDesigner( m_scrollarea, e );
    }
    m_scrollarea->setWidget( m_designer );

    m_sourceeditor->setSourceData( m_original.documentElement().firstChildElement( "data-source" ) );
    blockSignals( true );
    setReportData( m_sourceeditor->selectFromTag() );
    blockSignals( false );
    slotPropertySetChanged();

    connect(m_designer, SIGNAL(dirty()), SLOT(slotModified()));
    connect(m_designer, SIGNAL(propertySetChanged()), SLOT(slotPropertySetChanged()));
    connect(m_designer, SIGNAL(itemInserted(QString)), this, SLOT(slotItemInserted(QString)));

    connect(this, SIGNAL(cutActivated()), m_designer, SLOT(slotEditCut()));
    connect(this, SIGNAL(copyActivated()), m_designer, SLOT(slotEditCopy()));
    connect(this, SIGNAL(pasteActivated()), m_designer, SLOT(slotEditPaste()));
    connect(this, SIGNAL(deleteActivated()), m_designer, SLOT(slotEditDelete()));

    m_designer->setModified( false );
    slotModified();
}

QDomDocument ReportDesigner::document() const
{
    QDomDocument document( "planreportdefinition" );
    document.appendChild( document.createProcessingInstruction(
        "xml",
        "version=\"1.0\" encoding=\"UTF-8\"" ) );

    QDomElement e = document.createElement( "planreportdefinition" );
    e.setAttribute( "editor", "Plan" );
    e.setAttribute( "mime", "application/x-vnd.kde.plan.report.definition" );
    e.setAttribute( "version", "1.0" );
    document.appendChild( e );

    if ( m_sourceeditor ) {
        m_sourceeditor->sourceData( e );
    }
    e.appendChild( m_designer->document() );
    /*    debugPlan<<"ReportDesignerView::document:";
     *    debugPlan<<document.toString();*/
    return document;
}

void ReportDesigner::createDockers()
{
    // Add dockers
    DockWidget *dw;
    QWidget *w;

    dw = new DockWidget( this, "Tools", i18nc( "@title:window report group edit tools", "Tools" ) );
    dw->setLocation( Qt::LeftDockWidgetArea );
    w = new QWidget( dw );
    Ui::ReportToolsWidget tw;
    tw.setupUi( w );

    // allow only the following item types, there is not appropriate data for others
    const QStringList itemtypes = QStringList()
        << QLatin1String("org.kde.kreport.line")
        << QLatin1String("org.kde.kreport.label")
        << QLatin1String("org.kde.kreport.field")
        << QLatin1String("org.kde.kreport.text")
        << QLatin1String("org.kde.kreport.check")
        << QLatin1String("org.kde.kreport.chart")
        << QLatin1String("org.kde.kreport.web");
    QActionGroup *ag = new QActionGroup( this );
    int i = 0;
    foreach( QAction *a, m_designer->itemActions( ag ) ) {
        if ( itemtypes.contains( a->objectName() ) ) {
            QToolButton *tb = new QToolButton( w );
            tb->setObjectName( a->objectName() );
            tb->setIcon( a->icon() );
            tb->setText( a->text() );
            if ( tb->objectName() == QLatin1String("org.kde.kreport.web") ) {
                tb->setToolTip( i18nc( "@into:tooltip", "Rich text" ) );
            } else {
                tb->setToolTip( a->toolTip() );
            }
            tb->setCheckable( true );
            tw.horizontalLayout->insertWidget( i++, tb );
            connect(tb, SIGNAL(clicked(bool)), SLOT(slotInsertAction()));
            connect(this, SIGNAL(resetButtonState(bool)), tb, SLOT(setChecked(bool)));
        }
    }

    m_sourceeditor = tw.sourceEditor;
    m_sourceeditor->setModel( createSourceModel( m_sourceeditor ) );
    connect(m_sourceeditor, SIGNAL(selectFromChanged(QString)), SLOT(setReportData(QString)));

    m_propertyeditor = tw.propertyEditor;

    dw->setWidget( w );
    addDocker( dw );

    dw = new DockWidget( this, "Sections", i18nc( "@title:window report section docker", "Headers && Footers" ) );
    dw->setLocation( Qt::RightDockWidgetArea );
    w = new QScrollArea( dw );
    Ui::ReportSectionsWidget sw;
    sw.setupUi( w );
    dw->setWidget( w );
    connect(sw.reportheader, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.reportfooter, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.headerFirstpage, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.headerLastpage, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.headerOddpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.headerEvenpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.headerAllpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.footerFirstpage, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.footerLastpage, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.footerOddpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.footerEvenpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));
    connect(sw.footerAllpages, SIGNAL(toggled(bool)), this, SLOT(slotSectionToggled(bool)));

    addDocker( dw );

    dw = new DockWidget( this, "Groups", i18nc( "@title:window report group section docker", "Groups" ) );
    dw->setLocation( Qt::RightDockWidgetArea );
    w = new QWidget( dw );
    m_groupsectioneditor->setupUi( w );

    dw->setWidget( w );

    addDocker( dw );
}

void ReportDesigner::setReportData( const QString &tag )
{

    ReportData *rd = Report::findReportData( m_reportdatamodels, tag );
    if ( rd != m_designer->reportData() ) {
        emit optionsModified();
    }
    m_designer->setReportData( rd );

    m_groupsectioneditor->setData( m_designer, rd );
}

QStandardItemModel *ReportDesigner::createSourceModel( QObject *parent ) const
{
    QStandardItemModel *m = new QStandardItemModel( parent );
    foreach ( ReportData *r, m_reportdatamodels ) {
        if ( r->isMainDataSource() ) {
            QStandardItem *item = new QStandardItem( r->sourceName() );
            item->setData( r->objectName(), Reports::TagRole );
            item->setEditable( false );
            m->appendRow( item );
        }
    }
    return m;
}

void ReportDesigner::slotPropertySetChanged()
{
    if ( m_propertyeditor ) {
        m_propertyeditor->changeSet( m_designer->itemPropertySet() );
    }
}

void ReportDesigner::slotInsertAction()
{
    m_designer->slotItem( sender()->objectName() );
}

void ReportDesigner::slotItemInserted(const QString &)
{
    emit resetButtonState( false );
}

void ReportDesigner::slotSectionToggled( bool on )
{
    QString n = sender()->objectName();
    if ( n == "reportheader" ) {
        on ? m_designer->insertSection( KReportSectionData::ReportHeader )
           : m_designer->removeSection( KReportSectionData::ReportHeader );
    } else if ( n == "reportfooter" ) {
        on ? m_designer->insertSection( KReportSectionData::ReportFooter )
        : m_designer->removeSection( KReportSectionData::ReportFooter );
    } else if ( n == "headerFirstpage" ) {
        on ? m_designer->insertSection( KReportSectionData::PageHeaderFirst )
        : m_designer->removeSection( KReportSectionData::PageHeaderFirst );
    } else if ( n == "headerLastpage" ) {
        on ? m_designer->insertSection( KReportSectionData::PageHeaderLast )
        : m_designer->removeSection( KReportSectionData::PageHeaderLast );
    } else if ( n == "headerOddpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageHeaderOdd )
        : m_designer->removeSection( KReportSectionData::PageHeaderOdd );
    } else if ( n == "headerEvenpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageHeaderEven )
        : m_designer->removeSection( KReportSectionData::PageHeaderEven );
    } else if ( n == "headerAllpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageHeaderAny )
        : m_designer->removeSection( KReportSectionData::PageHeaderAny );
    } else if ( n == "footerFirstpage" ) {
        on ? m_designer->insertSection( KReportSectionData::PageFooterFirst )
        : m_designer->removeSection( KReportSectionData::PageFooterFirst );
    } else if ( n == "footerLastpage" ) {
        on ? m_designer->insertSection( KReportSectionData::PageFooterLast )
        : m_designer->removeSection( KReportSectionData::PageFooterLast );
    } else if ( n == "footerOddpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageFooterOdd )
        : m_designer->removeSection( KReportSectionData::PageFooterOdd );
    } else if ( n == "footerEvenpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageFooterEven )
        : m_designer->removeSection( KReportSectionData::PageFooterEven );
    } else if ( n == "footerAllpages" ) {
        on ? m_designer->insertSection( KReportSectionData::PageFooterAny )
        : m_designer->removeSection( KReportSectionData::PageFooterAny );
    } else {
        debugPlan<<"unknown section";
    }
}

bool ReportDesigner::loadContext(const KoXmlElement& context)
{
    KoXmlElement e = context.namedItem( "planreportdefinition" ).toElement();
    if ( e.isNull() ) {
        e = context.namedItem( "kplatoreportdefinition" ).toElement();
    }
    if ( ! e.isNull() ) {
        QDomDocument doc( "context" );
        KoXml::asQDomElement( doc, e );
        setData( doc );
    } else {
        debugPlan<<"Invalid context xml";
        setData( QDomDocument() ); // create an empty designer
    }
    return true;
}

void ReportDesigner::saveContext(QDomElement& context) const
{
    context.appendChild( document().documentElement().cloneNode() );
}

//---------------------
GroupSectionEditor::GroupSectionEditor( QObject *parent )
    : QObject( parent ),
    designer( 0 ),
    reportdata( 0 )
{
    clear();
}

void GroupSectionEditor::setupUi( QWidget *widget )
{
    gsw.setupUi( widget );
    gsw.view->setModel( &model );
    gsw.view->setItemDelegateForColumn( 0, new EnumDelegate( gsw.view ) );
    gsw.view->setItemDelegateForColumn( 1, new CheckStateItemDelegate( gsw.view ) );
    gsw.view->setItemDelegateForColumn( 2, new EnumDelegate( gsw.view ) );
    gsw.view->setItemDelegateForColumn( 3, new EnumDelegate( gsw.view ) );
    gsw.view->setItemDelegateForColumn( 4, new EnumDelegate( gsw.view ) );

    gsw.btnAdd->setIcon(koIcon("list-add"));
    gsw.btnRemove->setIcon(koIcon("list-remove"));
    gsw.btnMoveUp->setIcon(koIcon("arrow-up"));
    gsw.btnMoveDown->setIcon(koIcon("arrow-down"));

    gsw.btnRemove->setEnabled( false );
    gsw.btnMoveUp->setEnabled( false );
    gsw.btnMoveDown->setEnabled( false );

    connect(gsw.view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(slotSelectionChanged(QItemSelection)));
    connect(gsw.btnAdd, SIGNAL(clicked(bool)), SLOT(slotAddRow()));
    connect(gsw.btnRemove, SIGNAL(clicked(bool)), SLOT(slotRemoveRows()));
    connect(gsw.btnMoveUp, SIGNAL(clicked(bool)), SLOT(slotMoveRowUp()));
    connect(gsw.btnMoveDown, SIGNAL(clicked(bool)), SLOT(slotMoveRowDown()));
}

void GroupSectionEditor::slotSelectionChanged( const QItemSelection &sel )
{
    QItemSelectionModel *m = gsw.view->selectionModel();
    gsw.btnRemove->setEnabled( ! sel.isEmpty() );
    gsw.btnMoveUp->setEnabled( ! sel.isEmpty() && ! m->isRowSelected( 0, QModelIndex() ) );
    gsw.btnMoveDown->setEnabled( ! sel.isEmpty() && ! m->isRowSelected( model.rowCount() - 1, QModelIndex() ) );
}

void GroupSectionEditor::clear()
{
    model.clear();
    QStringList n;
    n << i18nc( "@title:column", "Column" )
        << i18nc( "@title:column", "Sort" )
        << i18nc( "@title:column", "Header" )
        << i18nc( "@title:column", "Footer" )
        << i18nc( "@title:column", "Page Break" );
    model.setHorizontalHeaderLabels( n );

    model.setHeaderData( 0, Qt::Horizontal, i18nc( "@info:tooltip", "Groups data by the selected column" ), Qt::ToolTipRole );
    model.setHeaderData( 1, Qt::Horizontal, i18nc( "@info:tooltip", "Sorts data" ), Qt::ToolTipRole );
    model.setHeaderData( 2, Qt::Horizontal, i18nc( "@info:tooltip", "Show header section" ), Qt::ToolTipRole );
    model.setHeaderData( 3, Qt::Horizontal, i18nc( "@info:tooltip", "Show footer section" ), Qt::ToolTipRole );
    model.setHeaderData( 4, Qt::Horizontal, i18nc( "@info:tooltip", "Insert page break" ), Qt::ToolTipRole );
}

void GroupSectionEditor::setData( KReportDesigner *d, ReportData *rd )
{
    clear();
    designer = d;
    reportdata = rd;
    KReportDesignerSectionDetail *sd = designer->detailSection();
    if ( ! sd ) {
        return;
    }
    for (int i = 0; i < sd->groupSectionCount(); i++) {
        KReportDesignerSectionDetailGroup *g = sd->groupSection( i );
        ColumnItem *ci = new ColumnItem( g );
        ci->names = rd->fieldNames();
        ci->keys = rd->fieldKeys();

        SortItem *si = new SortItem( g );
        HeaderItem *hi = new HeaderItem( g );
        FooterItem *fi = new FooterItem( g );
        PageBreakItem *pi = new PageBreakItem( g );

        model.appendRow( QList<QStandardItem*>() << ci << si << hi << fi << pi );
    }
}

void GroupSectionEditor::slotAddRow()
{
    KReportDesignerSectionDetail *sd = designer->detailSection();
    if ( ! sd ) {
        return;
    }
    KReportDesignerSectionDetailGroup * g = new KReportDesignerSectionDetailGroup( reportdata->fieldKeys().value( 0 ), sd, sd );

    sd->insertGroupSection( sd->groupSectionCount(), g );

    ColumnItem *ci = new ColumnItem( g );
    ci->names = reportdata->fieldNames();
    ci->keys = reportdata->fieldKeys();

    SortItem *si = new SortItem( g );
    HeaderItem *hi = new HeaderItem( g );
    FooterItem *fi = new FooterItem( g );
    PageBreakItem *pi = new PageBreakItem( g );

    model.appendRow( QList<QStandardItem*>() << ci << si << hi << fi << pi );
}

void GroupSectionEditor::slotRemoveRows()
{
    KReportDesignerSectionDetail *sd = designer->detailSection();
    if ( ! sd ) {
        return;
    }
    QList<int> rows;
    foreach ( const QModelIndex &idx, gsw.view->selectionModel()->selectedRows() ) {
        rows <<idx.row();
    }
    qSort( rows );
    for (int i = rows.count() - 1; i >= 0; --i ) {
        int row = rows.at( i );
        QList<QStandardItem*> items = model.takeRow( row );
        sd->removeGroupSection( row, true );
        qDeleteAll( items );
    }
}

void GroupSectionEditor::slotMoveRowUp()
{
    KReportDesignerSectionDetail *sd = designer->detailSection();
    if ( ! sd ) {
        return;
    }
    QList<int> rows;
    foreach ( const QModelIndex &idx, gsw.view->selectionModel()->selectedRows() ) {
        rows <<idx.row();
    }
    qSort( rows );
    if ( rows.isEmpty() || rows.first() == 0 ) {
        return;
    }
    foreach ( int row, rows ) {
        QList<QStandardItem*> items = model.takeRow( row );
        KReportDesignerSectionDetailGroup *g = sd->groupSection( row );
        bool showgh = g->groupHeaderVisible();
        bool showgf = g->groupFooterVisible();
        sd->removeGroupSection( row );
        sd->insertGroupSection( row - 1, g );
        g->setGroupHeaderVisible( showgh );
        g->setGroupFooterVisible( showgf );
        model.insertRow( row - 1, items );
    }
    QModelIndex idx1 = model.index( rows.first()-1, 0 );
    QModelIndex idx2 = model.index( rows.last()-1, 0 );
    QItemSelection s = QItemSelection ( idx1, idx2 );
    gsw.view->selectionModel()->select( s, QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select );
}


void GroupSectionEditor::slotMoveRowDown()
{
    KReportDesignerSectionDetail *sd = designer->detailSection();
    if ( ! sd ) {
        return;
    }
    QList<int> rows;
    foreach ( const QModelIndex &idx, gsw.view->selectionModel()->selectedRows() ) {
        rows <<idx.row();
    }
    qSort( rows );
    if ( rows.isEmpty() || rows.last() >= model.rowCount() - 1 ) {
        return;
    }
    for ( int i = rows.count() - 1; i >= 0; --i ) {
        int row = rows.at( i );
        QList<QStandardItem*> items = model.takeRow( row );
        KReportDesignerSectionDetailGroup *g = sd->groupSection( row );
        bool showgh = g->groupHeaderVisible();
        bool showgf = g->groupFooterVisible();
        sd->removeGroupSection( row );
        sd->insertGroupSection( row + 1, g );
        g->setGroupHeaderVisible( showgh );
        g->setGroupFooterVisible( showgf );
        model.insertRow( row + 1, items );
    }
    QModelIndex idx1 = model.index( rows.first()+1, 0 );
    QModelIndex idx2 = model.index( rows.last()+1, 0 );
    QItemSelection s = QItemSelection ( idx1, idx2 );
    gsw.view->selectionModel()->select( s, QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select );
}

//----------------
GroupSectionEditor::ColumnItem::ColumnItem( KReportDesignerSectionDetailGroup *g )
    : Item( g )
{
}

QVariant GroupSectionEditor::ColumnItem::data( int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return names.value( keys.indexOf( group->column() ) );
        case Role::EnumList: return names;
        case Role::EnumListValue: return keys.indexOf( group->column() );
        default: break;
    }
    return Item::data( role );
}

void GroupSectionEditor::ColumnItem::setData( const QVariant &value, int role )
{
    if ( role == Qt::EditRole ) {
        group->setColumn( keys.value( value.toInt() ) );
        return;
    }
    return Item::setData( value, role );
}

//---------------------
GroupSectionEditor::SortItem::SortItem( KReportDesignerSectionDetailGroup *g )
    : Item( g )
{
    names << i18n( "Ascending" ) << i18n( "Descending" );
}

QVariant GroupSectionEditor::SortItem::data( int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return QVariant();
        case Qt::ToolTipRole: return group->sort() ? names.value( 1 ) : names.value( 0 );
        case Qt::DecorationRole: return group->sort() ? koIcon("arrow-down") : koIcon("arrow-up");
        case Qt::EditRole: return group->sort() ? Qt::Unchecked : Qt::Checked;
        case Role::EnumList: return names;
        case Role::EnumListValue: return  group->sort() ? 1 : 0;
        default: break;
    }
    return Item::data( role );
}

void GroupSectionEditor::SortItem::setData( const QVariant &value, int role )
{
    if ( role == Qt::EditRole ) {
        group->setSort( value.toInt() == 0 ? Qt::AscendingOrder : Qt::DescendingOrder );
        return;
    } else if ( role == Qt::CheckStateRole ) {
        group->setSort( value.toInt() == 0 ? Qt::DescendingOrder : Qt::AscendingOrder );
        return;
    }
    return Item::setData( value, role );
}

//---------------------
GroupSectionEditor::HeaderItem::HeaderItem( KReportDesignerSectionDetailGroup *g )
    : Item( g )
{
    names << i18n( "No" ) << i18n( "Yes" );
    setCheckable( true );
}

QVariant GroupSectionEditor::HeaderItem::data( int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return QVariant();
        case Qt::CheckStateRole: return  group->groupHeaderVisible() ? Qt::Checked : Qt::Unchecked;
        case Role::EnumList: return names;
        case Role::EnumListValue: return  group->groupHeaderVisible() ? 1 : 0;
        default: break;
    }
    return Item::data( role );
}

void GroupSectionEditor::HeaderItem::setData( const QVariant &value, int role )
{
    debugPlan<<value<<role;
    if ( role == Qt::EditRole ) {
        group->setGroupHeaderVisible( value.toInt() == 1 );
        return;
    } else if ( role == Qt::CheckStateRole ) {
        group->setGroupHeaderVisible( value.toInt() > 0 );
        return;
    }
    return Item::setData( value, role );
}

//---------------------
GroupSectionEditor::FooterItem::FooterItem( KReportDesignerSectionDetailGroup *g )
    : Item( g )
{
    names << i18n( "No" ) << i18n( "Yes" );
    setCheckable( true );
}

QVariant GroupSectionEditor::FooterItem::data( int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return QVariant();
        case Qt::CheckStateRole: return group->groupFooterVisible() ? Qt::Checked : Qt::Unchecked;
        case Role::EnumList: return names;
        case Role::EnumListValue: return group->groupFooterVisible() ? 1 : 0;
        default: break;
    }
    return Item::data( role );
}

void GroupSectionEditor::FooterItem::setData( const QVariant &value, int role )
{
    if ( role == Qt::EditRole ) {
        group->setGroupFooterVisible( value.toInt() == 1 );
        return;
    } else if ( role == Qt::CheckStateRole ) {
        group->setGroupFooterVisible( value.toInt() > 0 );
        return;
    }
    return Item::setData( value, role );
}

//---------------------
GroupSectionEditor::PageBreakItem::PageBreakItem( KReportDesignerSectionDetailGroup *g )
    : Item( g )
{
    names << i18n( "None" ) << i18n( "After footer" ) << i18n( "Before header" );
}

QVariant GroupSectionEditor::PageBreakItem::data( int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return names.value( (int)group->pageBreak() );
        case Qt::ToolTipRole: return names.value( (int)group->pageBreak() );
        case Role::EnumList: return names;
        case Role::EnumListValue: return (int)group->pageBreak();
        default: break;
    }
    return Item::data( role );
}

void GroupSectionEditor::PageBreakItem::setData( const QVariant &value, int role )
{
    if ( role == Qt::EditRole ) {
        group->setPageBreak( (KReportDesignerSectionDetailGroup::PageBreak)( value.toInt() ) );
        return;
    }
    return Item::setData( value, role );
}

} // namespace KPlato
