/*
    Copyright (C) 2000, S.R.Haque <shaheedhaque@hotmail.com>.
    This file is part of the KDE project
 
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

DESCRIPTION

    This file impements an abstraction of the on-disk format of Microsoft 
    Word documents. The public interface implements iterators that allow
    convenient traversal of the types of data in a document.

    We also add hand-generated structures and routines to complement all
    the machine-generated ones in our superclass.
*/

#ifndef MSWORD_H
#define MSWORD_H

#include <kdebug.h>
#include <mswordgenerated.h>

class MsWord: public MsWordGenerated
{
public:

    // Construction.

    MsWord(
        const U8 *mainStream,
        const U8 *table0Stream,
        const U8 *table1Stream,
        const U8 *dataStream);
    virtual ~MsWord();

    // Hand-generated structure definitions and reader prototypes. Reader
    // prototypes are also declared for inherited structure definitions
    // which are OK, but where the inherited reader must be overridden.

    static unsigned read(
        const U8 *in,
        QString *out,
        unsigned count = 1,
        bool unicode = false);

    typedef struct PAPXFKP
    {
        U16 istd;
        U16 grpprlBytes;
        U8 *grpprl;
    } PAPXFKP;
    static unsigned read(const U8 *in, PAPXFKP *out, unsigned count = 1);

    // STyle Definition (STD)
    typedef struct STD
    {

        // invariant style identifier
        U16 sti:12;

        // spare field for any temporary use, always reset back to zero!
        U16 fScratch:1;

        // PHEs of all text with this style are wrong
        U16 fInvalHeight:1;

        // UPEs have been generated
        U16 fHasUpe:1;

        // std has been mass-copied; if unused at save time, style should be 
        // deleted
        U16 fMassCopy:1;

        // style type code
        U16 sgc:4;

        // base style
        U16 istdBase:12;

        // # of UPXs (and UPEs)
        U16 cupx:4;

        // next style
        U16 istdNext:12;

        // offset to end of upx's, start of upe's
        U16 bchUpe;

        // auto redefine style when appropriate
        U16 fAutoRedef:1;

        // hidden from UI?
        U16 fHidden:1;

        // unused bits
        U16 unused8_3:14;

        // sub-names are separated by chDelimStyle
        QString xstzName;

        // 
        const U8 *grupx;
    } __attribute__ ((packed)) STD;
    static unsigned read(const U8 *in, STD *out, unsigned count=1);

    static unsigned read(const U8 *in, FIB *out, unsigned count=1);

protected:

    // Iterator for text by paragraph. The callback gotParagraph() returns
    // the text fetched. TBD: This is a crappy iterator design. 

    void getParagraphs();
    virtual void gotParagraph(const QString &text);

    // Iterator for the styles in the style sheet. The callback gotStyle()
    // returns the style fetched. TBD: This is a crappy iterator design.

    void getStyles();
    virtual void gotStyle(const QString &name);

    // Some fundamental data structures. We keep pointers to our streams,
    // and a copy of the FIB.

    const U8 *m_mainStream;
    const U8 *m_tableStream;
    const U8 *m_dataStream;
    FIB m_fib;

    // For the grpprl array, we store the offset to the
    // byte count preceeding the first entry, and the number of entries.

    struct
    {
    	U32 byteCountOffset;
        U32 count;
    } m_grpprls;

    // This class allows standardised treatment of plexes of different types.
    // It is designed to be instantiated locally, once for each plex we are
    // interested in.

    template <class T>
    class Plex
    {
    public:

        // We would like to define the constructor in terms of a callback to
        // take the data within the plex. Unfortunately, egcs-2.91-66 does not
        // support taking the address of bound pointer-to-member.
	//
        // Plex(bool (*callback)(unsigned start, unsigned end, const <T>& 

	Plex(MsWord *client, const U8 *plex, const U32 byteCount);

        // We would like to define the iterator as a proper friend class with a
        // constructor like this:
        //
        // template<class T>
        // PlexIterator<T>(Plex<T> plex);
	//
	// but that results in a compiler error.

        void startIteration();
        bool getNext(U32 *startFc, U32 *endFc, T *data);
    protected:
        MsWord *m_client;
        const U8 *m_plex;
        const U32 m_byteCount;
        unsigned m_crun;
        const U8 *m_fcNext;
        const U8 *m_dataNext;
        unsigned m_i;
    };

    // This class allows standardised treatment of FKPs of different types.
    // It is designed to be instantiated locally, once for each FKP we are
    // interested in.

    template <class T1, class T2>
    class Fkp
    {
    public:

        // We would like to define the constructor in terms of a callback to
        // take the data within the plex. Unfortunately, egcs-2.91-66 does not
        // support taking the address of bound pointer-to-member.
	//
        // Fkp(bool (*callback)(unsigned start, unsigned end, const <T>& 

	Fkp(MsWord *client, const U8 *fkp);

        // We would like to define the iterator as a proper friend class with a
        // constructor like this:
        //
        // template<class T>
        // FkpIterator<T>(Fkp<T> fkp);
	//
	// but that results in a compiler error.

        void startIteration();
        bool getNext(U32 *startFc, U32 *endFc, U8 *rgb, T1 *data1, T2 *data2);
    protected:
        MsWord *m_client;
        const U8 *m_fkp;
        U8 m_crun;
        const U8 *m_fcNext;
        const U8 *m_dataNext;
        U8 m_i;
    };

private:

    static const int area = 30003;
    Plex<PCD> *m_pcd;
    QString m_error;
    void error(unsigned line, const char *reason);
    void getPAPXFKP(const U8 *textStartFc, U32 textLength, bool unicode);
    void getPAPX(
        const U8 *fkp,
        const U8 *textStartFc,
        U32 textLength,
        bool unicode);
};
#endif


