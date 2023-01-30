#ifndef MOCKTABLE_H
#define MOCKTABLE_H
#include "mockattribute.h"
#include <QWidget>
#include <QMap>
#include <QJsonObject>
class MockTable : public QWidget
{
    Q_OBJECT
    QString name{"table_name"};
    int rows = 100;
    QVector<MockAttribute*> attributes{};
    QVector<int> attribute_row_numbers{};
    QPushButton* deleteBtn{};
    QWidget* tblAttrNamesWidget{};
public:
    explicit MockTable(QWidget *parent = nullptr);
    void add_attribute();
    QJsonObject to_json() const;
    QPushButton* delete_btn() { return deleteBtn; }
signals:
};

#endif // MOCKTABLE_H
