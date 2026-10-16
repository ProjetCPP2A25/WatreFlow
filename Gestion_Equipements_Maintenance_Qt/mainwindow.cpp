#include "mainwindow.h"

#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <QMap>
#include <QPageLayout>
#include <QColor>
#include <QFrame>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QAbstractItemView>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QMenuBar>
#include <QStatusBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileDialog>
#include <algorithm>

static QWidget *makeCard(const QString &label, QLabel **value, const QString &accent)
{
    auto *card = new QFrame;
    card->setObjectName("metricCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    auto *top = new QLabel(label);
    top->setObjectName("metricLabel");
    *value = new QLabel("0");
    (*value)->setObjectName("metricValue");
    (*value)->setStyleSheet(QString("color:%1;").arg(accent));
    layout->addWidget(top);
    layout->addWidget(*value);
    return card;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("WaterFlow | Équipements & Maintenance");
    resize(1420, 860);
    setupStyle();
    seedData();

    auto *root = new QWidget;
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(245);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(18, 24, 18, 20);
    side->setSpacing(8);

    auto *brand = new QLabel("💧  WaterFlow");
    brand->setObjectName("brand");
    auto *subtitle = new QLabel("SMART WATER FACTORY");
    subtitle->setObjectName("brandSub");
    side->addWidget(brand);
    side->addWidget(subtitle);
    side->addSpacing(28);

    auto *section = new QLabel("ESPACE MAINTENANCE");
    section->setObjectName("sideSection");
    side->addWidget(section);

    auto *stack = new QStackedWidget;
    QWidget *dashboard = buildDashboardPage();
    QWidget *equipPage = buildEquipmentPage();
    QWidget *maintenancePage = buildMaintenancePage();
    QWidget *statsPage = buildStatisticsPage();
    stack->addWidget(dashboard);
    stack->addWidget(equipPage);
    stack->addWidget(maintenancePage);
    stack->addWidget(statsPage);

    const QStringList navLabels = {
        "▦   Vue d'ensemble",
        "⚙   Équipements",
        "◷   Maintenance préventive",
        "▥   Fiabilité & statistiques"
    };
    for (int i = 0; i < navLabels.size(); ++i) {
        auto *btn = new QPushButton(navLabels[i]);
        btn->setObjectName("navButton");
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        side->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [=]() {
            stack->setCurrentIndex(i);
            for (int j = 0; j < side->count(); ++j) {
                if (auto *b = qobject_cast<QPushButton*>(side->itemAt(j)->widget()))
                    b->setChecked(b == btn);
            }
        });
    }
    side->addStretch();

    auto *userCard = new QFrame;
    userCard->setObjectName("userCard");
    auto *userLayout = new QVBoxLayout(userCard);
    auto *userName = new QLabel("Yasmine Daadoucha");
    userName->setStyleSheet("font-weight:700;color:white;");
    auto *userRole = new QLabel("Responsable maintenance");
    userRole->setStyleSheet("color:#B8D5E8;font-size:11px;");
    userLayout->addWidget(userName);
    userLayout->addWidget(userRole);
    side->addWidget(userCard);

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(28, 22, 28, 24);
    contentLayout->setSpacing(16);

    auto *topbar = new QHBoxLayout;
    auto *pageTitle = new QLabel("Vue d'ensemble");
    pageTitle->setObjectName("pageTitle");
    auto *topSpacer = new QWidget;
    topSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *today = new QLabel(QDate::currentDate().toString("dddd dd MMMM yyyy"));
    today->setObjectName("dateLabel");
    auto *avatar = new QLabel("YD");
    avatar->setObjectName("avatar");
    topbar->addWidget(pageTitle);
    topbar->addWidget(topSpacer);
    topbar->addWidget(today);
    topbar->addSpacing(12);
    topbar->addWidget(avatar);
    contentLayout->addLayout(topbar);

    connect(stack, &QStackedWidget::currentChanged, this, [=](int index) {
        const QStringList titles = {"Vue d'ensemble", "Gestion des équipements", "Maintenance préventive", "Fiabilité & statistiques"};
        pageTitle->setText(titles.value(index));
        if (index == 0 || index == 1 || index == 2 || index == 3) {
            refreshTable();
            refreshMetrics();
        }
    });

    contentLayout->addWidget(stack);
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(content);
    setCentralWidget(root);
    statusBar()->showMessage("Connecté • Mode démonstration — données en mémoire", 5000);
    refreshTable();
    refreshMetrics();
}

