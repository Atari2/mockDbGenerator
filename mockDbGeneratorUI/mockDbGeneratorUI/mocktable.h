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
    QLineEdit* nameWidget{nullptr};
    QLineEdit* rowsWidget{nullptr};
public:
    explicit MockTable(QWidget *parent = nullptr);
    MockAttribute* add_attribute();
    QJsonObject to_json() const;
    QPushButton* delete_btn() { return deleteBtn; }
    void setName(const QString& str) { name = str; nameWidget->setText(name); }
    void setRowNumber(int rowNumber) { rows = rowNumber; rowsWidget->setText(QString::number(rowNumber)); }
    void setAttributesVisible() { tblAttrNamesWidget->setVisible(true); }
signals:
};

#endif // MOCKTABLE_H
