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
    void set_pk() { kbox->setCurrentIndex(static_cast<std::underlying_type_t<KeyType>>(KeyType::PrimaryKey)); }
    void set_fk() { kbox->setCurrentIndex(static_cast<std::underlying_type_t<KeyType>>(KeyType::ForeignKey)); }
    void setAttrType(AttributeType type) { tbox->setCurrentIndex(static_cast<std::underlying_type_t<AttributeType>>(type)); }
    void setGenType(GenerationType type);
    void setStart(const QString& start);
    void setStep(const QJsonValue& step);
    void setLength(const QString& length);
    void setName(const QString& name) { m_name = name; name_edit->setText(m_name); }
    void setRefTable(const QString& tblName) { ref_table->setText(tblName); }
    void setRefAttr(const QString& tblAttr) { ref_attr->setText(tblAttr); }
    const QString& name() const { return m_name; }
    QPushButton* delete_btn() { return delete_button; }
signals:

};

#endif // MOCKATTRIBUTE_H
