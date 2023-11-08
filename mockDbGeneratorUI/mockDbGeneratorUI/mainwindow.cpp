#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QProcess>
#include <QStringView>
#include <QMessageBox>
#include <utility>
#include <array>
#include <algorithm>

static void unpack_python_script() {
#define RC ":/resources/"
#define BASE
#define DG "dataGenerators/"
#define SR "structureReader/"
#define TW "typeWrappers/"
#define DT "data/"
#define MAKE_RC(name, fold) std::pair{QString{RC fold name}, QString{fold name}}
    std::array names{
        MAKE_RC("mockDbGenerator.py", BASE),
        MAKE_RC("__init__.py", DG),
        MAKE_RC("__init__.py", SR),
        MAKE_RC("__init__.py", TW),
        MAKE_RC("generators.py", DG),
        MAKE_RC("reader.py", SR),
        MAKE_RC("types.py", TW),
        MAKE_RC("female-names-list.txt", DT),
        MAKE_RC("male-names-list.txt", DT),
        MAKE_RC("surnames-list.txt", DT),
        MAKE_RC("words.txt", DT)
    };
    std::array directories{
        "dataGenerators",
        "structureReader",
        "typeWrappers",
        "data"
    };
#undef RC
#undef BASE
#undef DG
#undef SR
#undef TW
#undef DT
#undef MAKE_RC
    QDir d;
    for (const auto& dir : directories) {
        d.mkpath(dir);
    }
    for (const auto& [rcname, name] : names) {
        QFile rcfile{rcname};
        rcfile.open(QFile::OpenModeFlag::ReadOnly);
        QFile fsfile{name};
        fsfile.open(QFile::OpenModeFlag::WriteOnly);
        fsfile.write(rcfile.readAll());
        rcfile.close();
        fsfile.close();
    }
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Mock Database Generator");
    unpack_python_script();
    QWidget* mainWindow = new QWidget{this};
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* dumpWidget = new QWidget{mainWindow};
    QHBoxLayout* dumpLayout = new QHBoxLayout;
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, Qt::lightGray);
    dumpWidget->setAutoFillBackground(true);
    dumpWidget->setPalette(pal);
    dumpWidget->setLayout(dumpLayout);
    mainWindow->setLayout(layout);
    setCentralWidget(mainWindow);
    m_schema_name = new QLineEdit{"schema_name", dumpWidget};
    QPushButton* btn1 = new QPushButton{dumpWidget};
    QPushButton* btn2 = new QPushButton{dumpWidget};
    QPushButton* btn3 = new QPushButton{dumpWidget};
    QPushButton* btn4 = new QPushButton{dumpWidget};
    QPushButton* btn5 = new QPushButton{dumpWidget};
    btn1->setText("Add table");
    btn2->setText("Dump to json");
    btn3->setText("Generate data (csv)");
    btn4->setText("Generate data (sql)");
    btn5->setText("Import from JSON");
    dumpLayout->addWidget(m_schema_name);
    dumpLayout->addWidget(btn1);
    dumpLayout->addWidget(btn2);
    dumpLayout->addWidget(btn3);
    dumpLayout->addWidget(btn4);
    dumpLayout->addWidget(btn5);
    QObject::connect(btn1, &QPushButton::clicked, this, [this](int){
        add_table();
    });
    QObject::connect(btn2, &QPushButton::clicked, this, [this](int){
        dump_to_json();
    });
    QObject::connect(btn3, &QPushButton::clicked, this, [this](int){
        generate_csv();
    });
    QObject::connect(btn4, &QPushButton::clicked, this, [this](int){
        generate_sql();
    });
    QObject::connect(btn5, &QPushButton::clicked, this, [this](int){
        import_json();
    });
    layout->addWidget(dumpWidget);
}

MockTable* MainWindow::add_table() {
    // add table and vboxlayout
    MockTable* tbl = new MockTable{this};
    centralWidget()->layout()->addWidget(tbl);
    tables.append(tbl);
    QObject::connect(tbl->delete_btn(), &QPushButton::clicked, this, [this, tbl](bool){
        tables.removeOne(tbl);
        delete tbl;
    });
    return tbl;
}

