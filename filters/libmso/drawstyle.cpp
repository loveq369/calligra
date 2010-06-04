/* This file is part of the KDE project
   Copyright (C) 2010 KO GmbH <jos.van.den.oever@kogmbh.com>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#include "drawstyle.h"

namespace {
const MSO::OfficeArtCOLORREF white() {
    MSO::OfficeArtCOLORREF w;
    w.red = w.green = w.blue = 0xFF;
    w.fPaletteIndex = w.fPaletteRGB = w.fSystemRGB = w.fSchemeIndex
            = w.fSysIndex = false;
    return w;
}
const MSO::OfficeArtCOLORREF black() {
    MSO::OfficeArtCOLORREF b;
    b.red = b.green = b.blue = 0;
    b.fPaletteIndex = b.fPaletteRGB = b.fSystemRGB = b.fSchemeIndex
            = b.fSysIndex = false;
    return b;
}
const MSO::FixedPoint one() {
    MSO::FixedPoint one;
    one.integral = 1;
    one.fractional = 0;
    return one;
}
}

#define GETTER(TYPE, FOPT, NAME, DEFAULT) \
TYPE DrawStyle::NAME() const \
{ \
    const MSO::FOPT* p = 0; \
    if (sp) { \
        p = get<MSO::FOPT>(*sp); \
    } \
    if (!p && mastersp) { \
        p = get<MSO::FOPT>(*mastersp); \
    } \
    if (!p) { \
        p = get<MSO::FOPT>(d); \
    } \
    if (p) { \
        return p->NAME; \
    } \
    return DEFAULT; \
}

//     TYPE                    FOPT                  NAME                  DEFAULT  ODRAW Ref
GETTER(quint32,                FillType,             fillType,             0)       // 2.3.7.1
GETTER(MSO::OfficeArtCOLORREF, FillColor,            fillColor,            white()) // 2.3.7.2
GETTER(qint32,                 FillOpacity,          fillOpacity,          0x10000) // 2.3.7.3
GETTER(quint32,                FillBlip,             fillBlip,             0)       // 2.3.7.7
GETTER(qint32,                 FillDztype,           fillDztype,           0)       // 2.3.7.24
GETTER(quint32,                LineEndArrowhead,     lineEndArrowhead,     0)
GETTER(quint32,                LineStartArrowhead,   lineStartArrowhead,   0)
GETTER(quint32,                LineStartArrowWidth,  lineStartArrowWidth,  1)
GETTER(quint32,                LineEndArrowWidth,    lineEndArrowWidth,    1)
GETTER(quint32,                LineWidth,            lineWidth,            0x2535)
GETTER(qint32,                 ShadowOffsetX,        shadowOffsetX,        0x6338)
GETTER(qint32,                 ShadowOffsetY,        shadowOffsetY,        0x6338)
GETTER(MSO::FixedPoint,        ShadowOpacity,        shadowOpacity,        one())
GETTER(quint32,                LineDashing,          lineDashing,          0)
GETTER(MSO::OfficeArtCOLORREF, LineColor,            lineColor,            black())
GETTER(qint32,                 LineOpacity,          lineOpacity,          0x10000)
GETTER(qint32,                 TxflTextFlow,         txflTextFlow,         0)
GETTER(qint32,                 PosH,                 posH,                 0)
GETTER(qint32,                 PosRelH,              posRelH,              2)
GETTER(qint32,                 PosV,                 posV,                 0)
GETTER(qint32,                 PosRelV,              posRelV,              2)
GETTER(quint32,                PctHR,                pctHR,                0x000003e8)
GETTER(quint32,                AlignHR,              alignHR,              0)
GETTER(qint32,                 DxHeightHR,           dxHeightHR,           0)
GETTER(qint32,                 DxWidthHR,            dxWidthHR,            0)
GETTER(quint32,                PWrapPolygonVertices, pWrapPolygonVertices, 0)
GETTER(qint32,                 DxWrapDistLeft,       dxWrapDistLeft,       0x0001be7c)
GETTER(qint32,                 DyWrapDistTop,        dyWrapDistTop,        0)
GETTER(qint32,                 DxWrapDistRight,      dxWrapDistRight,      0x0001be7c)
GETTER(qint32,                 DyWrapDistBottom,     dyWrapDistBottom,     0)
GETTER(qint32,                 DxTextLeft,           dxTextLeft,           0)
GETTER(qint32,                 DyTextTop,            dyTextTop,            0)
GETTER(qint32,                 DxTextRight,          dxTextRight,          0)
GETTER(qint32,                 DyTextBottom,         dyTextBottom,         0)
GETTER(quint32,                Pib,                  pib,                  0)
#undef GETTER

#define GETTER(NAME, TEST, DEFAULT) \
bool DrawStyle::NAME() const \
{ \
    const MSO::FOPT* p = 0; \
    if (sp) { \
        p = get<MSO::FOPT>(*sp); \
        if (p && p->TEST) { \
            return p->NAME; \
        } \
    } \
    if (mastersp) { \
        p = get<MSO::FOPT>(*mastersp); \
        if (p && p->TEST) { \
            return p->NAME; \
        } \
    } \
    p = get<MSO::FOPT>(d); \
    if (p && p->TEST) { \
        return p->NAME; \
    } \
    return DEFAULT; \
}
// FOPT        NAME           TEST                       DEFAULT
#define FOPT FillStyleBooleanProperties
GETTER(fNoFillHitTest,        fUseNoFillHitTest,         false)
GETTER(fillUseRect,           fUseFillUseRext,           false)
GETTER(fillShape,             fUseFillShape,             true)
GETTER(fHitTestFill,          fUseHitTestFill,           true)
GETTER(fFilled,               fUseFilled,                true)
GETTER(fUseShapeAnchor,       fUseUseShapeAnchor,        false)
GETTER(fRecolorFillAsPicture, fUsefRecolorFillAsPicture, false)
#undef FOPT
#define FOPT LineStyleBooleanProperties
GETTER(fNoLineDrawDash,       fUseNoLineDrawDash,        false)
GETTER(fLineFillShape,        fUseLineFillShape,         false)
GETTER(fHitTestLine,          fUseHitTestLine,           true)
GETTER(fLine,                 fUsefLine,                 true)
GETTER(fArrowHeadsOK,         fUsefArrowHeadsOK,         false)
GETTER(fInsetPenOK,           fUseInsetPenOK,            true)
GETTER(fInsetPen,             fUseInsetPen,              false)
GETTER(fLineOpaqueBackColor,  fUsefLineOpaqueBackColor,  false)
#undef FOPT
#define FOPT GroupShapeBooleanProperties
GETTER(fPrint,                fUsefPrint,                true)
GETTER(fHidden,               fUsefHidden,               false)
GETTER(fOneD,                 fUsefOneD,                 false)
GETTER(fIsButton,             fUsefIsButton,             false)
GETTER(fOnDblClickNotify,     fUsefOnDblClickNotify,     false)
GETTER(fBehindDocument,       fUsefBehindDocument,       false)
GETTER(fEditedWrap,           fUsefEditedWrap,           false)
GETTER(fScriptAnchor,         fUsefScriptAnchor,         false)
GETTER(fReallyHidden,         fUsefReallyHidden,         false)
GETTER(fAllowOverlap,         fUsefAllowOverlap,         true)
GETTER(fUserDrawn,            fUsefUserDrawn,            false)
GETTER(fHorizRule,            fUsefHorizRule,            false)
GETTER(fNoshadeHR,            fUsefNoshadeHR,            false)
GETTER(fStandardHR,           fUsefStandardHR,           false)
GETTER(fIsBullet,             fUsefIsBullet,             false)
GETTER(fLayoutInCell,         fUsefLayoutInCell,         true)
#undef FOPT
