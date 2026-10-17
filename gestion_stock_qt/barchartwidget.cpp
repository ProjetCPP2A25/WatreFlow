#include "barchartwidget.h"
#include <QPainter>
#include <algorithm>

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void BarChartWidget::setData(const QStringList &labels, const QVector<double> &values) {
    m_labels = labels;
    m_values = values;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int n = m_values.size();
    if (n == 0)
        return;

    const QRectF area(8, 20, width() - 16, height() - 20 - 22);
    double maxV = 1;
    for (double v : m_values)
        maxV = std::max(maxV, v);
    const double slot = area.width() / n;
    const double bw = slot * 0.55;

    // ligne de base
    p.setPen(QPen(QColor("#d3e1ec"), 1));
    p.drawLine(QPointF(area.left(), area.bottom()), QPointF(area.right(), area.bottom()));

    QFont bold = font();
    bold.setBold(true);
    bold.setPointSizeF(8);
    QFont small = font();
    small.setPointSizeF(8);

    for (int i = 0; i < n; ++i) {
        const double h = (area.height() - 4) * (m_values[i] / maxV);
        const double x = area.left() + i * slot + (slot - bw) / 2;
        const QRectF bar(x, area.bottom() - h, bw, h);
        p.setPen(Qt::NoPen);
        p.setBrush(i % 2 == 0 ? QColor("#0d4b8b") : QColor("#00aeda"));
        p.drawRoundedRect(bar, 2, 2);

        p.setPen(QColor("#0b3d73"));
        p.setFont(bold);
        p.drawText(QRectF(x - 10, bar.top() - 16, bw + 20, 14), Qt::AlignCenter,
                   QString::number(m_values[i], 'f', 0));
        p.setPen(QColor("#6b7f94"));
        p.setFont(small);
        p.drawText(QRectF(x - 10, area.bottom() + 4, bw + 20, 16), Qt::AlignCenter,
                   i < m_labels.size() ? m_labels[i] : QString());
    }
}
