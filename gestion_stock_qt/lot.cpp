#include "lot.h"
#include <QMap>

namespace WF {
int maxStock(const QString &product) {
    static const QMap<QString, int> m{{"Eau plate 0,5 L", 22500},
                                      {"Eau plate 1,5 L", 15000},
                                      {"Eau gazeuse 0,5 L", 13000},
                                      {"Bonbonne 19 L", 3300}};
    return m.value(product, 10000);
}
} // namespace WF

QJsonObject Lot::toJson() const {
    QJsonObject o;
    o["id"] = id;
    o["dateTime"] = dateTime.toString(Qt::ISODate);
    o["line"] = line;
    o["product"] = product;
    o["produced"] = produced;
    o["stock"] = stock;
    o["threshold"] = threshold;
    o["operator"] = operatorName;
    o["status"] = status;
    return o;
}

Lot Lot::fromJson(const QJsonObject &o) {
    Lot l;
    l.id = o["id"].toInt();
    l.dateTime = QDateTime::fromString(o["dateTime"].toString(), Qt::ISODate);
    l.line = o["line"].toString();
    l.product = o["product"].toString();
    l.produced = o["produced"].toInt();
    l.stock = o["stock"].toInt();
    l.threshold = o["threshold"].toInt();
    l.operatorName = o["operator"].toString();
    l.status = o["status"].toString();
    return l;
}
