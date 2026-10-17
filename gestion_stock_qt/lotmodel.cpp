#include "lotmodel.h"
#include <QBrush>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QFile>

LotModel::LotModel(QObject *parent) : QAbstractTableModel(parent) {}

int LotModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_lots.size();
}
int LotModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : ColCount;
}

QVariant LotModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid() || idx.row() >= m_lots.size())
        return {};
    const Lot &l = m_lots.at(idx.row());

    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
        case ColId:        return l.id;
        case ColDate:      return l.dateTime.toString("dd/MM HH:mm");
        case ColLine:      return l.line;
        case ColProduct:   return l.product;
        case ColProduced:  return WF::fmt(l.produced);
        case ColStock:     return WF::fmt(l.stock);
        case ColThreshold: return WF::fmt(l.threshold);
        case ColStatus:    return l.status;
        }
    } else if (role == SortRole) {
        switch (idx.column()) {
        case ColId:        return l.id;
        case ColDate:      return l.dateTime;
        case ColLine:      return l.line;
        case ColProduct:   return l.product;
        case ColProduced:  return l.produced;
        case ColStock:     return l.stock;
        case ColThreshold: return l.threshold;
        case ColStatus:    return l.status;
        }
    } else if (role == Qt::ForegroundRole) {
        // stock sous le seuil du lot -> rouge
        if (idx.column() == ColStock && l.stock < l.threshold)
            return QBrush(QColor("#d63031"));
    } else if (role == Qt::TextAlignmentRole) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return {};
}

QVariant LotModel::headerData(int section, Qt::Orientation o, int role) const {
    if (o != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    static const QStringList h{"ID lot", "Date / heure", "Ligne", "Produit",
                               "Qté produite", "Stock", "Seuil", "Statut"};
    return section >= 0 && section < h.size() ? h.at(section) : QVariant();
}

int LotModel::rowOfId(int id) const {
    for (int i = 0; i < m_lots.size(); ++i)
        if (m_lots.at(i).id == id)
            return i;
    return -1;
}

int LotModel::nextId() const {
    int m = 0;
    for (const Lot &l : m_lots)
        m = qMax(m, l.id);
    return m + 1;
}

void LotModel::addLot(const Lot &lot) {
    beginInsertRows(QModelIndex(), m_lots.size(), m_lots.size());
    m_lots.append(lot);
    endInsertRows();
    emit lotsChanged();
}

void LotModel::updateLot(int row, const Lot &lot) {
    if (row < 0 || row >= m_lots.size())
        return;
    m_lots[row] = lot;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
    emit lotsChanged();
}

void LotModel::removeLot(int row) {
    if (row < 0 || row >= m_lots.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_lots.removeAt(row);
    endRemoveRows();
    emit lotsChanged();
}

bool LotModel::load(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonArray arr = doc.object().value("lots").toArray();
    beginResetModel();
    m_lots.clear();
    for (const QJsonValue &v : arr)
        m_lots.append(Lot::fromJson(v.toObject()));
    endResetModel();
    return true;
}

bool LotModel::save(const QString &path) const {
    QJsonArray arr;
    for (const Lot &l : m_lots)
        arr.append(l.toJson());
    QJsonObject root;
    root["lots"] = arr;
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

void LotModel::seedDemo() {
    const QDate today = QDate::currentDate();
    auto mk = [&](int id, int daysAgo, int h, int m, const QString &line,
                  const QString &prod, int produced, int stock, int thr,
                  const QString &op, const QString &st) {
        Lot l;
        l.id = id;
        l.dateTime = QDateTime(today.addDays(-daysAgo), QTime(h, m));
        l.line = line; l.product = prod; l.produced = produced;
        l.stock = stock; l.threshold = thr; l.operatorName = op; l.status = st;
        return l;
    };
    beginResetModel();
    m_lots.clear();
    // historique (alimente le graphique « production par jour »)
    m_lots << mk(575, 4, 7, 30, "Ligne 1", "Eau plate 0,5 L", 42000, 15000, 4000, "Ben Salah Amine", "Terminé")
           << mk(576, 3, 8, 0,  "Ligne 2", "Eau plate 1,5 L", 38000, 5000, 3000, "Trabelsi Sami", "Terminé")
           << mk(577, 2, 7, 45, "Ligne 1", "Eau plate 0,5 L", 51000, 16000, 4000, "Ben Salah Amine", "Terminé")
           << mk(578, 1, 9, 15, "Ligne 3", "Eau gazeuse 0,5 L", 47000, 5200, 1200, "Gharbi Nour", "Terminé");
    // lots du jour (comme sur la maquette)
    m_lots << mk(587, 0, 6, 10,  "Ligne 1", "Eau plate 0,5 L",   24000, 18400, 4000, "Trabelsi Sami", "Terminé")
           << mk(588, 0, 8, 40,  "Ligne 2", "Eau plate 1,5 L",   12500, 6200,  3000, "Trabelsi Sami", "Terminé")
           << mk(589, 0, 10, 15, "Ligne 3", "Eau gazeuse 0,5 L", 6000,  900,   1200, "Ben Salah Amine", "En cours")
           << mk(590, 0, 11, 30, "Ligne 1", "Eau plate 0,5 L",   18000, 18400, 4000, "Gharbi Nour", "Terminé")
           << mk(591, 0, 13, 0,  "Ligne 4", "Bonbonne 19 L",     1500,  2100,  500,  "Gharbi Nour", "Terminé");
    endResetModel();
}

// ---------------------------------------------------------------------------
bool LotFilterProxy::filterAcceptsRow(int row, const QModelIndex &) const {
    auto *m = qobject_cast<LotModel *>(sourceModel());
    if (!m || row < 0 || row >= m->rowCount())
        return false;
    const Lot &l = m->lotAt(row);
    if (!m_product.isEmpty() && l.product != m_product)
        return false;
    if (!m_status.isEmpty() && l.status != m_status)
        return false;
    if (!m_search.isEmpty()) {
        const QString hay = QString::number(l.id) + ' ' + l.product + ' ' + l.line + ' ' +
                            l.operatorName + ' ' + l.status + ' ' +
                            l.dateTime.toString("dd/MM/yyyy HH:mm");
        if (!hay.contains(m_search, Qt::CaseInsensitive))
            return false;
    }
    return true;
}
