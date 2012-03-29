/* This file is part of the Calligra project
   Copyright (C) 2002 Werner Trobin <trobin@kde.org>
   Copyright (C) 2002 David Faure <faure@kde.org>
   Copyright (C) 2008 Benjamin Cail <cricketc@gmail.com>
   Copyright (C) 2009 Inge Wallin   <inge@lysator.liu.se>
   Copyright (C) 2010 Boudewijn Rempt <boud@kogmbh.com>
   Copyright (C) 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the Library GNU General Public
   version 2 of the License, or (at your option) version 3 or,
   at the discretion of KDE e.V (which shall act as a proxy as in
   section 14 of the GPLv3), any later version..

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "generated/leinputstream.h"
#include "drawstyle.h"

#include "document.h"
#include "conversion.h"
#include "texthandler.h"
#include "graphicshandler.h"
//#include "versionmagic.h"
#include "mswordodfimport.h"
#include "msodraw.h"
#include "msdoc.h"

#include <KoUnit.h>
#include <KoPageLayout.h>
#include <kdebug.h>

#include <wv2/src/styles.h>
#include <wv2/src/ustring.h>
#include <wv2/src/word97_generated.h>
#include <wv2/src/parser.h>
#include <wv2/src/parserfactory.h>
#include <wv2/src/paragraphproperties.h>
#include <wv2/src/associatedstrings.h>

#include <klocale.h>
#include <KoStore.h>
#include <KoFontFace.h>

#include <QBuffer>
#include <QColor>

//TODO: provide all streams to the wv2 parser; POLE storage is going to replace
//OLE storage soon!
Document::Document(const std::string& fileName,
                   MSWordOdfImport* filter,
//                    KoFilterChain* chain,
                   KoXmlWriter* bodyWriter, KoXmlWriter* metaWriter, KoXmlWriter* manifestWriter,
                   KoStore* store, KoGenStyles* mainStyles,
                   LEInputStream& wordDocument, POLE::Stream& table, LEInputStream* data)
        : m_textHandler(0)
        , m_tableHandler(0)
        , m_replacementHandler(new WordsReplacementHandler)
        , m_graphicsHandler(0)
        , m_filter(filter)
//         , m_chain(chain)
        , m_parser(wvWare::ParserFactory::createParser(fileName))
        , m_bodyFound(false)
        , m_footNoteNumber(0)
        , m_endNoteNumber(0)
        , m_bodyWriter(0)
        , m_mainStyles(0)
        , m_metaWriter(0)
        , m_headerWriter(0)
        , m_headerCount(0)
        , m_writingHeader(false)
        , m_evenOpen(false)
        , m_firstOpen(false)
        , m_buffer(0)
        , m_bufferEven(0)
        , m_writeMasterPageName(false)
        , m_omittMasterPage(false)
        , m_useLastMasterPage(false)
        , m_wdstm(wordDocument)
        , m_tblstm(0)
        , m_datastm(data)
        , m_tblstm_pole(table)
{
    kDebug(30513);
    addBgColor("#ffffff"); //initialize the background-colors stack

    if (m_parser) { // 0 in case of major error (e.g. unsupported format)

        m_bodyWriter = bodyWriter; //pointer for writing to the body
        m_mainStyles = mainStyles; //KoGenStyles object for collecting styles
        m_metaWriter = metaWriter; //pointer for writing to meta.xml
        m_buffer = 0; //set pointers to 0
        m_bufferEven = 0;
        m_headerWriter = 0;

        m_textHandler  = new WordsTextHandler(m_parser, bodyWriter, mainStyles);
        m_textHandler->setDocument(this);
        m_tableHandler = new WordsTableHandler(bodyWriter, mainStyles);
        m_tableHandler->setDocument(this);
        m_graphicsHandler = new WordsGraphicsHandler(this, bodyWriter, manifestWriter, store, mainStyles,
                                                     m_parser->getDrawings(), m_parser->fib());

        connect(m_textHandler, SIGNAL(subDocFound(const wvWare::FunctorBase*, int)),
                this, SLOT(slotSubDocFound(const wvWare::FunctorBase*, int)));
        connect(m_textHandler, SIGNAL(footnoteFound(const wvWare::FunctorBase*, int)),
                this, SLOT(slotFootnoteFound(const wvWare::FunctorBase*, int)));
        connect(m_textHandler, SIGNAL(annotationFound(const wvWare::FunctorBase*,int)),
                this, SLOT(slotAnnotationFound(const wvWare::FunctorBase*, int)));
        connect(m_textHandler, SIGNAL(headersFound(const wvWare::FunctorBase*, int)),
                this, SLOT(slotHeadersFound(const wvWare::FunctorBase*, int)));
        connect(m_textHandler, SIGNAL(tableFound(Words::Table*)),
                this, SLOT(slotTableFound(Words::Table*)));
        connect(m_textHandler, SIGNAL(inlineObjectFound(const wvWare::PictureData&, KoXmlWriter*)),
                this, SLOT(slotInlineObjectFound(const wvWare::PictureData&, KoXmlWriter*)));
        connect(m_textHandler, SIGNAL(floatingObjectFound(unsigned int, KoXmlWriter*)),
                this, SLOT(slotFloatingObjectFound(unsigned int, KoXmlWriter*)));
        connect(m_graphicsHandler, SIGNAL(textBoxFound(unsigned int, bool)),
                this, SLOT(slotTextBoxFound(unsigned int, bool)));

        m_parser->setSubDocumentHandler(this);
        m_parser->setTextHandler(m_textHandler);
        m_parser->setTableHandler(m_tableHandler);
        m_parser->setGraphicsHandler(m_graphicsHandler);
        m_parser->setInlineReplacementHandler(m_replacementHandler);

        processStyles();
        processAssociatedStrings();

//         connect( m_tableHandler,
//                  SIGNAL( sigTableCellStart( int, int, int, int, const QRectF&, const QString&,
//                          const wvWare::Word97::BRC&, const wvWare::Word97::BRC&, const wvWare::Word97::BRC&,
//                          const wvWare::Word97::BRC&, const wvWare::Word97::SHD& ) ),
//                  this,
//                  SLOT( slotTableCellStart( int, int, int, int, const QRectF&, const QString&,
//                        const wvWare::Word97::BRC&, const wvWare::Word97::BRC&, const wvWare::Word97::BRC&,
//                        const wvWare::Word97::BRC&, const wvWare::Word97::SHD& ) ) );
//         connect( m_tableHandler, SIGNAL( sigTableCellEnd() ), this, SLOT( slotTableCellEnd() ) );
    }
}

Document::~Document()
{
    delete m_textHandler;
    delete m_tableHandler;
    delete m_replacementHandler;
    delete m_graphicsHandler;
    //expecting the background-color of the document on top of the stack
    Q_ASSERT(m_bgColors.size() == 1);
    m_bgColors.clear();
}

//set whether or not document has header or footer
//set tabstop value
//add footnote settings & endnote settings
//write out header & footer type
//write out picture information
void Document::finishDocument()
{
    kDebug(30513);

    //finish a header if we need to - this should only be necessary if there's
    //an even header w/o an odd header
//     if (m_oddOpen) {
//         QString contents = QString::fromUtf8(m_buffer->buffer(), m_buffer->buffer().size());
//         m_masterStyle->addChildElement(QString::number(m_headerCount), contents);

//         m_oddOpen = false;
//         delete m_headerWriter;
//         m_headerWriter = 0;
//         delete m_buffer;
//         m_buffer = 0;
//         //we're done with this header, so reset to false
//         m_writingHeader = false;
//     }

    const wvWare::Word97::DOP& dop = m_parser->dop();

    m_initialFootnoteNumber = dop.nFtn;
    m_initialEndnoteNumber = dop.nEdn;

    //"tabStopValue", (double)dop.dxaTab / 20.0

    Q_ASSERT(m_mainStyles);
    if (m_mainStyles) {

        QString footnoteConfig("<text:notes-configuration "
                               "text:note-class=\"footnote\" "
                               "text:default-style-name=\"Footnote\" "
                               "text:citation-style-name=\"Footnote_20_Symbol\" "
                               "text:citation-body-style-name=\"Footnote_20_anchor\" "
                               "text:master-page-name=\"Footnote\" "
                               "style:num-format=\"%1\" "
                               "text:start-value=\"%2\" "
                               "text:footnotes-position=\"page\" "
                               "text:start-numbering-at=\"%3\" "
                               "/>");
        //FIXME: If document that has an nFib <= 0x00D9, then use DOP.  Else
        //use the infos from SEP (sprmSFpc, sprmSRncFtn, sprmSNFtn, sprmSNfcFtnRef).
        m_mainStyles->insertRawOdfStyles(KoGenStyles::DocumentStyles,
                                         footnoteConfig.arg(Conversion::numberFormatCode(dop.nfcFtnRef2))
                                                       .arg(m_initialFootnoteNumber)
                                                       .arg(Conversion::rncToStartNumberingAt(dop.rncFtn))
                                                       .toLatin1());

        // ms-word has start-numbering-at (rncEdn) for endnotes, but ODF doesn't really support it
        QString endnoteConfig("<text:notes-configuration "
                              "text:note-class=\"endnote\" "
                              "text:default-style-name=\"Endnote\" "
                              "text:citation-style-name=\"Endnote_20_Symbol\" "
                              "text:citation-body-style-name=\"Endnote_20_anchor\" "
                              "text:master-page-name=\"Endnote\" "
                              "style:num-format=\"%1\" "
                              "text:start-value=\"%2\" "
                              //"text:start-numbering-at=\"%3\" "
                              "/>");
        //FIXME: If document that has an nFib <= 0x00D9, then use DOP.  Else
        //use the infos from SEP (sprmSFEndnote, sprmSRncEdn, sprmSNEdn, sprmSNfcEdnRef).
        m_mainStyles->insertRawOdfStyles(KoGenStyles::DocumentStyles,
                                         endnoteConfig.arg(Conversion::numberFormatCode(dop.nfcEdnRef2))
                                                      .arg(m_initialEndnoteNumber)
//                                                           .arg(Conversion::rncToStartNumberingAt(dop.rncEdn))
                                                      .toLatin1());
    }
//     QDomElement elementDoc = m_mainDocument.documentElement();
//     QDomElement element;
//     element = m_mainDocument.createElement("ATTRIBUTES");
//     element.setAttribute("processing",0); // WP
//     char allHeaders = ( wvWare::HeaderData::HeaderEven |
//                         wvWare::HeaderData::HeaderOdd |
//                         wvWare::HeaderData::HeaderFirst );
//     element.setAttribute("hasHeader", m_headerFooters & allHeaders ? 1 : 0 );
//     char allFooters = ( wvWare::HeaderData::FooterEven |
//                         wvWare::HeaderData::FooterOdd |
//                         wvWare::HeaderData::FooterFirst );
//     element.setAttribute("hasFooter", m_headerFooters & allFooters ? 1 : 0 );
//     //element.setAttribute("unit","mm"); // How to figure out the unit to use?

//     element.setAttribute("tabStopValue", (double)dop.dxaTab / 20.0 );
//     elementDoc.appendChild(element);

//     // Done at the end: write the type of headers/footers,
//     // depending on which kind of headers and footers we received.
//     QDomElement paperElement = elementDoc.namedItem("PAPER").toElement();
//     Q_ASSERT ( !paperElement.isNull() ); // slotSectionFound should have been called!
//     if ( !paperElement.isNull() ) {
//         kDebug(30513) <<"m_headerFooters=" << m_headerFooters;
//         paperElement.setAttribute("hType", Conversion::headerMaskToHType( m_headerFooters ) );
//         paperElement.setAttribute("fType", Conversion::headerMaskToFType( m_headerFooters ) );
//     }

//     // Write out <PICTURES> tag
//     QDomElement picturesElem = m_mainDocument.createElement("PICTURES");
//     elementDoc.appendChild( picturesElem );
//     for( QStringList::Iterator it = m_pictureList.begin(); it != m_pictureList.end(); ++it ) {
//         QDomElement keyElem = m_mainDocument.createElement("KEY");
//         picturesElem.appendChild( keyElem );
//         keyElem.setAttribute( "filename", *it );
//         keyElem.setAttribute( "name", *it );
//     }
}

//write document info, author, fullname, title, about
void Document::processAssociatedStrings()
{
    kDebug(30513) ;
    wvWare::AssociatedStrings strings(m_parser->associatedStrings());
    if (!strings.author().isNull()) {
        m_metaWriter->startElement("meta:initial-creator");
        m_metaWriter->addTextNode(Conversion::string(strings.author()));
        m_metaWriter->endElement();
    }
    if (!strings.title().isNull()) {
        m_metaWriter->startElement("dc:title");
        kDebug(30513) << "TITLE: " << Conversion::string(strings.title());
        m_metaWriter->addTextNode(Conversion::string(strings.title()));
        m_metaWriter->endElement();
    }
    if (!strings.subject().isNull()) {
        m_metaWriter->startElement("dc:subject");
        m_metaWriter->addTextNode(Conversion::string(strings.subject()));
        m_metaWriter->endElement();
    }
    if (!strings.lastRevBy().isNull()) {
        m_metaWriter->startElement("dc:creator");
        m_metaWriter->addTextNode(Conversion::string(strings.lastRevBy()));
        m_metaWriter->endElement();
    }
    if (!strings.keywords().isNull()) {
        m_metaWriter->startElement("meta:keyword");
        m_metaWriter->addTextNode(Conversion::string(strings.keywords()));
        m_metaWriter->endElement();
    }
    if (!strings.comments().isNull()) {
        m_metaWriter->startElement("meta:comments");
        m_metaWriter->addTextNode(Conversion::string(strings.comments()));
        m_metaWriter->endElement();
    }
}

void Document::processStyles()
{
    kDebug(30513) ;

    const wvWare::StyleSheet& styles = m_parser->styleSheet();
    unsigned int count = styles.size();
    kDebug(30513) << "styles count=" << count;

    //loop through each style
    for (unsigned int i = 0; i < count ; ++i) {
        //grab style
        const wvWare::Style* style = styles.styleByIndex(i);
        Q_ASSERT(style);
        QString displayName = Conversion::string(style->name());
        QString name = Conversion::styleName2QString(style->name());

        // if the invariant style identifier says it's a style used for line numbers
        if (style->sti() == 40) {
            m_lineNumbersStyleName = name;  // store the name of that style
        }

        // Process paragraph styles.
        if (style && style->type() == sgcPara) {
            //create this style & add formatting info to it
            kDebug(30513) << "creating ODT paragraphstyle" << name;
            KoGenStyle userStyle(KoGenStyle::ParagraphStyle, "paragraph");
            userStyle.addAttribute("style:display-name", displayName);

            const wvWare::Style* followingStyle = styles.styleByIndex(style->followingStyle());
            if (followingStyle && followingStyle != style) {
                QString followingName = Conversion::styleName2QString(followingStyle->name());
                userStyle.addAttribute("style:next-style-name", followingName);
            }

            const wvWare::Style* parentStyle = styles.styleByIndex(style->m_std->istdBase);
            if (parentStyle) {
                userStyle.setParentName(Conversion::styleName2QString(parentStyle->name()));
            }

            //set font name in style
            QString fontName = m_textHandler->getFont(style->chp().ftcAscii);
            if (!fontName.isEmpty()) {
                m_mainStyles->insertFontFace(KoFontFace(fontName));
                userStyle.addProperty(QString("style:font-name"), fontName, KoGenStyle::TextType);
            }

            // Process the paragraph and character properties.
            Paragraph::applyParagraphProperties(style->paragraphProperties(), &userStyle, parentStyle, false, 0, 0, QString());
            Paragraph::applyCharacterProperties(&style->chp(), &userStyle, parentStyle, false, false, currentBgColor());

            // Add style to main collection, using the name that it
            // had in the .doc.
            QString actualName = m_mainStyles->insert(userStyle, name, KoGenStyles::DontAddNumberToName);
            kDebug(30513) << "added style " << actualName;

            //save names of TOC related styles
            if (actualName.contains("TOC")) {
                m_tocStyleNames.append(actualName);
            }
        } else if (style && style->type() == sgcChp) {
            //create this style & add formatting info to it
            kDebug(30513) << "creating ODT textstyle" << name;
            KoGenStyle userStyle(KoGenStyle::TextStyle, "text");
            userStyle.addAttribute("style:display-name", displayName);

            const wvWare::Style* parentStyle = styles.styleByIndex(style->m_std->istdBase);
            if (parentStyle) {
                userStyle.setParentName(Conversion::styleName2QString(parentStyle->name()));
            }

            //set font name in style
            QString fontName = m_textHandler->getFont(style->chp().ftcAscii);
            if (!fontName.isEmpty()) {
                m_mainStyles->insertFontFace(KoFontFace(fontName));
                userStyle.addProperty(QString("style:font-name"), fontName, KoGenStyle::TextType);
            }

            // Process the character and paragraph properties.
            Paragraph::applyCharacterProperties(&style->chp(), &userStyle, parentStyle, false, false, currentBgColor());

            //add style to main collection, using the name that it had in the .doc
            QString actualName = m_mainStyles->insert(userStyle, name, KoGenStyles::DontAddNumberToName);
            kDebug(30513) << "added style " << actualName;
        }
    }
    //also create a defaul style which is needed to store the default tab spacing
    KoGenStyle defaultStyle(KoGenStyle::ParagraphStyle, "paragraph");
    defaultStyle.setDefaultStyle(true);
    defaultStyle.addPropertyPt("style:tab-stop-distance", (qreal)m_parser->dop().dxaTab / 20.0);
    m_mainStyles->insert(defaultStyle, "nevershown");
}

quint8 Document::parse()
{
    if (m_parser) {
        if (!m_parser->parse()) {
            return 1;
        }
    }
    //make sure texthandler is fine after parsing
    if (!m_textHandler->stateOk()) {
        kError(30513) << "TextHandler state after parsing NOT Ok!";
        return 2;
    }
    return 0;
}

void Document::setProgress(const int percent)
{
    m_filter->setProgress(percent);
}

//connects firstSectionFound signal & slot together; sets flag to true
void Document::bodyStart()
{
    kDebug(30513);
    connect(m_textHandler, SIGNAL(sectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP>)),
            this, SLOT(slotSectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP>)));
    connect(m_textHandler, SIGNAL(sectionEnd(wvWare::SharedPtr<const wvWare::Word97::SEP>)),
            this, SLOT(slotSectionEnd(wvWare::SharedPtr<const wvWare::Word97::SEP>)));
    m_bodyFound = true;
}

//disconnects firstSectionFound signal & slot
void Document::bodyEnd()
{
    //close a list if we need to
    if (m_textHandler->listIsOpen()) {
        kDebug(30513) << "closing the final list in the document body";
        m_textHandler->closeList();
    }

    disconnect(m_textHandler, SIGNAL(sectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP>)),
               this, SLOT(slotSectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP>)));
}

//create page-layout and master-page
void Document::slotSectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP> sep)
{
    kDebug(30513) ;
    m_omittMasterPage = false;
    m_useLastMasterPage = false;

    //does this section require a specific first page
    bool firstPage = sep->fTitlePage || sep->pgbApplyTo;

    // *******************************
    // page-layout style
    // *******************************
    kDebug(30513) << "preparing page-layout styles";
    KoGenStyle* pageLayoutStyle = new KoGenStyle(KoGenStyle::PageLayoutStyle);

    //set page-layout attributes
    setPageLayoutStyle(pageLayoutStyle, sep, 0);
    pageLayoutStyle->setAutoStyleInStylesDotXml(true);

    //NOTE: Each section may require a new page-layout.  If this is not the
    //case and the header/footer content didn't change, the <style:master-page>
    //element can be omitted.  Except of continuous section break a manual page
    //break has to be inserted.

    //TODO: Even page/Odd page section break support

    //FIXME: missing support for fo:break-before="page" in table properties so
    //let's omitt the <style:master-page> element only in case of a continuous
    //section break

    if ( !firstPage && !headersChanged() && (m_pageLayoutStyle_last == *pageLayoutStyle) ){

//         if (sep->bkc != 0) {
//             textHandler()->set_breakBeforePage(true);
//         }

        switch (sep->bkc) {
        case bkcContinuous:
            kDebug(30513) << "omitting page-layout & master-page creation";
            m_omittMasterPage = true;
            break;
        case bkcNewPage:
        case bkcEvenPage:
        case bkcOddPage:
            kDebug(30513) << "using the last defined master-page";
            m_useLastMasterPage = true;
            m_writeMasterPageName = true;
            break;
        default:
            kWarning(30513) << "Warning: section break type (" << sep->bkc << ") NOT SUPPORTED!";
            m_omittMasterPage = true;
            break;
        }

        //cleaning required!
        delete pageLayoutStyle;
    } else {
        //save the actual KoGenStyle!
        m_pageLayoutStyle_last = *pageLayoutStyle;

        //add data into corresponding lists
        m_pageLayoutStyle_list.prepend(pageLayoutStyle);
    }

    //yeah, we can omitt creation of a new master-page
    if (m_omittMasterPage || m_useLastMasterPage) {
        return;
    }

    //check if a first-page specific page-layout has to be created
    if (firstPage) {
       pageLayoutStyle = new KoGenStyle(KoGenStyle::PageLayoutStyle);

       //set page-layout attributes for the first page
       setPageLayoutStyle(pageLayoutStyle, sep, 1);
       pageLayoutStyle->setAutoStyleInStylesDotXml(true);

       //add data into corresponding lists
       m_pageLayoutStyle_list.prepend(pageLayoutStyle);
    }

    // *******************************
    // master-page style
    // *******************************
    KoGenStyle* masterStyle = new KoGenStyle(KoGenStyle::MasterPageStyle);
    QString masterStyleName;

    //NOTE: The first master-page-name has to be "Standard", words has hard
    //coded that the value of fo:backgroud-color from this style is used for
    //the entire frameset.
    if (m_textHandler->sectionNumber() > 1) {
        masterStyleName.append("MP");
        masterStyleName.append(QString::number(m_textHandler->sectionNumber()));
    } else {
        masterStyleName.append("Standard");
    }
    masterStyle->addAttribute("style:display-name", masterStyleName);

    //add data into corresponding lists
    m_masterPageName_list.prepend(masterStyleName);
    m_masterPageStyle_list.prepend(masterStyle);

    //initialize the header/footer list
    m_hasHeader_list.prepend(false);
    m_hasFooter_list.prepend(false);

    //check if a first-page specific master-page has to be created
    if (firstPage) {
        masterStyle = new KoGenStyle(KoGenStyle::MasterPageStyle);
        masterStyleName.clear();
        masterStyleName.append("First_Page");

        if (m_textHandler->sectionNumber() > 1) {
            masterStyleName.append(QString::number(m_textHandler->sectionNumber()));
        }
        masterStyle->addAttribute("style:display-name", masterStyleName);
        masterStyle->addAttribute("style:next-style-name", m_masterPageName_list.last());

        //add data into corresponding lists
        m_masterPageName_list.prepend(masterStyleName);
        m_masterPageStyle_list.prepend(masterStyle);

        //initialize the header/footer list
        m_hasHeader_list.prepend(false);
        m_hasFooter_list.prepend(false);
    }
    //required by handlers
    m_writeMasterPageName = true;
    //required by this module
    m_lastMasterPageName = m_masterPageName_list.first();

    for (int i = 0; i < m_masterPageName_list.size(); i++) {
        kDebug(30513) << "prepared master-page style:" << m_masterPageName_list[i];
    }
}

void Document::slotSectionEnd(wvWare::SharedPtr<const wvWare::Word97::SEP> sep)
{
    kDebug(30513);
    KoGenStyle* masterPageStyle = 0;
    KoGenStyle* pageLayoutStyle = 0;
    QString pageLayoutName;

    for (int i = 0; i < m_masterPageName_list.size(); i++) {

        pageLayoutStyle = m_pageLayoutStyle_list[i];
        masterPageStyle = m_masterPageStyle_list[i];
        Q_ASSERT(pageLayoutStyle);
        Q_ASSERT(masterPageStyle);

        //set the margins - depends on whether a header/footer is present
        if (m_hasHeader_list[i]) {
            pageLayoutStyle->addPropertyPt("fo:margin-top", (double)sep->dyaHdrTop / 20.0);
        } else if (sep->brcTop.brcType == 0) {
            pageLayoutStyle->addPropertyPt("fo:margin-top", qAbs((double)sep->dyaTop) / 20.0);
        }
        if (m_hasFooter_list[i]) {
            pageLayoutStyle->addPropertyPt("fo:margin-bottom", (double)sep->dyaHdrBottom / 20.0);
        } else if (sep->brcBottom.brcType == 0) {
            pageLayoutStyle->addPropertyPt("fo:margin-bottom", qAbs((double)sep->dyaBottom) / 20.0);
        }

        pageLayoutName = m_mainStyles->insert(*pageLayoutStyle, "Mpm");
        masterPageStyle->addAttribute("style:page-layout-name", pageLayoutName);
        m_mainStyles->insert(*masterPageStyle, m_masterPageName_list[i], KoGenStyles::DontAddNumberToName);

        //delete objects, we've added them to the collection
        delete masterPageStyle;
        delete pageLayoutStyle;
    }
    //clear lists
    m_pageLayoutStyle_list.clear();
    m_masterPageStyle_list.clear();
    m_masterPageName_list.clear();
    m_hasHeader_list.clear();
    m_hasFooter_list.clear();

    //reset header data
    m_headerCount = 0;
}

void Document::headersMask(QList<bool> mask)
{
    kDebug(30513) ;
    m_headersMask = mask;
}

//creates a frameset element with the header info
void Document::headerStart(wvWare::HeaderData::Type type)
{
    kDebug(30513) << "startHeader type=" << type << " (" << Conversion::headerTypeToFramesetName(type) << ")";
    // NOTE: According to "Word Binary File Format (.doc) Structure
    // Specification", headers are stored and therefore emitted in the
    // following order: Header Even, Header Odd, Footer Even, Footer Odd,
    // Header First, Footer First.

    m_headerCount++;
    int i = m_hasHeader_list.size() - 1; //index of the last item

    // NOTE: We are assuming in the parser code that odd header/footer is
    // present by default if m_headers is not empty.

    switch (type) {
    case wvWare::HeaderData::HeaderEven:
        //write to the buffer for even headers/footers
        m_bufferEven = new QBuffer();
        m_bufferEven->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_bufferEven);
        m_evenOpen = true;
        m_headerWriter->startElement("style:header-left");
        break;
    case wvWare::HeaderData::HeaderOdd:
        //set up buffer & writer for odd header
        m_buffer = new QBuffer();
        m_buffer->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_buffer);
        m_headerWriter->startElement("style:header");
        m_hasHeader_list.replace(i, true);
        break;
    case wvWare::HeaderData::FooterEven:
        //write to the buffer for even headers/footers
        m_bufferEven = new QBuffer();
        m_bufferEven->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_bufferEven);
        m_evenOpen = true;
        m_headerWriter->startElement("style:footer-left");
        break;
    case wvWare::HeaderData::FooterOdd:
        //set up buffer & writer for odd header
        m_buffer = new QBuffer();
        m_buffer->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_buffer);
        m_headerWriter->startElement("style:footer");
        m_hasFooter_list.replace(i, true);
        break;
    case wvWare::HeaderData::HeaderFirst:
        m_buffer = new QBuffer();
        m_buffer->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_buffer);
        m_firstOpen = true;
        m_headerWriter->startElement("style:header");
        m_hasHeader_list.replace(0, true);
        break;
    case wvWare::HeaderData::FooterFirst:
        m_buffer = new QBuffer();
        m_buffer->open(QIODevice::WriteOnly);
        m_headerWriter = new KoXmlWriter(m_buffer);
        m_firstOpen = true;
        m_headerWriter->startElement("style:footer");
        m_hasFooter_list.replace(0, true);
        break;
    }
    //tell other handlers we're writing a header
    m_writingHeader = true;
}

//creates empty frameset element?
void Document::headerEnd()
{
    kDebug(30513) ;
    //close a list if we need to (you can have a list inside a header)
    if (m_textHandler->listIsOpen()) {
        kDebug(30513) << "closing a list in a header/footer";
        m_textHandler->closeList();
    }
    // NOTE: We are assuming in the parser code that odd header/footer is
    // present by default if the m_headers variable is not empty.  It helps us
    // now to write the even header/footer content.

    // Close writer & add the header/footer content into the appropriate
    // master-page.

    // If it was an even header/footer, we wrote to this writer, but we won't
    // do anything with it.

    if (m_evenOpen) {
        m_headerWriter->endElement(); //style:header-left/footer-left
        m_evenOpen = false;
    }
    else {
        KoGenStyle* masterPageStyle = 0;
        QString name = 0;
        if (m_firstOpen) {
            name = m_masterPageName_list.first();
            masterPageStyle = m_masterPageStyle_list.first();
            m_firstOpen = false;
    }
    else {
            name = m_masterPageName_list.last();
            masterPageStyle = m_masterPageStyle_list.last();
    }
        Q_ASSERT(masterPageStyle);
        m_headerWriter->endElement(); //style:header/footer

        //add the even header/footer content here
        if (m_bufferEven) {
            m_headerWriter->addCompleteElement(m_bufferEven);
            delete m_bufferEven;
            m_bufferEven = 0;
        }
        QString contents = QString::fromUtf8(m_buffer->buffer(), m_buffer->buffer().size());
        masterPageStyle->addChildElement(QString::number(m_headerCount), contents);
        kDebug(30513) << "updating master-page style:" << name;

        delete m_buffer;
        m_buffer = 0;
    }

    delete m_headerWriter;
    m_headerWriter = 0;

    //we're done with this header, so reset to false
    m_writingHeader = false;
}

void Document::footnoteStart()
{
    kDebug(30513);
}

void Document::footnoteEnd()
{
    kDebug(30513);
}


void Document::annotationStart()
{
}

void Document::annotationEnd()
{
}

//NOTE: disable this for now - we should be able to do everything in
//TableHandler create frame for the table cell?
// void Document::slotTableCellStart( int row, int column, int rowSpan, int columnSpan, const QRectF& cellRect,
//                                    const QString& tableName,
//                                    const wvWare::Word97::BRC& brcTop, const wvWare::Word97::BRC& brcBottom,
//                                    const wvWare::Word97::BRC& brcLeft, const wvWare::Word97::BRC& brcRight,
//                                    const wvWare::Word97::SHD& shd )
// {
//     kDebug(30513) ;

//     //need to set up cell style here probably don't need generateFrameBorder()
//     //<table:table-cell> tag in content.xml

//     QDomElement framesetElement = m_mainDocument.createElement("FRAMESET");
//     framesetElement.setAttribute( "frameType", 1 /* text */ );
//     framesetElement.setAttribute( "frameInfo", 0 /* normal text */ );
//     framesetElement.setAttribute( "grpMgr", tableName );
//     QString name = i18nc("Table_Name Cell row,column", "%1 Cell %2,%3",tableName,row,column);
//     framesetElement.setAttribute( "name", name );
//     framesetElement.setAttribute( "row", row );
//     framesetElement.setAttribute( "col", column );
//     framesetElement.setAttribute( "rows", rowSpan );
//     framesetElement.setAttribute( "cols", columnSpan );
//     m_framesetsElement.appendChild(framesetElement);

