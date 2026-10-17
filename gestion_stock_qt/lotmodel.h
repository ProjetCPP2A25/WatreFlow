#pragma once
#include "lot.h"
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QVector>

class LotModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColId, ColDate, ColLine, ColProduct, ColProduced, ColStock,
                  ColThreshold, ColStatus, ColCount };
    enum Role { SortRole = Qt::UserRole + 1 };

    explicit LotModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;

    const QVector<Lot> &lots() const { return m_lots; }
    const Lot &lotAt(int row) const { return m_lots.at(row); }
    int rowOfId(int id) const;
    int nextId() const;

    void addLot(const Lot &lot);
    void updateLot(int row, const Lot &lot);
    void removeLot(int row);

    bool load(const QString &path);
    bool save(const QString &path) const;
    void seedDemo();

signals:
    void lotsChanged(); // émis après toute modification (ajout/maj/suppression)

private:
    QVector<Lot> m_lots;
};

// Filtrage : recherche texte + produit + statut
class LotFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    void setSearch(const QString &s) { m_search = s.trimmed(); invalidateFilter(); }
    void setProduct(const QString &p) { m_product = p; invalidateFilter(); }
    void setStatus(const QString &s) { m_status = s; invalidateFilter(); }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
    QString m_search, m_product, m_status; // vide = pas de filtre
};
