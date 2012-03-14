/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "kis_texture_option.h"

#include <QWidget>
#include <QString>
#include <QByteArray>
#include <QCheckBox>
#include <QBuffer>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>

#include <klocale.h>

#include <kis_resource_server_provider.h>
#include <kis_pattern_chooser.h>
#include <kis_slider_spin_box.h>
#include <kis_multipliers_double_slider_spinbox.h>
#include <kis_pattern.h>

class KisTextureOptionWidget : public QWidget
{
public:

    KisTextureOptionWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
        QFormLayout *formLayout = new QFormLayout;
        chooser = new KisPatternChooser(this);
        chooser->setGrayscalePreview(true);
        chooser->setMaximumHeight(250);
        chooser->setCurrentItem(0, 0);
        formLayout->addRow(i18n("Pattern:"), chooser);

        scaleSlider = new KisMultipliersDoubleSliderSpinBox(this);
        scaleSlider->setRange(0.0, 2.0, 2);
        scaleSlider->setValue(1.0);
        scaleSlider->addMultiplier(0.1);
        scaleSlider->addMultiplier(2);
        scaleSlider->addMultiplier(10);

        formLayout->addRow(i18n("Scale:"), scaleSlider);

        rotationSlider = new KisDoubleSliderSpinBox(this);
        rotationSlider->setRange(0.0, 360.0, 2);
        rotationSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        formLayout->addRow(i18n("Rotation:"), rotationSlider);

        offsetSliderX = new KisSliderSpinBox(this);
        offsetSliderX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        formLayout->addRow(i18n("Horizontal Offset:"), offsetSliderX);

        offsetSliderY = new KisSliderSpinBox(this);
        offsetSliderY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        formLayout->addRow(i18n("Vertical Offset:"), offsetSliderY);

        strengthSlider = new KisDoubleSliderSpinBox(this);
        strengthSlider->setRange(0.0, 1.0);
        strengthSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        formLayout->addRow(i18n("Strength:"), strengthSlider);

        chkInvert = new QCheckBox("", this);
        chkInvert->setChecked(false);
        formLayout->addRow(i18n("Invert Texture:"), chkInvert);

        cmbChannel = new QComboBox(this);
        cmbChannel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QStringList channels;
        channels << i18n("Alpha") << i18n("Hue") << i18n("Saturation") << i18n("Lightness");
        cmbChannel->addItems(channels);
        formLayout->addRow(i18n("Channel:"), cmbChannel);

        setLayout(formLayout);
    }


    KisPatternChooser *chooser;
    KisMultipliersDoubleSliderSpinBox *scaleSlider;
    KisDoubleSliderSpinBox *rotationSlider;
    KisSliderSpinBox *offsetSliderX;
    KisSliderSpinBox *offsetSliderY;
    KisDoubleSliderSpinBox *strengthSlider;
    QCheckBox *chkInvert;
    QComboBox *cmbChannel;
};

KisTextureOption::KisTextureOption(QObject *)
    : KisPaintOpOption(i18n("Pattern"), KisPaintOpOption::textureCategory(), true)
{
    setChecked(false);
    m_optionWidget = new KisTextureOptionWidget;
    m_optionWidget->hide();
    setConfigurationPage(m_optionWidget);

    connect(m_optionWidget->chooser, SIGNAL(resourceSelected(KoResource*)), SLOT(resetGUI(KoResource*)));

    resetGUI(m_optionWidget->chooser->currentResource());
}

KisTextureOption::~KisTextureOption()
{
}