void MainWindow::setupStyle()
{
    setStyleSheet(R"(
        QMainWindow, QWidget { background:#F3F7FB; color:#17324D; font-family:"Segoe UI"; font-size:10pt; }
        #sidebar { background:#0A4D8C; }
        #brand { color:white; font-size:23pt; font-weight:800; }
        #brandSub { color:#8EDCF0; font-size:8pt; font-weight:700; letter-spacing:1px; }
        #sideSection { color:#91B8D6; font-size:8pt; font-weight:700; margin-top:5px; }
        QPushButton#navButton { text-align:left; color:#DCEAF5; background:transparent; border:0; padding:13px 12px; border-radius:7px; font-size:10pt; }
        QPushButton#navButton:hover { background:#1765A2; color:white; }
        QPushButton#navButton:checked { background:#00B4D8; color:#073653; font-weight:700; }
        #userCard { background:#124678; border:1px solid #2B6495; border-radius:10px; }
        #pageTitle { font-size:21pt; font-weight:800; color:#0A4D8C; }
        #dateLabel { color:#6F8498; }
        #avatar { background:#D5F3FA; color:#0A4D8C; font-weight:800; border-radius:18px; min-width:36px; min-height:36px; qproperty-alignment: AlignCenter; }
        #metricCard, #panel { background:white; border:1px solid #E0EAF2; border-radius:12px; }
        #metricLabel { color:#71869A; font-size:9pt; font-weight:600; }
        #metricValue { font-size:25pt; font-weight:800; }
        QLabel#sectionTitle { color:#0A4D8C; font-size:13pt; font-weight:800; }
        QLabel#muted { color:#71869A; }
        QLineEdit, QComboBox, QDateEdit, QSpinBox, QTextEdit { background:#FFFFFF; border:1px solid #D5E1EB; border-radius:6px; padding:8px; min-height:18px; selection-background-color:#00B4D8; }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QSpinBox:focus { border:1px solid #00B4D8; }
        QPushButton { background:#00B4D8; color:#063954; border:0; border-radius:6px; padding:9px 14px; font-weight:700; }
        QPushButton:hover { background:#0A4D8C; color:white; }
        QPushButton#secondary { background:#E6F4FA; color:#0A4D8C; border:1px solid #D0E8F2; }
        QPushButton#danger { background:#FDE9E7; color:#C0392B; }
        QTableWidget { background:white; border:1px solid #E0EAF2; border-radius:8px; gridline-color:#EAF0F5; selection-background-color:#D9F4FA; selection-color:#17324D; alternate-background-color:#F7FBFE; }
        QHeaderView::section { background:#0A4D8C; color:white; border:0; padding:11px 8px; font-weight:700; }
        QTableWidget::item { padding:7px; }
        QScrollBar:vertical { background:#EEF4F8; width:10px; margin:0; }
        QScrollBar::handle:vertical { background:#B9CEDD; border-radius:5px; min-height:20px; }
    )");
}

void MainWindow::seedData()
{
    const QDate today = QDate::currentDate();
    equipment = {
        {1001, "Pompe de captage P-01", "Pompe", "Atelier A", today.addYears(-3), 30, today.addDays(-24), "En service", "M. Ben Salem"},
        {1002, "Filtre industriel F-02", "Filtre", "Zone filtration", today.addYears(-2), 45, today.addDays(-50), "En maintenance", "S. Trabelsi"},
        {1003, "Ligne embouteillage L-01", "Ligne de production", "Atelier B", today.addYears(-4), 60, today.addDays(-67), "En panne", "M. Ben Salem"},
        {1004, "Réservoir R-03", "Réservoir", "Zone stockage", today.addYears(-1), 90, today.addDays(-18), "En service", "A. Gharbi"},
        {1005, "Pompe de transfert P-04", "Pompe", "Atelier A", today.addYears(-2), 30, today.addDays(-29), "En service", "S. Trabelsi"},
        {1006, "Filtre de sécurité F-05", "Filtre", "Zone filtration", today.addYears(-1), 45, today.addDays(-12), "Hors service", "A. Gharbi"}
    };
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,8,0,0);
    layout->setSpacing(18);
    auto *intro = new QLabel("Bonjour");
    intro->setStyleSheet("font-size:17pt;font-weight:800;color:#17324D;");
    auto *desc = new QLabel("Voici l'état actuel du parc d'équipements et les actions à prioriser.");
    desc->setObjectName("muted");
    layout->addWidget(intro);
    layout->addWidget(desc);

    auto *cards = new QGridLayout;
    cards->setSpacing(14);
    cards->addWidget(makeCard("ÉQUIPEMENTS TOTAL", &totalValue, "#0A4D8C"),0,0);
    cards->addWidget(makeCard("EN SERVICE", &serviceValue, "#27AE60"),0,1);
    cards->addWidget(makeCard("MAINTENANCE À SUIVRE", &maintenanceValue, "#D9820B"),0,2);
    cards->addWidget(makeCard("EN PANNE / HORS SERVICE", &brokenValue, "#D94343"),0,3);
    layout->addLayout(cards);

    auto *panel = new QFrame;
    panel->setObjectName("panel");
    auto *p = new QVBoxLayout(panel);
    p->setContentsMargins(20,18,20,18);
    auto *title = new QLabel("À traiter en priorité");
    title->setObjectName("sectionTitle");
    auto *sub = new QLabel("Les échéances proches et les équipements nécessitant une intervention.");
    sub->setObjectName("muted");
    p->addWidget(title);
    p->addWidget(sub);
    auto *alertTable = new QTableWidget(0,4);
    alertTable->setHorizontalHeaderLabels({"Équipement","État","Prochaine maintenance","Priorité"});
    alertTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    alertTable->verticalHeader()->setVisible(false);
    alertTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    alertTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    alertTable->setMinimumHeight(260);
    p->addWidget(alertTable);
    for (const auto &e : equipment) {
        const QDate due = nextMaintenance(e);
        if (e.state == "En panne" || e.state == "Hors service" || due <= QDate::currentDate().addDays(7)) {
            int r = alertTable->rowCount();
            alertTable->insertRow(r);
            alertTable->setItem(r,0,new QTableWidgetItem(e.name));
            alertTable->setItem(r,1,new QTableWidgetItem(e.state));
            alertTable->setItem(r,2,new QTableWidgetItem(due.toString("dd/MM/yyyy")));
            QString priority = (e.state == "En panne" || e.state == "Hors service") ? "URGENT" : (due < QDate::currentDate() ? "EN RETARD" : "À PLANIFIER");
            auto *item = new QTableWidgetItem(priority);
            item->setForeground(QColor((priority=="URGENT" || priority=="EN RETARD") ? "#D94343" : "#D9820B"));
            alertTable->setItem(r,3,item);
        }
    }
    layout->addWidget(panel);
    layout->addStretch();
    return page;
}

QWidget *MainWindow::buildEquipmentPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,8,0,0);
    layout->setSpacing(14);

    auto *toolbar = new QHBoxLayout;
    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("Rechercher par nom, atelier, responsable…");
    searchEdit->setMinimumWidth(280);
    typeFilter = new QComboBox;
    typeFilter->addItems({"Tous les types","Pompe","Filtre","Ligne de production","Réservoir"});
    stateFilter = new QComboBox;
    stateFilter->addItems({"Tous les états","En service","En maintenance","Hors service","En panne"});
    auto *newBtn = new QPushButton("+ Nouvel équipement");
    toolbar->addWidget(searchEdit,2);
    toolbar->addWidget(typeFilter);
    toolbar->addWidget(stateFilter);
    toolbar->addWidget(newBtn);
    layout->addLayout(toolbar);

    auto *panel = new QFrame;
    panel->setObjectName("panel");
    auto *p = new QVBoxLayout(panel);
    p->setContentsMargins(14,14,14,14);
    auto *section = new QLabel("Parc des équipements");
    section->setObjectName("sectionTitle");
    auto *hint = new QLabel("Sélectionnez une ligne pour modifier ou supprimer une fiche.");
    hint->setObjectName("muted");
    p->addWidget(section);
    p->addWidget(hint);

    table = new QTableWidget(0,8);
    table->setHorizontalHeaderLabels({"ID","Désignation","Type","Atelier / zone","État","Dernière maintenance","Prochaine maintenance","Responsable"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setMinimumHeight(340);
    p->addWidget(table);

    auto *actions = new QHBoxLayout;
    auto *editBtn = new QPushButton("Modifier la fiche");
    editBtn->setObjectName("secondary");
    auto *deleteBtn = new QPushButton("Supprimer");
    deleteBtn->setObjectName("danger");
    auto *pdfBtn = new QPushButton("Exporter / Imprimer PDF");
    pdfBtn->setObjectName("secondary");
    actions->addWidget(editBtn);
    actions->addWidget(deleteBtn);
    actions->addStretch();
    actions->addWidget(pdfBtn);
    p->addLayout(actions);
    layout->addWidget(panel);

    auto *formPanel = new QFrame;
    formPanel->setObjectName("panel");
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(18,16,18,16);
    auto *formTitle = new QLabel("Fiche équipement");
    formTitle->setObjectName("sectionTitle");
    formLayout->addWidget(formTitle);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(9);
    nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("Ex. Pompe de captage P-07");
    typeEdit = new QComboBox;
    typeEdit->addItems({"Pompe","Filtre","Ligne de production","Réservoir","Autre"});
    locationEdit = new QLineEdit;
    locationEdit->setPlaceholderText("Ex. Atelier A");
    commissioningEdit = new QDateEdit(QDate::currentDate());
    commissioningEdit->setCalendarPopup(true);
    frequencyEdit = new QSpinBox;
    frequencyEdit->setRange(1,3650);
    frequencyEdit->setValue(30);
    frequencyEdit->setSuffix(" jours");
    lastMaintenanceEdit = new QDateEdit(QDate::currentDate());
    lastMaintenanceEdit->setCalendarPopup(true);
    stateEdit = new QComboBox;
    stateEdit->addItems({"En service","En maintenance","Hors service","En panne"});
    managerEdit = new QLineEdit;
    managerEdit->setPlaceholderText("Nom du responsable technique");
    auto addField = [&](int row,int col,const QString &label,QWidget *w) {
        auto *v = new QVBoxLayout;
        auto *l = new QLabel(label);
        l->setStyleSheet("font-weight:700;color:#526A80;font-size:9pt;");
        v->addWidget(l);
        v->addWidget(w);
        grid->addLayout(v,row,col);
    };
    addField(0,0,"Désignation *",nameEdit);
    addField(0,1,"Type",typeEdit);
    addField(0,2,"Atelier / zone",locationEdit);
    addField(0,3,"État",stateEdit);
    addField(1,0,"Date de mise en service",commissioningEdit);
    addField(1,1,"Fréquence préventive",frequencyEdit);
    addField(1,2,"Dernière maintenance",lastMaintenanceEdit);
    addField(1,3,"Responsable technique",managerEdit);
    formLayout->addLayout(grid);
    auto *formActions = new QHBoxLayout;
    auto *saveBtn = new QPushButton("Enregistrer l'équipement");
    auto *clearBtn = new QPushButton("Réinitialiser");
    clearBtn->setObjectName("secondary");
    formActions->addStretch();
    formActions->addWidget(clearBtn);
    formActions->addWidget(saveBtn);
    formLayout->addLayout(formActions);
    layout->addWidget(formPanel);

    connect(searchEdit,&QLineEdit::textChanged,this,&MainWindow::refreshTable);
    connect(typeFilter,&QComboBox::currentTextChanged,this,&MainWindow::refreshTable);
    connect(stateFilter,&QComboBox::currentTextChanged,this,&MainWindow::refreshTable);
    connect(newBtn,&QPushButton::clicked,this,[=](){ clearForm(); nameEdit->setFocus(); });
    connect(saveBtn,&QPushButton::clicked,this,&MainWindow::saveEquipment);
    connect(clearBtn,&QPushButton::clicked,this,&MainWindow::clearForm);
    connect(editBtn,&QPushButton::clicked,this,&MainWindow::loadSelectedIntoForm);
    connect(deleteBtn,&QPushButton::clicked,this,&MainWindow::deleteEquipment);
    connect(table,&QTableWidget::cellDoubleClicked,this,[=](int,int){loadSelectedIntoForm();});
    connect(pdfBtn,&QPushButton::clicked,this,[=](){
        QString path = QFileDialog::getSaveFileName(this,"Exporter la liste en PDF","equipements.pdf","PDF (*.pdf)");
        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf",Qt::CaseInsensitive)) path += ".pdf";
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        printer.setPageOrientation(QPageLayout::Landscape);
        QTextDocument doc;
        QString html = "<h1 style='color:#0A4D8C'>WaterFlow — Parc des équipements</h1><p>Export du " + QDate::currentDate().toString("dd/MM/yyyy") + "</p><table border='1' cellspacing='0' cellpadding='6' width='100%'><tr style='background:#0A4D8C;color:white'><th>ID</th><th>Désignation</th><th>Type</th><th>Zone</th><th>État</th><th>Prochaine maintenance</th><th>Responsable</th></tr>";
        for (const auto &e : equipment) {
            html += "<tr><td>"+QString::number(e.id)+"</td><td>"+e.name.toHtmlEscaped()+"</td><td>"+e.type.toHtmlEscaped()+"</td><td>"+e.location.toHtmlEscaped()+"</td><td>"+e.state.toHtmlEscaped()+"</td><td>"+nextMaintenance(e).toString("dd/MM/yyyy")+"</td><td>"+e.manager.toHtmlEscaped()+"</td></tr>";
        }
        html += "</table>";
        doc.setHtml(html);
        doc.print(&printer);
        QMessageBox::information(this,"Export PDF","Le fichier PDF a été créé.");
    });
    return page;
}

QWidget *MainWindow::buildMaintenancePage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,8,0,0);
    auto *intro = new QLabel("Anticipez les pannes grâce à un planning de maintenance préventive.");
    intro->setObjectName("muted");
    layout->addWidget(intro);

    auto *cards = new QGridLayout;
    QLabel *dueNow, *dueSoon, *overdue;
    cards->addWidget(makeCard("EN RETARD", &overdue, "#D94343"),0,0);
    cards->addWidget(makeCard("DANS LES 7 JOURS", &dueSoon, "#D9820B"),0,1);
    cards->addWidget(makeCard("PROCHAINE ÉCHÉANCE", &dueNow, "#0A4D8C"),0,2);
    layout->addLayout(cards);

    auto *panel = new QFrame;
    panel->setObjectName("panel");
    auto *p = new QVBoxLayout(panel);
    p->setContentsMargins(18,18,18,18);
    auto *title = new QLabel("Planning des interventions");
    title->setObjectName("sectionTitle");
    p->addWidget(title);
    auto *schedule = new QTableWidget(0,5);
    schedule->setHorizontalHeaderLabels({"Équipement","Responsable","Dernière maintenance","Échéance calculée","Alerte"});
    schedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    schedule->verticalHeader()->setVisible(false);
    schedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    p->addWidget(schedule);
    int late = 0, soon = 0;
    QDate earliest;
    for (const auto &e : equipment) {
        QDate due = nextMaintenance(e);
        if (due < QDate::currentDate()) ++late;
        if (due >= QDate::currentDate() && due <= QDate::currentDate().addDays(7)) ++soon;
        if (!earliest.isValid() || due < earliest) earliest = due;
        int r = schedule->rowCount();
        schedule->insertRow(r);
        schedule->setItem(r,0,new QTableWidgetItem(e.name));
        schedule->setItem(r,1,new QTableWidgetItem(e.manager));
        schedule->setItem(r,2,new QTableWidgetItem(e.lastMaintenance.toString("dd/MM/yyyy")));
        schedule->setItem(r,3,new QTableWidgetItem(due.toString("dd/MM/yyyy")));
        QString alert = due < QDate::currentDate() ? "En retard" : (due <= QDate::currentDate().addDays(7) ? "À planifier" : "Planifiée");
        auto *a = new QTableWidgetItem(alert);
        a->setForeground(QColor(alert=="En retard" ? "#D94343" : (alert=="À planifier" ? "#D9820B" : "#27AE60")));
        schedule->setItem(r,4,a);
    }
    overdue->setText(QString::number(late));
    dueSoon->setText(QString::number(soon));
    dueNow->setText(earliest.isValid() ? earliest.toString("dd/MM/yyyy") : "—");
    layout->addWidget(panel);
    layout->addStretch();
    return page;
}

QWidget *MainWindow::buildStatisticsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0,8,0,0);
    auto *desc = new QLabel("Indicateurs de suivi du parc — calculés à partir des fiches d'équipements.");
    desc->setObjectName("muted");
    layout->addWidget(desc);

    auto *grid = new QGridLayout;
    auto *statePanel = new QFrame; statePanel->setObjectName("panel");
    auto *stateLayout = new QVBoxLayout(statePanel);
    auto *stateTitle = new QLabel("Répartition par état"); stateTitle->setObjectName("sectionTitle");
    stateLayout->addWidget(stateTitle);
    auto *stateBars = new QVBoxLayout;
    const QStringList states = {"En service","En maintenance","Hors service","En panne"};
    const QStringList colors = {"#2ECC71","#F39C12","#95A5A6","#E74C3C"};
    for (int i=0;i<states.size();++i) {
        auto *row = new QHBoxLayout;
        auto *label = new QLabel(states[i]); label->setMinimumWidth(145);
        auto *bar = new QFrame; bar->setFixedHeight(16); bar->setStyleSheet(QString("background:%1;border-radius:5px;").arg(colors[i]));
        auto *count = new QLabel("0");
        row->addWidget(label); row->addWidget(bar,1); row->addWidget(count);
        stateBars->addLayout(row);
        bar->setProperty("stateName",states[i]);
        count->setProperty("stateName",states[i]);
        count->setObjectName("stateCount");
    }
    stateLayout->addLayout(stateBars);
    auto *typePanel = new QFrame; typePanel->setObjectName("panel");
    auto *typeLayout = new QVBoxLayout(typePanel);
    auto *typeTitle = new QLabel("Répartition par type"); typeTitle->setObjectName("sectionTitle");
    typeLayout->addWidget(typeTitle);
    auto *typeText = new QLabel; typeText->setObjectName("typeStats"); typeText->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    typeText->setStyleSheet("font-size:12pt;line-height:1.6;");
    typeLayout->addWidget(typeText,1);
    auto *reliabilityPanel = new QFrame; reliabilityPanel->setObjectName("panel");
    auto *reliabilityLayout = new QVBoxLayout(reliabilityPanel);
    auto *relTitle = new QLabel("Indice de fiabilité simplifié"); relTitle->setObjectName("sectionTitle");
    auto *relValue = new QLabel; relValue->setObjectName("reliabilityValue");
    relValue->setStyleSheet("font-size:30pt;font-weight:800;color:#0A4D8C;");
    auto *relDesc = new QLabel("Part des équipements actuellement en service. Le MTBF nécessite un historique réel des pannes et interventions.");
    relDesc->setWordWrap(true); relDesc->setObjectName("muted");
    reliabilityLayout->addWidget(relTitle);
    reliabilityLayout->addWidget(relValue);
    reliabilityLayout->addWidget(relDesc);
    reliabilityLayout->addStretch();
    grid->addWidget(statePanel,0,0);
    grid->addWidget(typePanel,0,1);
    grid->addWidget(reliabilityPanel,1,0,1,2);
    layout->addLayout(grid);
    layout->addStretch();

    QTimer *timer = new QTimer(page);
    timer->setSingleShot(true);
    connect(timer,&QTimer::timeout,page,[=](){
        QMap<QString,int> stateCounts, typeCounts;
        int inService=0;
        for (const auto &e : equipment) {
            stateCounts[e.state]++;
            typeCounts[e.type]++;
            if (e.state=="En service") ++inService;
        }
        for (auto *count : page->findChildren<QLabel*>("stateCount")) {
            QString state = count->property("stateName").toString();
            count->setText(QString::number(stateCounts[state]));
        }
        QString text;
        for (auto it=typeCounts.cbegin();it!=typeCounts.cend();++it)
            text += QString("•  %1 : <b>%2</b><br>").arg(it.key().toHtmlEscaped()).arg(it.value());
        typeText->setText(text.isEmpty() ? "Aucune donnée" : text);
        relValue->setText(equipment.isEmpty() ? "—" : QString("%1 %").arg(qRound(100.0*inService/equipment.size())));
    });
    timer->start(0);
    return page;
}

QDate MainWindow::nextMaintenance(const Equipment &e) const
{
    return e.lastMaintenance.addDays(e.frequency);
}

QString MainWindow::stateColor(const QString &state) const
{
    if (state=="En service") return "#2ECC71";
    if (state=="En maintenance") return "#F39C12";
    if (state=="En panne") return "#E74C3C";
    return "#95A5A6";
}

void MainWindow::refreshTable()
{
    if (!table) return;
    table->setRowCount(0);
    for (const auto &e : equipment) {
        if (typeFilter && typeFilter->currentIndex()>0 && e.type!=typeFilter->currentText()) continue;
        if (stateFilter && stateFilter->currentIndex()>0 && e.state!=stateFilter->currentText()) continue;
        QString needle = searchEdit ? searchEdit->text().trimmed() : "";
        if (!needle.isEmpty() && !QString("%1 %2 %3 %4 %5").arg(e.name,e.type,e.location,e.manager,QString::number(e.id)).contains(needle,Qt::CaseInsensitive)) continue;
        int r=table->rowCount(); table->insertRow(r);
        QStringList vals = {QString::number(e.id),e.name,e.type,e.location,e.state,e.lastMaintenance.toString("dd/MM/yyyy"),nextMaintenance(e).toString("dd/MM/yyyy"),e.manager};
        for (int c=0;c<vals.size();++c) {
            auto *item=new QTableWidgetItem(vals[c]);
            if (c==4) item->setForeground(QColor(stateColor(e.state)));
            table->setItem(r,c,item);
        }
    }
}

void MainWindow::refreshMetrics()
{
    if (totalValue) {
        int service=0, maintenance=0, broken=0;
        for (const auto &e : equipment) {
            if (e.state=="En service") ++service;
            else if (e.state=="En maintenance") ++maintenance;
            else ++broken;
        }
        totalValue->setText(QString::number(equipment.size()));
        serviceValue->setText(QString::number(service));
        maintenanceValue->setText(QString::number(maintenance));
        brokenValue->setText(QString::number(broken));
    }
}

void MainWindow::refreshFilters()
{
    refreshTable();
}

void MainWindow::clearForm()
{
    editingId=-1;
    if (!nameEdit) return;
    nameEdit->clear();
    typeEdit->setCurrentIndex(0);
    locationEdit->clear();
    commissioningEdit->setDate(QDate::currentDate());
    frequencyEdit->setValue(30);
    lastMaintenanceEdit->setDate(QDate::currentDate());
    stateEdit->setCurrentText("En service");
    managerEdit->clear();
}

void MainWindow::loadSelectedIntoForm()
{
    if (!table || table->currentRow()<0) {
        QMessageBox::information(this,"Sélection requise","Sélectionnez d'abord un équipement dans le tableau.");
        return;
    }
    int id=table->item(table->currentRow(),0)->text().toInt();
    for (const auto &e : equipment) if (e.id==id) {
        editingId=id;
        nameEdit->setText(e.name);
        typeEdit->setCurrentText(e.type);
        locationEdit->setText(e.location);
        commissioningEdit->setDate(e.commissioning);
        frequencyEdit->setValue(e.frequency);
        lastMaintenanceEdit->setDate(e.lastMaintenance);
        stateEdit->setCurrentText(e.state);
        managerEdit->setText(e.manager);
        statusBar()->showMessage(QString("Modification de la fiche #%1").arg(id),4000);
        return;
    }
}

void MainWindow::saveEquipment()
{
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this,"Champ obligatoire","La désignation de l'équipement est obligatoire.");
        nameEdit->setFocus();
        return;
    }
    if (locationEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this,"Champ obligatoire","Veuillez renseigner l'atelier ou la zone.");
        locationEdit->setFocus();
        return;
    }
    if (managerEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this,"Champ obligatoire","Veuillez renseigner le responsable technique.");
        managerEdit->setFocus();
        return;
    }
    if (lastMaintenanceEdit->date()>QDate::currentDate()) {
        QMessageBox::warning(this,"Date invalide","La dernière maintenance ne peut pas être dans le futur.");
        return;
    }

    Equipment e;
    e.id=editingId;
    e.name=nameEdit->text().trimmed();
    e.type=typeEdit->currentText();
    e.location=locationEdit->text().trimmed();
    e.commissioning=commissioningEdit->date();
    e.frequency=frequencyEdit->value();
    e.lastMaintenance=lastMaintenanceEdit->date();
    e.state=stateEdit->currentText();
    e.manager=managerEdit->text().trimmed();

    if (editingId<0) {
        int maxId=1000;
        for (const auto &item : equipment) maxId=qMax(maxId,item.id);
        e.id=maxId+1;
        equipment.append(e);
        QMessageBox::information(this,"Équipement ajouté",QString("L'équipement #%1 a été ajouté.").arg(e.id));
    } else {
        for (auto &item : equipment) if (item.id==editingId) { item=e; break; }
        QMessageBox::information(this,"Fiche modifiée","Les informations ont été mises à jour.");
    }
    clearForm();
    refreshTable();
    refreshMetrics();
}

void MainWindow::deleteEquipment()
{
    if (!table || table->currentRow()<0) {
        QMessageBox::information(this,"Sélection requise","Sélectionnez un équipement à supprimer.");
        return;
    }
    int id=table->item(table->currentRow(),0)->text().toInt();
    auto answer=QMessageBox::question(this,"Confirmer la suppression",QString("Supprimer l'équipement #%1 ? Cette action est irréversible.").arg(id),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if (answer!=QMessageBox::Yes) return;
    for (int i=0;i<equipment.size();++i) if (equipment[i].id==id) { equipment.removeAt(i); break; }
    clearForm();
    refreshTable();
    refreshMetrics();
    QMessageBox::information(this,"Suppression","L'équipement a été supprimé.");
}

void MainWindow::showPage(QWidget *, const QString &) {}
