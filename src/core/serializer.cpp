/*************************************************************************
 *  Copyright (C) 2020 by Jean Lima Andrade <jeno.andrade@gmail.com>     *
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

#include "core/markedclass.h"
#include "core/serializer.h"
#include "image/polygon.h"
#include "text/sentence.h"

#include <memory>

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtGlobal>
#include <QXmlStreamWriter>

QVector<MarkedObject*> Serializer::read(const QString& filename)
{
    QVector<MarkedObject*> objects;

    if (QFile::exists(filename)) {
        if (filename.endsWith(".xml"))
            objects = readXML(filename);
        else if (filename.endsWith(".json"))
            objects = readJSON(filename);
    }

    return objects;
}

QString Serializer::serialize(const QVector<MarkedObject*>& objects, OutputType output_type)
{
    if (output_type == OutputType::XML)
        return toXML(objects);

    else if (output_type == OutputType::JSON)
        return toJSON(objects);

    return QString();
}

QString Serializer::toXML(const QVector<MarkedObject*>& objects)
{
    if (objects.isEmpty())
        return QString();

    QString xmldoc;

    QXmlStreamWriter xmlWriter(&xmldoc);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("annotation");

    auto writeXMLObject = [&](const QString& unitName, int x, int y) {
        xmlWriter.writeStartElement(unitName);

        xmlWriter.writeStartElement("x");
        xmlWriter.writeCharacters(QString::number(x));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("y");
        xmlWriter.writeCharacters(QString::number(y));
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
    };

    for (MarkedObject* object : objects) {
        xmlWriter.writeStartElement("object");

        xmlWriter.writeStartElement("class");
        xmlWriter.writeCharacters(object->objClass()->name());
        xmlWriter.writeEndElement();

        if (object->type() == MarkedObject::Type::Sentence) {
            const Sentence *sentence = static_cast<const Sentence*>(object);
            writeXMLObject(sentence->unitName(), sentence->begin(), sentence->end());
        }
        else if (object->type() == MarkedObject::Type::Polygon) {
            const Polygon *polygon = static_cast<const Polygon*>(object);
            xmlWriter.writeStartElement("Polygon");
            for (const QPointF& point : *polygon)
                writeXMLObject(polygon->unitName(), point.x(), point.y());
            xmlWriter.writeEndElement();
        }

        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    return xmldoc;
}

QString Serializer::toJSON(const QVector<MarkedObject*>& objects)
{
    if (objects.isEmpty())
        return QString();

    QJsonArray classesArray;

    auto createUnitObject = [](int x, int y) {
        QJsonObject unitObject;
        unitObject.insert("x", QString::number(x));
        unitObject.insert("y", QString::number(y));
        return unitObject;
    };

    for (MarkedObject* object : objects) {
        QJsonObject recordObject;
        recordObject.insert("Class", object->objClass()->name());

        if (object->type() == MarkedObject::Type::Sentence) {
            const Sentence *sentence = static_cast<const Sentence*>(object);
            QJsonObject unitObject = createUnitObject(sentence->begin(), sentence->end());
            recordObject.insert(object->unitName(), unitObject);
        }
        else if (object->type() == MarkedObject::Type::Polygon) {
            QJsonArray objectsArray;

            const Polygon *polygon = static_cast<const Polygon*>(object);
            for (const QPointF& point : *polygon) {
                QJsonObject unitObject = createUnitObject(point.x(), point.y());
                QJsonObject markedObject;
                markedObject.insert(object->unitName(), unitObject);
                objectsArray.push_back(markedObject);
            }

            recordObject.insert("Polygon", objectsArray);
        }

        classesArray.push_back(recordObject);
    }

    return QJsonDocument(classesArray).toJson();
}

QVector<MarkedObject*> Serializer::readJSON(const QString& filename)
{
    QVector<MarkedObject*> savedObjects;
    QHash<QString, MarkedClass*> markedClasses;

    QByteArray data = getData(filename);
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonArray objectArray = doc.array();

    for (const QJsonValue& jsonObj : qAsConst(objectArray)) {
        QString className = jsonObj["Class"].toString();

        MarkedObject::Type objectType;
        if (jsonObj["st"] != QJsonValue::Undefined)
            objectType = MarkedObject::Type::Sentence;
        else if (jsonObj["Polygon"] != QJsonValue::Undefined)
            objectType = MarkedObject::Type::Polygon;

        if (!markedClasses.contains(className))
            markedClasses[className] = new MarkedClass(className);

        MarkedObject* object = nullptr;
        if (objectType == MarkedObject::Type::Sentence) {
            QJsonObject sentenceObj = jsonObj["st"].toObject();
            object = new Sentence(markedClasses[className], sentenceObj["x"].toDouble(), sentenceObj["y"].toDouble());
        }
        else if (objectType == MarkedObject::Type::Polygon) {
            QVector<QPointF> polygonPoints;
            for (const QJsonValue& unitObj : jsonObj["Polygon"].toArray()) {
                QJsonObject pointJson = unitObj["pt"].toObject();
                QPointF pointObj(pointJson["x"].toDouble(), pointJson["y"].toDouble());
                polygonPoints << pointObj;
            }
            object = new Polygon(markedClasses[className], polygonPoints);
        }

        if (object != nullptr)
            savedObjects.append(object);
    }

    return savedObjects;
}

QVector<MarkedObject*> Serializer::readXML(const QString& filename)
{
    QVector<MarkedObject*> savedObjects;
    QHash<QString, MarkedClass*> markedClasses;

    QByteArray data = getData(filename);

    QXmlStreamReader xmlReader(data);
    xmlReader.readNextStartElement();

    auto readNextXY = [&]() {
        xmlReader.readNextStartElement();
        double x = xmlReader.readElementText().toDouble();
        xmlReader.readNextStartElement();
        double y = xmlReader.readElementText().toDouble();
        xmlReader.skipCurrentElement();

        return QPair<int, int>(x, y);
    };

    while (!xmlReader.atEnd()) {
        QXmlStreamReader::TokenType token = xmlReader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            xmlReader.readNextStartElement();

            QString className = xmlReader.readElementText();

            if (!markedClasses.contains(className))
                markedClasses[className] = new MarkedClass(className);

            xmlReader.readNextStartElement();

            MarkedObject* object;
            if (xmlReader.name() == "Polygon") {
                QVector<QPointF> polygonPoints;

                while (xmlReader.name() != "object") {
                    if (xmlReader.name() == "pt") {
                        QPair<int, int> pointXY = readNextXY();
                        polygonPoints << QPointF(pointXY.first, pointXY.second);
                    }
                    xmlReader.readNextStartElement();
                }

                object = new Polygon(markedClasses[className], polygonPoints);
            }
            else if (xmlReader.name() == "st") {
                QPair<int, int> sentenceBeginEnd = readNextXY();
                object = new Sentence(markedClasses[className], sentenceBeginEnd.first, sentenceBeginEnd.second);
            }

            savedObjects << object;
        }
    }

    return savedObjects;
}

QByteArray Serializer::getData(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly|QIODevice::Text);
    QByteArray data = file.readAll();
    file.close();

    return data;
}

bool Serializer::write(const QString &filepath, const QVector<MarkedObject*>& objects, OutputType output_type)
{
    if (!objects.isEmpty()) {
        QString filename = QString(filepath);
        // this part needs improvement
        filename.replace(QRegularExpression(".jpg|.jpeg|.png|.xpm|.txt"), (output_type == OutputType::XML ? ".xml" : ".json"));

        QString document = serialize(objects, output_type);

        if (!document.isEmpty()) {
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly|QIODevice::Text)) {
                file.write(document.toUtf8());
                file.close();
                return true;
            }
        }
    }

    return false;
}

const char* Serializer::filterString(OutputType output_type)
{
    if (output_type == OutputType::XML)
        return "XML files (*.xml *.XML)";
    else if (output_type == OutputType::JSON)
        return "JSON files (*.json *.JSON)";
    return "";
}