void MainWindow::dump_to_json() {
    QString filename = m_schema_name->text() + ".json";
    QJsonDocument doc{};
    QJsonObject mainObj{};
    QJsonArray tables{};
    for (const auto* tbl : this->tables) {
        tables.append(tbl->to_json());
    }
    mainObj.insert("tables", tables);
    doc.setObject(mainObj);
    QFile file{filename};
    file.open(QFile::OpenModeFlag::WriteOnly);
    file.write(doc.toJson());
    file.close();
}

void MainWindow::generate_csv() {
    dump_to_json();
    QProcess proc{};
    QStringList args;
    args << "mockDbGenerator.py" << "--file" << (m_schema_name->text() + ".json");
    args << "--csv";
    proc.start("py", args);
    if (!proc.waitForFinished()) {
        qDebug() << "Proces failed";
    };
    QMessageBox::information(this, "Command result", proc.readAllStandardOutput());
}

enum class SQLDialect {
    Oracle,
    Postgres
};

void MainWindow::generate_sql() {
    QVector<QString> valid_dialects {
        "Oracle", "PostgreSQL"
    };
    dump_to_json();
    QDialog* dialog = new QDialog{this};
    QObject::connect(dialog, &QDialog::finished, this, [this](int d) {
        SQLDialect dl = static_cast<SQLDialect>(d);
        QProcess proc{};
        QStringList args;
        args << "mockDbGenerator.py" << "--file" << (m_schema_name->text() + ".json");
        args << "--sql";
        if (dl == SQLDialect::Oracle) {
            args << "--dialect" << "oracle";
        } else {
            args << "--dialect" << "postgres";
        }
        proc.start("py", args);
        if (!proc.waitForFinished()) {
            qDebug() << "Process failed";
        }
        auto error = proc.readAllStandardError();
        auto output = proc.readAllStandardOutput();
        if (error.length() > 0) {
            QMessageBox::information(this, "Command exited with text on stderr", "STDERR: " + error + "\nSTDOUT: " + output);
        } else {
            QMessageBox::information(this, "Command result", output);
        }
    });
    dialog->setWindowTitle("Choose dialect");
    QVBoxLayout* dialog_layout = new QVBoxLayout;
    QVBoxLayout* central_widget_layout = new QVBoxLayout;
    QWidget* central_widget = new QWidget{};
    central_widget->setLayout(central_widget_layout);
    dialog_layout->addWidget(central_widget);
    dialog->setLayout(dialog_layout);
    for (const auto& dialect : valid_dialects) {
        QPushButton* btn = new QPushButton{dialect, central_widget};
        QObject::connect(btn, &QPushButton::pressed, this, [dialect, dialog](){
            if (dialect == "Oracle") {
                dialog->done(static_cast<int>(SQLDialect::Oracle));
            } else {
                dialog->done(static_cast<int>(SQLDialect::Postgres));
            }
        });
        central_widget->layout()->addWidget(btn);
    }
    dialog->open();


}
void MainWindow::import_json() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    QDir::currentPath(),
                                                    tr("JSON (*.json)"));
    if (fileName.size() == 0) {
        return;
    }
    auto schemaname = QFileInfo{fileName}.baseName();
    m_schema_name->setText(schemaname);
    QFile file{fileName};
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    auto str = file.readAll();
    auto schema = QJsonDocument::fromJson(str);
    const auto& tablesArray = schema["tables"];
    if (!tablesArray.isArray()) {
        return;
    }
    const auto& tablesArrayRef = tablesArray.toArray();
    for (const auto& tbl : tablesArrayRef) {
        parse_json_table(tbl);
    }
}