//     QDomElement frameElem = createInitialFrame( framesetElement, cellRect.left(), cellRect.right(), cellRect.top(), cellRect.bottom(), true, NoFollowup );
//     generateFrameBorder( frameElem, brcTop, brcBottom, brcLeft, brcRight, shd );

//     m_textHandler->setFrameSetElement( framesetElement );
// }

//add empty element to end it?
// void Document::slotTableCellEnd()
// {
//     kDebug(30513) ;
//     //</table:table-cell>
//     m_textHandler->setFrameSetElement( QDomElement() );
// }

//set up frame borders (like for a table cell?)
//set the background fill
// void Document::generateFrameBorder( QDomElement& frameElementOut,
//                                     const wvWare::Word97::BRC& brcTop, const wvWare::Word97::BRC& brcBottom,
//                                     const wvWare::Word97::BRC& brcLeft, const wvWare::Word97::BRC& brcRight,
//                                     const wvWare::Word97::SHD& shd )
// {
//     kDebug(30513) ;
//     // Frame borders
//     //figure out what this is supposed to do!

//     if ( brcTop.ico != 255 && brcTop.dptLineWidth != 255 ) // see tablehandler.cpp
//     Conversion::setBorderAttributes( frameElementOut, brcTop, "t" );
//     if ( brcBottom.ico != 255 && brcBottom.dptLineWidth != 255 ) // see tablehandler.cpp
//     Conversion::setBorderAttributes( frameElementOut, brcBottom, "b" );
//     if ( brcLeft.ico != 255 && brcLeft.dptLineWidth != 255 ) // could still be 255, for first column
//     Conversion::setBorderAttributes( frameElementOut, brcLeft, "l" );
//     if ( brcRight.ico != 255 && brcRight.dptLineWidth != 255 ) // could still be 255, for last column
//     Conversion::setBorderAttributes( frameElementOut, brcRight, "r" );

