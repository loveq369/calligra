/* This file is part of the KDE project

   (C) Copyright 2008 by Lorenzo Villani <lvillani@binaryhelix.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KEXI_WEBFORMS_READ_H
#define KEXI_WEBFORMS_READ_H

struct RequestData;

namespace KexiWebForms {
    /*! @short Callback function for Read handler */
    namespace Read {
        /*!
         * Simply show an HTML page containing a table displaying
         * data in a given database table.
         * This function uses the request URI to determine which
         * table to read: ie /view/books will show contents of the table
         * named 'books'
         *
         * @param RequestData a pointer to a RequestData structure
         * @see KexiWebForms::RequestData
         */
        void show(RequestData*);
    }
}

#endif /* KEXI_WEBFORMS_READ_H */