void MainWindow::parse_json_table(const QJsonValue& jtbl) {
    MockTable* tbl = add_table();
    const auto& jtableName = jtbl["name"];
    const auto& jquantity = jtbl["rows"];
    const auto& jattributesArray = jtbl["attributes"];
    const auto& jprimaryKeys = jtbl["primary_keys"];
    if (!jtableName.isString() || !jattributesArray.isArray() || !jprimaryKeys.isArray()) {
        // error
        return;
    }
    auto tableName = jtableName.toString();
    auto rowNumber = jquantity.isDouble() ? static_cast<int>(jquantity.toDouble()) : jquantity.toString().toInt();
    auto attributes = jattributesArray.toArray();
    if (attributes.size() > 0) {
        tbl->setAttributesVisible();
    }
    auto primaryKeys = jprimaryKeys.toArray();
    QVector<QString> primaryKeyNames{};
    primaryKeyNames.reserve(primaryKeys.size());
    std::transform(primaryKeys.begin(), primaryKeys.end(), std::back_inserter(primaryKeyNames), [](const auto& jobj) {
        if (jobj.isString()) {
            return jobj.toString();
        }
        return QString{"Invalid attribute name"};
    });
    tbl->setName(tableName);
    tbl->setRowNumber(rowNumber);
    for (const auto& jattr : attributes) {
        if (!jattr.isObject()) {
            continue;
        }
        const auto& attr = jattr.toObject();
        const auto& jattrName = attr["name"];
        const auto& jattrType = attr["type"];
        if (!jattrName.isString() || !jattrType.isString() )
            continue;
        auto attrName = jattrName.toString();
        auto attrType = jattrType.toString().toUpper();
        MockAttribute* wattr = tbl->add_attribute();
        // set props
        if (primaryKeyNames.contains(attrName)) {
            wattr->set_pk();
        }
        wattr->setName(attrName);
        if (attrType == "FOREIGN_KEY") {
            wattr->set_fk();
            const auto& jrefs = attr["references"];
            if (!jrefs.isObject())
                continue;
            const auto& refs = jrefs.toObject();
            wattr->setRefTable(refs["table"].toString());
            wattr->setRefAttr(refs["attribute"].toString());
        } else {
            const auto& jgenType = attr["generation"];
            if (!jgenType.isString())
                continue;
            auto genType = jgenType.toString().toUpper();
            if (attrType == "INTEGER") {
                wattr->setAttrType(MockAttribute::AttributeType::Integer);
                wattr->setStart(attr["start"].isString() ? attr["start"].toString() : "0");
                wattr->setStep(attr["step"]);
            } else if (attrType == "STRING") {
                wattr->setAttrType(MockAttribute::AttributeType::String);
                wattr->setLength(attr["length"].toString());
            } else if (attrType == "REAL") {
                wattr->setAttrType(MockAttribute::AttributeType::Real);
                wattr->setStart(attr["start"].isString() ? attr["start"].toString() : "0");
                wattr->setStep(attr["step"]);
            } else if (attrType == "DATE") {
                wattr->setAttrType(MockAttribute::AttributeType::Date);
                wattr->setStart(attr["start"].isString() ? attr["start"].toString() : "0");
                wattr->setStep(attr["step"]);
            }
            if (genType == "RANDOM") {
                wattr->setGenType(MockAttribute::GenerationType::Random);
            } else if (genType == "INCREASING") {
                wattr->setGenType(MockAttribute::GenerationType::Increasing);
            } else if (genType == "DECREASING") {
                wattr->setGenType(MockAttribute::GenerationType::Decreasing);
            } else if (genType == "REPEATING") {
                wattr->setGenType(MockAttribute::GenerationType::Repeating);
            } else if (genType == "NAMESURNAME") {
                wattr->setGenType(MockAttribute::GenerationType::NameSurname);
            } else if (genType == "EMAIL") {
                wattr->setGenType(MockAttribute::GenerationType::Email);
            } else if (genType == "PHONE") {
                wattr->setGenType(MockAttribute::GenerationType::Phone);
            } else if (genType == "NATURALTEXT") {
                wattr->setGenType(MockAttribute::GenerationType::NaturalText);
            }
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