//     // Frame background brush (color and fill style)
//     if ( shd.icoFore != 0 || shd.icoBack != 0 )
//     {
//         // If ipat = 0 (solid fill), icoBack is the background color.  But
//         // otherwise, icoFore is the one we need to set as bkColor (and icoBack
//         // is usually white; it's the other color of the pattern, something
//         // that we can't set in Qt apparently).
//     int bkColor = shd.ipat ? shd.icoFore : shd.icoBack;
//     kDebug(30513) <<"generateFrameBorder:" <<" icoFore=" << shd.icoFore <<" icoBack=" << shd.icoBack <<" ipat=" << shd.ipat <<" -> bkColor=" << bkColor;

//         // Reverse-engineer MSWord's own hackery: it models various gray levels
//         // using dithering. But this looks crappy with Qt. So we go back to a
//         // QColor.
//         bool grayHack = ( shd.ipat && shd.icoFore == 1 && shd.icoBack == 8 );
//         if ( grayHack )
//         {
//             bool ok;
//             int grayLevel = Conversion::ditheringToGray( shd.ipat, &ok );
//             if ( ok )
//             {
//                 QColor color( 0, 0, grayLevel, QColor::Hsv );
//                 QString prefix = "bk";
//                 frameElementOut.setAttribute( "bkRed", color.red() );
//                 frameElementOut.setAttribute( "bkBlue", color.blue() );
//                 frameElementOut.setAttribute( "bkGreen", color.green() );
//             }
//             else grayHack = false;
//         }
//         if ( !grayHack )
//         {
//             Conversion::setColorAttributes( frameElementOut, bkColor, "bk", true );
//             //Fill style
//             int brushStyle = Conversion::fillPatternStyle( shd.ipat );
//             frameElementOut.setAttribute( "bkStyle", brushStyle );
//         }
//     }
// }

