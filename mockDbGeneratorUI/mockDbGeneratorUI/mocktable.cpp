#include "mocktable.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>

class GridLayoutUtil {

public:

    // Removes the contents of the given layout row.
    static void removeRow(QGridLayout *layout, int row, bool deleteWidgets = true) {
        remove(layout, row, deleteWidgets);
        layout->setRowMinimumHeight(row, 0);
        layout->setRowStretch(row, 0);
    }

private:

    // Removes all layout items which span the given row and column.
    static void remove(QGridLayout *layout, int row, bool deleteWidgets) {
        // We avoid usage of QGridLayout::itemAtPosition() here to improve performance.
        for (int col = layout->columnCount() - 1; col >= 0; --col) {
            QLayoutItem* item = layout->itemAtPosition(row, col);
            if (item == nullptr) {
                QMessageBox::critical(nullptr, "Internal error",
                                      QString::asprintf("Trying to delete item in grid at row %d and column %d but it returned null.", row, col));
                continue;
            }
            layout->removeItem(item);
            if (deleteWidgets) {
                deleteChildWidgets(item);
            }
            delete item;
        }
    }
    // Deletes all child widgets of the given layout item.
    static void deleteChildWidgets(QLayoutItem *item) {
        QLayout *layout = item->layout();
        if (layout) {
            // Process all child items recursively.
            int itemCount = layout->count();
            for (int i = 0; i < itemCount; i++) {
                deleteChildWidgets(layout->itemAt(i));
            }
        }
        delete item->widget();
    }
};

MockTable::MockTable(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout* tblLayout = new QVBoxLayout;
    setLayout(tblLayout);
    QWidget* tblAttrWidget = new QWidget{this};
    tblAttrNamesWidget = new QWidget{this};
    QHBoxLayout* tblAttrWidgetLayout = new QHBoxLayout;
    QGridLayout* tblAttrGridWidget = new QGridLayout;
    tblAttrWidget->setLayout(tblAttrWidgetLayout);
    tblAttrNamesWidget->setLayout(tblAttrGridWidget);
    const std::array<QString, 9> labelTexts{
        "Name",
        "Type",
        "Key type",
        "Generation",
        "Start",
        "Step",
        "Length",
        "Ref. table",
        "Ref. attr."
    };
    int i = 0;
    for (const auto& text : labelTexts) {
        QLabel* lbl = new QLabel{text, this};
        lbl->setAlignment(Qt::AlignmentFlag::AlignHCenter | Qt::AlignmentFlag::AlignBottom);
        tblAttrGridWidget->addWidget(lbl, 0, i++);
    }
    tblAttrNamesWidget->setVisible(false);
    QLabel* label1 = new QLabel{"Table name:", tblAttrWidget};
    QLineEdit* nameWidget = new QLineEdit{"table_name", tblAttrWidget};
    QLabel* label2 = new QLabel{"Row count:", tblAttrWidget};
    QLineEdit* rowsWidget = new QLineEdit{"100", tblAttrWidget};
    QPushButton* addAttributeButton = new QPushButton{"Add attribute", tblAttrWidget};
    deleteBtn = new QPushButton{"Delete table", parent};
    rowsWidget->setValidator(new QIntValidator{0, 10'000'000, rowsWidget});
    tblAttrWidgetLayout->addWidget(label1);
    tblAttrWidgetLayout->addWidget(nameWidget);
    tblAttrWidgetLayout->addWidget(label2);
    tblAttrWidgetLayout->addWidget(rowsWidget);
    tblAttrWidgetLayout->addWidget(addAttributeButton);
    tblAttrWidgetLayout->addWidget(deleteBtn);
    QObject::connect(addAttributeButton, &QPushButton::clicked, this, [this](bool c){
        if (attributes.empty()) {
            tblAttrNamesWidget->setVisible(true);
        }
        this->add_attribute();
    });
    QObject::connect(nameWidget, &QLineEdit::editingFinished, this, [this, nameWidget](){
        name = nameWidget->text();
    });
    QObject::connect(rowsWidget, &QLineEdit::editingFinished, this, [this, rowsWidget]() {
        rows = rowsWidget->text().toInt();
    });
    layout()->addWidget(tblAttrWidget);
    layout()->addWidget(tblAttrNamesWidget);
}
void MockTable::add_attribute() {
    QGridLayout* gl = static_cast<QGridLayout*>(tblAttrNamesWidget->layout());
    auto sz = static_cast<int>(attributes.size());
    qDebug() << "Adding attribute number " << sz << ", current rows " << gl->rowCount();
    attribute_row_numbers.append(sz + 1);        // attribute will get added at row count()
    MockAttribute* wd = new MockAttribute{"attr_name", static_cast<int>(attributes.size()) + 1, gl, this};
    layout()->addWidget(wd);
    attributes.append(wd);
    qDebug() << "row numbers: " << attribute_row_numbers;
    QObject::connect(wd->delete_btn(), &QPushButton::clicked, this, [this, wd](bool){
        int attrIdx = attributes.indexOf(wd);
        qDebug() << "removing attribute at index " << attrIdx;
        if (attrIdx == -1) {
            QMessageBox::critical(nullptr, "Internal error",
                                  QString::asprintf("Trying to delete attribute %s but it wasn't found in attributes array", wd->name().data()));
            return;
        }
        attributes.removeAt(attrIdx);
        int row = attribute_row_numbers[attrIdx];
        qDebug() << "which corresponds at row " << row;
        attribute_row_numbers.removeAt(attrIdx);
        qDebug() << "remaining rows " << attribute_row_numbers;
        auto* l = static_cast<QGridLayout*>(tblAttrNamesWidget->layout());
        GridLayoutUtil::removeRow(l, row, true);
        delete wd;
        if (attributes.empty()) {
            tblAttrNamesWidget->setVisible(false);
        }
    });
}
QJsonObject MockTable::to_json() const {
    QJsonObject obj{};
    obj.insert("name", name);
    obj.insert("rows", rows);
    QJsonArray jprimary_keys{};
    QJsonArray jattributes{};
    for (const auto* attr : attributes) {
        if (attr->is_pk()) {
            jprimary_keys.append(attr->name());
        }
        jattributes.append(attr->to_json());
    }
    obj.insert("attributes", jattributes);
    obj.insert("primary_keys", jprimary_keys);
    return obj;
}