void KisTextureOption::writeOptionSetting(KisPropertiesConfiguration* setting) const
{

    KisPattern *pattern = static_cast<KisPattern*>(m_optionWidget->chooser->currentResource());
    qreal scale = m_optionWidget->scaleSlider->value();
    qreal rotation = m_optionWidget->rotationSlider->value();
    int offsetX = m_optionWidget->offsetSliderX->value();
    int offsetY = m_optionWidget->offsetSliderY->value();
    qreal strength = m_optionWidget->strengthSlider->value();
    bool invert = (m_optionWidget->chkInvert->checkState() == Qt::Checked);
    TextureChannel activeChannel = (TextureChannel)m_optionWidget->cmbChannel->currentIndex();

    if (!pattern) return;

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pattern->image().save(&buffer, "PNG");

    setting->setProperty("Texture/Pattern/Pattern", ba.toBase64());
    setting->setProperty("Texture/Pattern/PatternFileName", pattern->filename());
    setting->setProperty("Texture/Pattern/Name", pattern->name());
    setting->setProperty("Texture/Pattern/Scale", scale);
    setting->setProperty("Texture/Pattern/Rotation", rotation);
    setting->setProperty("Texture/Pattern/OffsetX", offsetX);
    setting->setProperty("Texture/Pattern/OffsetY", offsetY);
    setting->setProperty("Texture/Pattern/Strength", strength);
    setting->setProperty("Texture/Pattern/Invert", invert);
    setting->setProperty("Texture/Pattern/Channel", int(activeChannel));
}

void KisTextureOption::readOptionSetting(const KisPropertiesConfiguration* setting)
{
    QByteArray ba = QByteArray::fromBase64(setting->getString("Texture/Pattern/Pattern").toAscii());
    QImage img;
    img.loadFromData(ba, "PNG");
    QString name = setting->getString("Texture/Pattern/Name");
    if (name.isEmpty()) {
        name = setting->getString("Texture/Pattern/FileName");
    }
    KisPattern *pattern;
    if (!img.isNull()) {
        pattern = new KisPattern(img, name);
    }
    else {
        pattern = 0;
    }
    // now check whether the pattern already occurs, if not, add it to the
    // resources.
    if (pattern) {
        bool found = false;
        foreach(KoResource *res, KisResourceServerProvider::instance()->patternServer()->resources()) {
            KisPattern *pat = static_cast<KisPattern *>(res);
            if (pat == pattern) {
                delete pattern;
                pattern = pat;
                found = true;
                break;
            }
        }
        if (!found) {
            KisResourceServerProvider::instance()->patternServer()->addResource(pattern, false);
        }
        m_optionWidget->offsetSliderX->setRange(0, pattern->image().width() / 2);
        m_optionWidget->offsetSliderY->setRange(0, pattern->image().height() / 2);
    }
    else {
        pattern = static_cast<KisPattern*>(m_optionWidget->chooser->currentResource());
    }

    m_optionWidget->scaleSlider->setValue(setting->getDouble("Texture/Pattern/Scale", 1.0));
    m_optionWidget->rotationSlider->setValue(setting->getDouble("Texture/Pattern/Rotation"));
    m_optionWidget->offsetSliderX->setValue(setting->getInt("Texture/Pattern/OffsetX"));
    m_optionWidget->offsetSliderY->setValue(setting->getInt("Texture/Pattern/OffsetY"));
    m_optionWidget->strengthSlider->setValue(setting->getDouble("Texture/Pattern/Strength"));
    m_optionWidget->chkInvert->setChecked(setting->getBool("Texture/Pattern/Invert"));
    m_optionWidget->cmbChannel->setCurrentIndex(setting->getInt("Texture/Pattern/Channel"));

}

void KisTextureOption::resetGUI(KoResource* res)
{
    KisPattern *pattern = static_cast<KisPattern *>(res);
    m_optionWidget->scaleSlider->setValue(1.0);
    m_optionWidget->rotationSlider->setValue(0.0);
    m_optionWidget->offsetSliderX->setRange(0, pattern->image().width() / 2);
    m_optionWidget->offsetSliderX->setValue(0);
    m_optionWidget->offsetSliderY->setRange(0, pattern->image().height() / 2);
    m_optionWidget->offsetSliderY->setValue(0);
    m_optionWidget->strengthSlider->setValue(1.0);
    m_optionWidget->chkInvert->setChecked(false);
    m_optionWidget->cmbChannel->setCurrentIndex(0);
}