//create SubDocument object & add it to the queue
void Document::slotSubDocFound(const wvWare::FunctorBase* functor, int data)
{
    kDebug(30513) ;
    SubDocument subdoc(functor, data, QString(), QString());
    m_subdocQueue.push(subdoc);
}

void Document::slotFootnoteFound(const wvWare::FunctorBase* functor, int data)
{
    kDebug(30513) ;
    SubDocument subdoc(functor, data, QString(), QString());
    (*subdoc.functorPtr)();
    delete subdoc.functorPtr;
}

void Document::slotAnnotationFound(const wvWare::FunctorBase* functor, int data)
{
    kDebug(30513) ;
    SubDocument subdoc(functor, data, QString(), QString());
    (*subdoc.functorPtr)();
    delete subdoc.functorPtr;
}

void Document::slotHeadersFound(const wvWare::FunctorBase* functor, int data)
{
    kDebug(30513) ;
    SubDocument subdoc(functor, data, QString(), QString());
    (*subdoc.functorPtr)();
    delete subdoc.functorPtr;
}

//add Words::Table object to the table queue
void Document::slotTableFound(Words::Table* table)
{
    kDebug(30513);

    m_tableHandler->tableStart(table);
    QList<Words::Row> &rows = table->rows;
    for (QList<Words::Row>::Iterator it = rows.begin(); it != rows.end(); ++it) {
        Words::TableRowFunctorPtr f = (*it).functorPtr;
        Q_ASSERT(f);
        (*f)(); // call it
        delete f; // delete it
    }
    m_tableHandler->tableEnd();

    //cleanup table
    delete table;
    table = 0;

    //m_tableQueue.push( table );
}

