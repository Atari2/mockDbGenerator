#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "mocktable.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    MockTable* add_table();
    void dump_to_json();
    void generate_sql();
    void generate_csv();
    void import_json();
    void parse_json_table(const QJsonValue& obj);
private:
    Ui::MainWindow *ui;
    QVector<MockTable*> tables;
    QLineEdit* m_schema_name{};

};
#endif // MAINWINDOW_H
