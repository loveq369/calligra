/* This file is part of the KDE project
   Copyright (C) 1999 Werner Trobin <trobin@kde.org>

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
*/

#include <powerpointfilter.h>

const QDomDocument * const PowerPointFilter::part() {

    m_part=QDomDocument("DOC");
    m_part.setContent(QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                              "<DOC author=\"Reginald Stadlbauer\" email=\"reggie@kde.org\" editor=\"KPresenter\" mime=\"application/x-kpresenter\" syntaxVersion=\"2\">"
                              "<PAPER format=\"5\" ptWidth=\"680\" ptHeight=\"510\" mmWidth =\"240\" mmHeight=\"180\" inchWidth =\"9.44882\" inchHeight=\"7.08661\" orientation=\"0\" unit=\"0\">"
                              "<PAPERBORDERS mmLeft=\"0\" mmTop=\"0\" mmRight=\"0\" mmBottom=\"0\" ptLeft=\"0\" ptTop=\"0\" ptRight=\"0\" ptBottom=\"0\" inchLeft=\"0\" inchTop=\"0\" inchRight=\"0\" inchBottom=\"0\"/>"
                              "</PAPER>"
                              "<BACKGROUND rastX=\"10\" rastY=\"10\" bred=\"255\" bgreen=\"255\" bblue=\"255\">"
                              "<PAGE>"
                              "<BACKTYPE value=\"0\"/>"
                              "<BACKVIEW value=\"1\"/>"
                              "<BACKCOLOR1 red=\"255\" green=\"255\" blue=\"255\"/>"
                              "<BACKCOLOR2 red=\"255\" green=\"255\" blue=\"255\"/>"
                              "<BCTYPE value=\"0\"/>"
                              "<BGRADIENT unbalanced=\"0\" xfactor=\"100\" yfactor=\"100\"/>"
                              "<PGEFFECT value=\"0\"/>"
                              "</PAGE>"
                              "</BACKGROUND>"
                              "<HEADER show=\"0\">"
                              "<ORIG x=\"0\" y=\"0\"/>"
                              "<SIZE width=\"-1\" height=\"-1\"/>"
                              "<SHADOW distance=\"0\" direction=\"5\" red=\"160\" green=\"160\" blue=\"164\"/>"
                              "<EFFECTS effect=\"0\" effect2=\"0\"/>"
                              "<PRESNUM value=\"0\"/>"
                              "<ANGLE value=\"0\"/>"
                              "<FILLTYPE value=\"0\"/>"
                              "<GRADIENT red1=\"255\" green1=\"0\" blue1=\"0\" red2=\"0\" green2=\"255\" blue2=\"0\" type=\"1\" unbalanced=\"0\" xfactor=\"100\" yfactor=\"100\"/>"
                              "<PEN red=\"0\" green=\"0\" blue=\"0\" width=\"1\" style=\"0\"/>"
                              "<BRUSH red=\"0\" green=\"0\" blue=\"0\" style=\"0\"/>"
                              "<DISAPPEAR effect=\"0\" doit=\"0\" num=\"1\"/>"
                              "<TEXTOBJ lineSpacing=\"0\" paragSpacing=\"0\" margin=\"0\" bulletType1=\"0\" bulletType2=\"1\" bulletType3=\"2\" bulletType4=\"3\" bulletColor1=\"#000000\" bulletColor2=\"#000000\" bulletColor3=\"#000000\" bulletColor4=\"#000000\">"
                              "<P align=\"1\" type=\"0\" depth=\"0\">"
                              "<TEXT family=\"utopia\" pointSize=\"20\" bold=\"0\" italic=\"0\" underline=\"0\" color=\"#000000\"> </TEXT>"
                              "</P>"
                              "</TEXTOBJ>"
                              "</HEADER>"
                              "<FOOTER show=\"0\">"
                              "<ORIG x=\"0\" y=\"0\"/>"
                              "<SIZE width=\"-1\" height=\"-1\"/>"
                              "<SHADOW distance=\"0\" direction=\"5\" red=\"160\" green=\"160\" blue=\"164\"/>"
                              "<EFFECTS effect=\"0\" effect2=\"0\"/>"
                              "<PRESNUM value=\"0\"/>"
                              "<ANGLE value=\"0\"/>"
                              "<FILLTYPE value=\"0\"/>"
                              "<GRADIENT red1=\"255\" green1=\"0\" blue1=\"0\" red2=\"0\" green2=\"255\" blue2=\"0\" type=\"1\" unbalanced=\"0\" xfactor=\"100\" yfactor=\"100\"/>"
                              "<PEN red=\"0\" green=\"0\" blue=\"0\" width=\"1\" style=\"0\"/>"
                              "<BRUSH red=\"0\" green=\"0\" blue=\"0\" style=\"0\"/>"
                              "<DISAPPEAR effect=\"0\" doit=\"0\" num=\"1\"/>"
                              "<TEXTOBJ lineSpacing=\"0\" paragSpacing=\"0\" margin=\"0\" bulletType1=\"0\" bulletType2=\"1\" bulletType3=\"2\" bulletType4=\"3\" bulletColor1=\"#000000\" bulletColor2=\"#000000\" bulletColor3=\"#000000\" bulletColor4=\"#000000\">"
                              "<P align=\"1\" type=\"0\" depth=\"0\">"
                              "<TEXT family=\"utopia\" pointSize=\"20\" bold=\"0\" italic=\"0\" underline=\"0\" color=\"#000000\"> </TEXT>"
                              "</P>"
                              "</TEXTOBJ>"
                              "</FOOTER>"
                              "<OBJECTS>"
                              "<OBJECT type=\"4\">"
                              "<ORIG x=\"30\" y=\"30\"/>"
                              "<SIZE width=\"610\" height=\"43\"/>"
                              "<SHADOW distance=\"0\" direction=\"5\" red=\"160\" green=\"160\" blue=\"164\"/>"
                              "<EFFECTS effect=\"0\" effect2=\"0\"/>"
                              "<PRESNUM value=\"0\"/>"
                              "<ANGLE value=\"0\"/>"
                              "<FILLTYPE value=\"0\"/>"
                              "<GRADIENT red1=\"255\" green1=\"0\" blue1=\"0\" red2=\"0\" green2=\"255\" blue2=\"0\" type=\"1\" unbalanced=\"0\" xfactor=\"100\" yfactor=\"100\"/>"
                              "<PEN red=\"0\" green=\"0\" blue=\"0\" width=\"1\" style=\"0\"/>"
                              "<BRUSH red=\"0\" green=\"0\" blue=\"0\" style=\"0\"/>"
                              "<DISAPPEAR effect=\"0\" doit=\"0\" num=\"1\"/>"
                              "<TEXTOBJ lineSpacing=\"0\" paragSpacing=\"0\" margin=\"0\" bulletType1=\"0\" bulletType2=\"0\" bulletType3=\"0\" bulletType4=\"0\" bulletColor1=\"#ff0000\" bulletColor2=\"#ff0000\" bulletColor3=\"#ff0000\" bulletColor4=\"#ff0000\">"
                              "<P align=\"4\" type=\"0\" depth=\"0\">"
                              "<TEXT family=\"utopia\" pointSize=\"36\" bold=\"0\" italic=\"0\" underline=\"0\" color=\"#000000\">Sorry</TEXT><TEXT family=\"utopia\" pointSize=\"36\" bold=\"1\" italic=\"0\" underline=\"0\" color=\"#000000\"> </TEXT>"
                              "</P>"
                              "</TEXTOBJ>"
                              "</OBJECT>"
                              "<OBJECT type=\"4\">"
                              "<ORIG x=\"30\" y=\"130\"/>"
                              "<SIZE width=\"610\" height=\"24\"/>"
                              "<SHADOW distance=\"0\" direction=\"5\" red=\"160\" green=\"160\" blue=\"164\"/>"
                              "<EFFECTS effect=\"0\" effect2=\"0\"/>"
                              "<PRESNUM value=\"0\"/>"
                              "<ANGLE value=\"0\"/>"
                              "<FILLTYPE value=\"0\"/>"
                              "<GRADIENT red1=\"255\" green1=\"0\" blue1=\"0\" red2=\"0\" green2=\"255\" blue2=\"0\" type=\"1\" unbalanced=\"0\" xfactor=\"100\" yfactor=\"100\"/>"
                              "<PEN red=\"0\" green=\"0\" blue=\"0\" width=\"1\" style=\"0\"/>"
                              "<BRUSH red=\"0\" green=\"0\" blue=\"0\" style=\"0\"/>"
                              "<DISAPPEAR effect=\"0\" doit=\"0\" num=\"1\"/>"
                              "<TEXTOBJ lineSpacing=\"0\" paragSpacing=\"0\" margin=\"0\" bulletType1=\"0\" bulletType2=\"0\" bulletType3=\"0\" bulletType4=\"0\" bulletColor1=\"#000000\" bulletColor2=\"#ff0000\" bulletColor3=\"#ff0000\" bulletColor4=\"#ff0000\">"
                              "<P align=\"0\" type=\"1\" depth=\"0\">"
                              "<TEXT family=\"utopia\" pointSize=\"20\" bold=\"0\" italic=\"0\" underline=\"0\" color=\"#000000\">We are terribly sorry, but the ppt filter is not implemented yet</TEXT><TEXT family=\"utopia\" pointSize=\"20\" bold=\"1\" italic=\"0\" underline=\"0\" color=\"#000000\"> </TEXT>"
                              "</P>"
                              "</TEXTOBJ>"
                              "</OBJECT>"
                              "</OBJECTS>"
                              "<INFINITLOOP value=\"0\"/>"
                              "<MANUALSWITCH value=\"1\"/>"
                              "<PRESSPEED value=\"1\"/>"
                              "<PRESSLIDES value=\"0\"/>"
                              "<SELSLIDES>"
                              "<SLIDE nr=\"0\" show=\"1\"/>"
                              "</SELSLIDES>"
                              "<PIXMAPS>"
                              "</PIXMAPS>"
                              "<CLIPARTS>"
                              "</CLIPARTS>"
                              "</DOC>"));
    return &m_part;
}

#include <powerpointfilter.moc>