void Document::slotInlineObjectFound(const wvWare::PictureData& data, KoXmlWriter* writer)
{
    kDebug(30513) ;
    Q_UNUSED(writer);
    m_graphicsHandler->setCurrentWriter(m_textHandler->currentWriter());
    m_graphicsHandler->handleInlineObject(data);
    m_graphicsHandler->setCurrentWriter(m_textHandler->currentWriter());
}

void Document::slotFloatingObjectFound(unsigned int globalCP, KoXmlWriter* writer)
{
    kDebug(30513) ;
    Q_UNUSED(writer);
    m_graphicsHandler->setCurrentWriter(m_textHandler->currentWriter());
    m_graphicsHandler->handleFloatingObject(globalCP);
    m_graphicsHandler->setCurrentWriter(m_textHandler->currentWriter());
}

void Document::slotTextBoxFound(unsigned int index, bool stylesxml)
{
    kDebug(30513) ;
    m_parser->parseTextBox(index, stylesxml);
}

//process through all the subDocs and the tables
void Document::processSubDocQueue()
{
    kDebug(30513) ;
    // Table cells can contain footnotes, and footnotes can contain tables [without footnotes though]
    // This is why we need to repeat until there's nothing more do to (#79024)
    while (!m_subdocQueue.empty()) {// || !m_tableQueue.empty()) {
        while (!m_subdocQueue.empty()) {
            SubDocument subdoc(m_subdocQueue.front());
            Q_ASSERT(subdoc.functorPtr);
            (*subdoc.functorPtr)(); // call it
            delete subdoc.functorPtr; // delete it
            m_subdocQueue.pop();
        }
//         while ( !m_tableQueue.empty() )
//         {
//             Words::Table& table = m_tableQueue.front();
//             m_tableHandler->tableStart( &table );
//             QList<Words::Row> &rows = table.rows;
//             for( QList<Words::Row>::Iterator it = rows.begin(); it != rows.end(); ++it ) {
//                 Words::TableRowFunctorPtr f = (*it).functorPtr;
//                 Q_ASSERT( f );
//                 (*f)(); // call it
//                 delete f; // delete it
//             }
//             m_tableHandler->tableEnd();
//             m_tableQueue.pop();
//         }
    }
}

