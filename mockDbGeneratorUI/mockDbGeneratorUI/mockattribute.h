#ifndef MOCKATTRIBUTE_H
#define MOCKATTRIBUTE_H

#include <QWidget>
#include <QMetaEnum>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QJsonObject>

template <typename QEnum>
QString enum_to_string(const QEnum value) {
    return QString{QMetaEnum::fromType<QEnum>().valueToKey(static_cast<int>(value))};
}

class MockAttribute : public QWidget
{
    Q_OBJECT

public:
    enum class AttributeType {
        Integer,
        Real,
        String,
        Date
    };
    enum class GenerationType {
        Random,
        Increasing,
        Decreasing,
        Repeating
    };
    enum class KeyType {
        None,
        PrimaryKey,
        ForeignKey
    };

    Q_ENUM(AttributeType);
    Q_ENUM(GenerationType);
    Q_ENUM(KeyType);
private:
    QString m_name;
    KeyType m_key_type{KeyType::None};
    GenerationType m_gen_type{GenerationType::Random};
    AttributeType m_attr_type{AttributeType::Integer};
    QComboBox* tbox{};
    QComboBox* gbox{};
    QComboBox* kbox{};
    QLineEdit* name_edit{};
    QLineEdit* start{};
    QDateEdit* start_date{};
    QLineEdit* step{};
    QWidget* step_date{};
    QLineEdit* length{};
    QLineEdit* ref_table{};
    QLineEdit* ref_attr{};
    QPushButton* delete_button{};

public:

    explicit MockAttribute(QString name, int row, QGridLayout* layout, QWidget *parent = nullptr);
    QJsonObject to_json() const;
    bool is_pk() const { return m_key_type == KeyType::PrimaryKey; }
    const QString& name() const { return m_name; }
    QPushButton* delete_btn() { return delete_button; }
signals:

};

#endif // MOCKATTRIBUTE_H
