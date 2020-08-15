/*************************************************************************
 *  Copyright (C) 2020 by Caio Jordão Carvalho <caiojcarvalho@gmail.com> *
 *                                                                       *
 *  This program is free software; you can redistribute it and/or        *
 *  modify it under the terms of the GNU General Public License as       *
 *  published by the Free Software Foundation; either version 3 of       *
 *  the License, or (at your option) any later version.                  *
 *                                                                       *
 *  This program is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 *************************************************************************/

#include "sentence.h"
#include "sentence_p.h"

#include <memory>

Sentence::Sentence(MarkedClass* objClass, quint64 begin, quint64 end) :
    MarkedObject(std::make_unique<SentencePrivate>(begin, end), objClass)
{
}

bool Sentence::isValid()
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    return dpointer->m_begin < dpointer->m_end;
}

bool Sentence::hasBetween(int number)
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    return number < dpointer->m_end && number > dpointer->m_begin;
}

void Sentence::clear()
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    dpointer->m_begin = 0;
    dpointer->m_end = 0;
}

void Sentence::append(double memberX, double memberY)
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    dpointer->m_begin = memberX;
    dpointer->m_end = memberY;
}

int Sentence::size() const
{
    return 1;
}

QString Sentence::unitName() const
{
    return "st";
}

QString Sentence::type() const
{
    return "Sentence";
}

QString Sentence::memberX() const
{
    return "begin";
}

QString Sentence::memberY() const
{
    return "end";
}

qreal Sentence::XValueOf(int element) const
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    return dpointer->m_begin;
}

qreal Sentence::YValueOf(int element) const
{
    auto dpointer = static_cast<SentencePrivate*>(d_p.get());
    return dpointer->m_end;
}

