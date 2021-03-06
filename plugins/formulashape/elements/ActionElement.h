/* This file is part of the KDE project
   Copyright (C) 2006-2007 Alfredo Beaumont Sainz <alfredo.beaumont@gmail.com>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef ACTIONELEMENT_H
#define ACTIONELEMENT_H

#include "RowElement.h"
#include "koformula_export.h"

/**
 * @short Implementation of the MathML maction element
 *
 * Support for action elements in MathML. According to MathML spec 
 * (Section 3.6.1.1), a MathML conformant application is not required to 
 * recognize any single actiontype.
 */
class KOFORMULA_EXPORT ActionElement : public RowElement {
public:
    /// The standard constructor
    explicit ActionElement(BasicElement *parent = 0);

    /// @return The element's ElementType
    ElementType elementType() const;
};

#endif // ACTIONELEMENT_H
