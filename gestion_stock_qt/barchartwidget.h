#pragma once
#include <QStringList>
#include <QVector>
#include <QWidget>

// Petit histogramme dessiné à la main (aucune dépendance à Qt Charts)
class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setData(const QStringList &labels, const QVector<double> &values);
    QSize minimumSizeHint() const override { return {200, 140}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QStringList m_labels;
    QVector<double> m_values;
};
