/* -*- C++ -*-

  $Id$

  This file is part of KIllustrator.
  Copyright (C) 1998 Kai-Uwe Sattler (kus@iti.cs.uni-magdeburg.de)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <FloatSpinBox.h>
#include <qvalidator.h>

FloatSpinBox::FloatSpinBox (QWidget* parent, const char* name, int) :
    QSpinBox (parent, name) {

    minval = 1;
    maxval = 10;
    step = 1;
    value_ = 0.0;

    format = "%3.2f";
    setValue (1.0);
    setSteps (10, 10);
    val = new QDoubleValidator (minval, maxval, 2, this);
    setValidator (val);
}

FloatSpinBox::~FloatSpinBox () {
    delete val;
}

float FloatSpinBox::getValue () {
    return value_;
}

void FloatSpinBox::setFormatString (const char* fmt) {
    format = fmt;
}

void FloatSpinBox::setValue (float value) {
    if (minval <= value && value <= maxval) {
        value_ = value;
        QSpinBox::setValue((int)(value*100.0));
    }
}

void FloatSpinBox::setStep (float s) {
    step = s;
    setSteps (step * 10.0, step * 10.0);
}

float FloatSpinBox::getStep () const {
    return step;
}

void FloatSpinBox::setRange (float minVal, float maxVal) {
    if (minVal < maxVal) {
        minval = minVal;
        maxval = maxVal;
        QRangeControl::setRange (int (minVal * 100.0), int (maxVal * 100.0));
        val->setRange (minVal, maxVal, 2);
    }
}

void FloatSpinBox::getRange (float& minVal, float& maxVal) {
    minVal = minval; maxVal = maxval;
}

void FloatSpinBox::slotIncrease () {
    setValue ((float) getValue () + step);
}

void FloatSpinBox::slotDecrease () {
    setValue ((float) getValue () - step);
}

void FloatSpinBox::valueChange () {
    updateDisplay();
    emit valueChanged( (float) getValue() );
}

int FloatSpinBox::mapTextToValue (bool *ok) {
    value_ = text().toFloat(ok);
    return int (value_ * 100.0);
}

QString FloatSpinBox::mapValueToText (int v) {
    value_ = float (v) / 100.0;
    return QString().sprintf(format.latin1(), value_);
}

#include <FloatSpinBox.moc>