void Document::setPageLayoutStyle(KoGenStyle* pageLayoutStyle,
                                  wvWare::SharedPtr<const wvWare::Word97::SEP> sep,
                                  bool firstPage)
{
    // TODO: Check page-layout attributes specific for the first page.

    //get width & height in points
    double width = (double)sep->xaPage / 20.0;
    double height = (double)sep->yaPage / 20.0;
    pageLayoutStyle->addPropertyPt("fo:page-width", width);
    pageLayoutStyle->addPropertyPt("fo:page-height", height);
    pageLayoutStyle->addProperty("style:footnote-max-height", "0in");
    pageLayoutStyle->addProperty("style:writing-mode", "lr-tb");
    bool landscape = (sep->dmOrientPage == 2);
    pageLayoutStyle->addProperty("style:print-orientation", landscape ? "landscape" : "portrait");
    pageLayoutStyle->addProperty("style:num-format", "1");

    DrawStyle ds = m_graphicsHandler->getBgDrawStyle();
    if (ds.fFilled()) {
        switch (ds.fillType()) {
        case msofillSolid:
        {
            // PptToOdp::toQColor helper function can be used instead of this conversion
            MSO::OfficeArtCOLORREF clr = ds.fillColor();
            QColor color(clr.red, clr.green, clr.blue);
            QString tmp = color.name();
            pageLayoutStyle->addProperty("fo:background-color", tmp);

            //update the background-color information if required
            if (tmp != currentBgColor()) {
                updateBgColor(tmp);
            }
            break;
        }
        //TODO:
//         case msofillShade:
//         case msofillShadeCenter:
//         case msofillShadeShape:
//         case msofillShadeScale:
//         case msofillShadeTitle:
//
        //TODO:
//         case msofillPattern:
//         case msofillTexture:
//
        case msofillPicture:
        {
            // picture can be stored in OfficeArtBStoreContainer or in
            // fillBlip_complex if complex = true only picture in
            // OfficeArtBStoreContainer is handled now
            QString filePath = m_graphicsHandler->getPicturePath(ds.fillBlip());

            if (!filePath.isEmpty()) {
                QBuffer buffer;
                KoXmlWriter bkgImageWriter(&buffer);

                bkgImageWriter.startElement("style:background-image");
                bkgImageWriter.addAttribute("xlink:href", filePath);
                bkgImageWriter.addAttribute("xlink:type", "simple");
                bkgImageWriter.addAttribute("xlink:actuate", "onLoad");
                bkgImageWriter.endElement(); //style:background-image

                QString contents = QString::fromUtf8(((QBuffer*)bkgImageWriter.device())->buffer(),
                                         ((QBuffer*)bkgImageWriter.device())->buffer().size());

                pageLayoutStyle->addChildElement("0", contents);
            }
        break;
        }
        //TODO:
//         case msofillBackground:
        default:
            break;
        }
    }

    // NOTE: margin-top and margin-bottom are updated in slotSectionFound based
    // on the information if the header/footer was empty/non-empty.
    //
    // Maybe we shoud set the minimum height of header/footer to qAbs(dyaTop -
    // dyaHdrTop)/qAbs(dyaBottom - dyaHdrBottom)
    //
    // The height of both header and footer is unknown, so it's not possible to
    // set margin-bottom for the header and margin-top for the footer properly.
    // Both dyaTop and dyaBottom do not tell us where the header/footer ends.
    //
    // MSWord specific:
    // For each x in {header, footer, body}, x has an independent ruler.
    //
    // Also both header and footer are treated like a separate document, so if
    // you insert a border into the body, then header/footer margins do not
    // change.  We are not able to store this into ODF properly.

    QString header("<style:header-style>");
    header.append("<style:header-footer-properties");

    // The spec says the top-margin MUST be grown to avoid overlapping the
    // space that is occupied by headers.  Not ODF compatible.
    if (sep->dyaTop >= 0) {
        header.append(" style:dynamic-spacing=\"true\"");
        header.append(" fo:margin-bottom=\"0pt\"");
    } else {
        //TODO: tests required, I would prefer margin-bottom set to ZERO
        header.append(" style:dynamic-spacing=\"false\"");
        header.append(" fo:margin-bottom=\"");
#if 0
        qreal headerMarginTop = qAbs(sep->dyaTop) - sep->dyaHdrTop;
        if (headerMarginTop > 0) {
            header.append(QString::number(headerMarginTop / 20.0));
        } else
#endif
            header.append("0");
        header.append("pt\"");

    }
    header.append(" fo:min-height=\"14pt\"/>");
    header.append("</style:header-style>");

    QString footer("<style:footer-style>");
    footer.append("<style:header-footer-properties");

    // The spec says the bottom-margin MUST be grown to avoid overlapping the
    // space that is occupied by footers or footnotes.  Not ODF compatible.
    if (sep->dyaBottom > 0) {
        footer.append(" style:dynamic-spacing=\"true\"");
        footer.append(" fo:margin-top=\"0pt\"");
    } else {
        //TODO: tests required, I would prefer margin-top set to ZERO
        footer.append(" style:dynamic-spacing=\"false\"");
        footer.append(" fo:margin-top=\"");
        qreal headerMarginBottom = qAbs(sep->dyaBottom) - sep->dyaHdrBottom;
        if (headerMarginBottom >= 400) {
            footer.append(QString::number(headerMarginBottom / 20.0));
        } else {
            footer.append("14");
        }
        footer.append("pt\"");
    }
    footer.append(" fo:min-height=\"10pt\"/>");
    footer.append("</style:footer-style>");

    pageLayoutStyle->addProperty("1header-style", header, KoGenStyle::StyleChildElement);
    pageLayoutStyle->addProperty("2footer-style", footer, KoGenStyle::StyleChildElement);

    //Page borders, check to which page-layout the border information has to be
    //applied: 0 - all pages, 1 - first page only, 2 - all but first.

    //NOTE: 3 - whole document, not mentioned in the "Word Binary File Format
    //(.doc) Structure Specification", but mentiond in word97_generated.h
    if ( (sep->pgbApplyTo == 0) ||
        ((sep->pgbApplyTo == 1) && firstPage) ||
        ((sep->pgbApplyTo == 2) && !firstPage))
    {
        // FIXME: check if we can use fo:border instead of fo:border-left, etc.
        if (sep->brcLeft.brcType != 0) {
            pageLayoutStyle->addProperty("fo:border-left",
                                         Conversion::setBorderAttributes(sep->brcLeft));
            pageLayoutStyle->addProperty("calligra:specialborder-left",
                                         Conversion::borderCalligraAttributes(sep->brcLeft));
        }
        if (sep->brcTop.brcType != 0) {
            pageLayoutStyle->addProperty("fo:border-top",
                                         Conversion::setBorderAttributes(sep->brcTop));
            pageLayoutStyle->addProperty("calligra:specialborder-top",
                                         Conversion::borderCalligraAttributes(sep->brcTop));
        }
        if (sep->brcRight.brcType != 0) {
            pageLayoutStyle->addProperty("fo:border-right",
                                         Conversion::setBorderAttributes(sep->brcRight));
            pageLayoutStyle->addProperty("calligra:specialborder-right",
                                         Conversion::borderCalligraAttributes(sep->brcRight));
        }
        if (sep->brcBottom.brcType != 0) {
            pageLayoutStyle->addProperty("fo:border-bottom",
                                         Conversion::setBorderAttributes(sep->brcBottom));
            pageLayoutStyle->addProperty("calligra:specialborder-bottom",
                                         Conversion::borderCalligraAttributes(sep->brcBottom));
        }
    }
    // Set default left/right margins for the case when there is no border.
    // This will be changed below if there are borders defined.
    pageLayoutStyle->addPropertyPt("fo:margin-left", (double)sep->dxaLeft / 20.0);
    pageLayoutStyle->addPropertyPt("fo:margin-right", (double)sep->dxaRight / 20.0);

    // the pgbOffsetFrom variable determins how to calculate the margins and paddings.
    switch (sep->pgbOffsetFrom) {
    case pgbFromText:
        pageLayoutStyle->addPropertyPt("fo:padding-left",   sep->brcLeft.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:padding-top",    sep->brcTop.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:padding-right",  sep->brcRight.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:padding-bottom", sep->brcBottom.dptSpace);
        // FIXME: How should fo:margin be created in this case?
        break;
    case pgbFromEdge:
        // Add margin. This value is fetched directly from the BRC's.
        pageLayoutStyle->addPropertyPt("fo:margin-left",   sep->brcLeft.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:margin-top",    sep->brcTop.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:margin-right",  sep->brcRight.dptSpace);
        pageLayoutStyle->addPropertyPt("fo:margin-bottom", sep->brcBottom.dptSpace);

        // The *20 and /20 below is the conversion between twips (1/20th of a point) and points.
        pageLayoutStyle->addPropertyPt("fo:padding-left",
                                         (sep->dxaLeft - (sep->brcLeft.dptSpace * 20)) / 20);
        pageLayoutStyle->addPropertyPt("fo:padding-top",
                                         (qAbs(sep->dyaTop) - (sep->brcTop.dptSpace * 20)) / 20);
        pageLayoutStyle->addPropertyPt("fo:padding-right",
                                         (sep->dxaRight - (sep->brcRight.dptSpace * 20)) / 20);
        pageLayoutStyle->addPropertyPt("fo:padding-bottom",
                                         (qAbs(sep->dyaBottom) - (sep->brcBottom.dptSpace * 20)) / 20);
        break;
    }
    // TODO: use sep->fEndNote to set the 'use endnotes or footnotes' flag
}

bool Document::headersChanged(void) const
{
    int n = m_textHandler->sectionNumber() - 1;
    bool ret = false;

    if (n < m_headersMask.size()) {
        ret = m_headersMask[n];
    }
    return ret;
}

#include "document.moc"
