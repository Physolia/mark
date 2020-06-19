#ifndef MARKEDOBJECT_H
#define MARKEDOBJECT_H

#include <memory>
#include <QVariant>
#include <QString>

#include "ui/markedclass.h"

class MarkedClass;
class MarkedObjectPrivate;

class MarkedObject
{
public:
    explicit MarkedObject(std::unique_ptr<MarkedObjectPrivate> d_ptr, MarkedClass* objClass);

    MarkedClass* objClass() const;
    void setObjClass(MarkedClass* objClass);
    QString className() const;

    virtual void clear() = 0;
    virtual void append(QVariant obj) = 0;
    virtual int size() const = 0;
    virtual QString unitName() const = 0;
    virtual QString type() const = 0;
    virtual QString memberX() const = 0;
    virtual QString memberY() const = 0;
    virtual qreal XValueOf(int element) const = 0;
    virtual qreal YValueOf(int element) const = 0;

protected:
    std::unique_ptr<MarkedObjectPrivate> d_p;
};

#endif // MARKEDOBJECT_H
