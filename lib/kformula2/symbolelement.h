/* This file is part of the KDE project
   Copyright (C) 2001 Andrea Rizzi <rizzi@kde.org>
	              Ulrich Kuettler <ulrich.kuettler@mailbox.tu-dresden.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __SYMBOLELEMENT_H
#define __SYMBOLELEMENT_H

#include "artwork.h"
#include "basicelement.h"
#include "elementindex.h"


/**
 * A symbol is simply a piece of art.
 */
class SymbolElement : public BasicElement {
public:

    enum { contentPos, upperPos, lowerPos };

    SymbolElement(Artwork::SymbolType type = Artwork::Empty, BasicElement* parent = 0);
    ~SymbolElement();

    /**
     * Sets the cursor and returns the element the point is in.
     * The handled flag shows whether the cursor has been set.
     * This is needed because only the innermost matching element
     * is allowed to set the cursor.
     */
    virtual BasicElement* goToPos(FormulaCursor*, bool& handled,
                                  const QPoint& point, const QPoint& parentOrigin);
    
    // drawing
    //
    // Drawing depends on a context which knows the required properties like
    // fonts, spaces and such.
    // It is essential to calculate elements size with the same context
    // before you draw.
    
    /**
     * Calculates our width and height and
     * our children's parentPosition.
     */
    virtual void calcSizes(ContextStyle& context, int parentSize);

    /**
     * Draws the whole element including its children.
     * The `parentOrigin' is the point this element's parent starts.
     * We can use our parentPosition to get our own origin then.
     */
    virtual void draw(QPainter& painter, ContextStyle& context,
                      int parentSize, const QPoint& parentOrigin);

    
    // navigation
    // 
    // The elements are responsible to handle cursor movement themselves.
    // To do this they need to know the direction the cursor moves and
    // the element it comes from.
    //
    // The cursor might be in normal or in selection mode.
    
    /**
     * Enters this element while moving to the left starting inside
     * the element `from'. Searches for a cursor position inside
     * this element or to the left of it.
     */
    virtual void moveLeft(FormulaCursor* cursor, BasicElement* from);

    /**
     * Enters this element while moving to the right starting inside
     * the element `from'. Searches for a cursor position inside
     * this element or to the right of it.
     */
    virtual void moveRight(FormulaCursor* cursor, BasicElement* from);

    /**
     * Enters this element while moving up starting inside
     * the element `from'. Searches for a cursor position inside
     * this element or above it.
     */
    virtual void moveUp(FormulaCursor* cursor, BasicElement* from);

    /**
     * Enters this element while moving down starting inside
     * the element `from'. Searches for a cursor position inside
     * this element or below it.
     */
    virtual void moveDown(FormulaCursor* cursor, BasicElement* from);

    // children

    /**
     * Removes the child. If this was the main child this element might
     * request its own removal.
     * The cursor is the one that caused the removal. It has to be moved
     * to the place any user expects the cursor after that particular
     * element has been removed.
     */
    //virtual void removeChild(FormulaCursor* cursor, BasicElement* child);

    
    // main child
    //
    // If an element has children one has to become the main one.
    
    virtual SequenceElement* getMainChild() { return content; }
    virtual void setMainChild(SequenceElement*);

    
    /**
     * Inserts all new children at the cursor position. Places the
     * cursor according to the direction.
     *
     * You only can insert one index at a time. So the list must contain
     * exactly on SequenceElement. And the index you want to insert
     * must not exist already.
     *
     * The list will be emptied but stays the property of the caller.
     */
    virtual void insert(FormulaCursor*, QList<BasicElement>&, Direction);

    /**
     * Removes all selected children and returns them. Places the
     * cursor to where the children have been.
     *
     * The cursor has to be inside one of our indexes which is supposed
     * to be empty. The index will be removed and the cursor will
     * be placed to the removed index so it can be inserted again.
     * This methode is called by SequenceElement::remove only.
     *
     * The ownership of the list is passed to the caller.
     */
    virtual void remove(FormulaCursor*, QList<BasicElement>&, Direction);

    /**
     * Moves the cursor to a normal place where new elements
     * might be inserted.
     */
    virtual void normalize(FormulaCursor*, Direction);
    
    /**
     * Returns the child at the cursor.
     */
    virtual BasicElement* getChild(FormulaCursor*, Direction = beforeCursor);

    /**
     * Sets the cursor to select the child. The mark is placed before,
     * the position behind it.
     */
    virtual void selectChild(FormulaCursor* cursor, BasicElement* child);
    
    bool hasUpper() const { return upper != 0; }
    bool hasLower() const { return lower != 0; }
    
    // If we want to create an index we need a cursor that points there.
    
    void setToUpper(FormulaCursor* cursor);
    void setToLower(FormulaCursor* cursor);

    // Moves the cursor inside the index. The index has to exist.
    void moveToUpper(FormulaCursor*, Direction);
    void moveToLower(FormulaCursor*, Direction);
    
    // Generic access to each index.
    
    ElementIndexPtr getUpperIndex() { return ElementIndexPtr(new UpperIndex(this)); }
    ElementIndexPtr getLowerIndex() { return ElementIndexPtr(new LowerIndex(this)); }

    // Save&load
    //virtual QDomElement getElementDom(QDomDocument *doc);
    //virtual bool buildFromDom(QDomElement *elem);

protected:

    //Save/load support
    
    /**
     * Returns the tag name of this element type.
     */
    virtual QString getTagName() const { return "SYMBOL"; }
    
    /**
     * Appends our attributes to the dom element.
     */
    virtual void writeDom(QDomElement& element);
    
    /**
     * Reads our attributes from the element.
     * Returns false if it failed.
     */
    virtual bool readAttributesFromDom(QDomElement& element);

    /**
     * Reads our content from the node. Sets the node to the next node
     * that needs to be read.
     * Returns false if it failed.
     */
    virtual bool readContentFromDom(QDomNode& node);
    
private:

    /**
     * An index that belongs to us.
     */
    class SymbolElementIndex : public ElementIndex {
    public:
        SymbolElementIndex(SymbolElement* p) : parent(p) {}
        virtual SymbolElement* getElement() { return parent; }
    protected:
        SymbolElement* parent;
    };

    // We have a (very simple) type for every index.
    
    class UpperIndex : public SymbolElementIndex {
    public:
        UpperIndex(SymbolElement* parent) : SymbolElementIndex(parent) {}
        virtual void moveToIndex(FormulaCursor* cursor, Direction direction)
            { parent->moveToUpper(cursor, direction); }
        virtual void setToIndex(FormulaCursor* cursor)
            { parent->setToUpper(cursor); }
        virtual bool hasIndex() const
            { return parent->hasUpper(); }
    };

    class LowerIndex : public SymbolElementIndex {
    public:
        LowerIndex(SymbolElement* parent) : SymbolElementIndex(parent) {}
        virtual void moveToIndex(FormulaCursor* cursor, Direction direction)
            { parent->moveToLower(cursor, direction); }
        virtual void setToIndex(FormulaCursor* cursor)
            { parent->setToLower(cursor); }
        virtual bool hasIndex() const
            { return parent->hasLower(); }
    };


    void setToContent(FormulaCursor* cursor);
    
    SequenceElement* content;
    SequenceElement* upper;
    SequenceElement* lower;

    /**
     * Our symbol.
     */
    Artwork symbol;
};


#endif // __SYMBOLELEMENT_H
