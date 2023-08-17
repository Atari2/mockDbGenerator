#include "mockattribute.h"
#include "qcombobox.h"
#include <QLabel>
using GT = MockAttribute::GenerationType;
using AT = MockAttribute::AttributeType;

template <typename Enum>
requires std::is_enum_v<Enum>
auto down(Enum e) {
    return static_cast<std::underlying_type_t<Enum>>(e);
}
template <typename Enum>
requires std::is_enum_v<Enum>
auto up(std::underlying_type_t<Enum> e) {
    return static_cast<Enum>(e);
}

static bool type_gen_compatible(GT gen, AT type) {
    switch (type) {
    case AT::Integer:
    case AT::Real:
        return true;
    case AT::String:
        return gen == GT::Random || gen == GT::Repeating;
    case AT::Date:
        return gen != GT::Repeating;
    }
    return false;
}

static GT convert_gbox_idx_to_type(int index, AT type) {
    if (type != AT::String) {
        // for integers and real conversion is direct
        // since they're all valid
        return up<GT>(index);
    } else {
        // string type is only valid with random and repeating
        return index == 0 ? GT::Random : GT::Repeating;
    }
}


// normal indexes for tbox
// 0 => Integer
// 1 => Real
// 2 => String
// 3 => Date
static AT convert_tbox_idx_to_type(int index, GT type) {
    if (type == GT::Increasing || type == GT::Decreasing) {
        // increment and decrement don't support strings.
        // which means that index 2 now maps to date
        return index == 2 ? AT::Date : up<AT>(index);
    } else {
        // Random and Repeating indexes map normally
        return up<AT>(index);
    }
}

static int correct_gbox_index(GT gen, AT type) {
    if (type == AT::String) {
        return gen == GT::Random ? 0 : 1;
    } else {
        return down(gen);
    }
}

static int correct_tbox_index(GT gen, AT type) {
    if (gen == GT::Increasing || gen == GT::Decreasing) {
        return type == AT::Date ? 2 : down(type);
    } else {
        return down(type);
    }
}


template <typename Enum>
requires std::is_enum_v<Enum>
struct EnumIterator {
    using Ret = std::underlying_type_t<Enum>;
    Ret m_value;
    EnumIterator(Enum v) : m_value{down(v)} {}
    EnumIterator& operator++() { ++m_value; return *this; }
    Enum operator*() const { return static_cast<Enum>(m_value); }
    bool operator!=(const EnumIterator& other) const { return m_value != other.m_value; }
};

template <typename Enum>
requires std::is_enum_v<Enum>
struct EnumForEach {
    auto begin() const { return EnumIterator<Enum>{Enum{}}; }
    auto end() const { return EnumIterator<Enum>{static_cast<Enum>(QMetaEnum::fromType<Enum>().keyCount())}; }
};

template <typename Enum>
requires std::is_enum_v<Enum>
auto for_each_enum() {
    return EnumForEach<Enum>{};
}


MockAttribute::MockAttribute(QString attr_name, int row, QGridLayout* hl, QWidget *tbl)
    : QWidget{tbl}, m_name{attr_name}
{
    // name, type, key type, generation, start, step,
    // length, ref. table, ref. attr
    delete_button = new QPushButton{"Delete attr.", tbl};
    // name
    name_edit = new QLineEdit{this};
    name_edit->setText(m_name);
    QObject::connect(name_edit, &QLineEdit::editingFinished, this, [this] (){
        m_name = name_edit->text();
    });
    tbox = new QComboBox{this};
    kbox = new QComboBox{this};
    gbox = new QComboBox{this};
    // type
    for (auto v : for_each_enum<AT>()) {
        tbox->addItem(enum_to_string(v));
    }
    // primary key, foreign key, none
    for (auto v : for_each_enum<KeyType>()) {
        kbox->addItem(enum_to_string(v));
    }
    // generation
    for (auto v : for_each_enum<GT>()) {
        gbox->addItem(enum_to_string(v));
    }
    QObject::connect(tbox, &QComboBox::currentIndexChanged, this, [this](int index){
        AT val = convert_tbox_idx_to_type(index, m_gen_type);
        m_attr_type = val;
        if (m_attr_type == AT::String) {
            length->setEnabled(true);
        }
        const bool isDate = m_attr_type == AT::Date;
        start_date->setHidden(!isDate);
        start->setHidden(isDate);
        step_date->setHidden(!isDate);
        step->setHidden(isDate);
        if (!type_gen_compatible(m_gen_type, m_attr_type)) {
            m_gen_type = GT::Random;
        }
        gbox->blockSignals(true);
        gbox->clear();
        for (auto v : for_each_enum<GT>()) {
            if (type_gen_compatible(v, m_attr_type)) {
                gbox->addItem(enum_to_string(v));
            }
        }
        gbox->setCurrentIndex(correct_gbox_index(m_gen_type, m_attr_type));
        gbox->blockSignals(false);
    });
    QObject::connect(gbox, &QComboBox::currentIndexChanged, this, [this](int index){
        GT val = convert_gbox_idx_to_type(index, m_attr_type);
        m_gen_type = val;
        if (!type_gen_compatible(m_gen_type, m_attr_type)) {
            m_attr_type = AT::Integer;
        }
        tbox->blockSignals(true);
        tbox->clear();
        for (auto v : for_each_enum<AT>()) {
            if (type_gen_compatible(m_gen_type, v)) {
                tbox->addItem(enum_to_string(v));
            }
        }
        tbox->setCurrentIndex(correct_tbox_index(m_gen_type, m_attr_type));
        tbox->blockSignals(false);

    });
    QObject::connect(kbox, &QComboBox::currentIndexChanged, this, [this](int index) {
        KeyType v = up<KeyType>(index);
        if (v == m_key_type)
            return;
        tbox->blockSignals(true);
        gbox->blockSignals(true);
        if (v == KeyType::ForeignKey) {
            tbox->setDisabled(true);
            gbox->setDisabled(true);
            start->setDisabled(true);
            step->setDisabled(true);
            length->setDisabled(true);
            ref_table->setEnabled(true);
            ref_attr->setEnabled(true);
            m_key_type = v;
            tbox->clear();
            gbox->clear();
        } else if (m_key_type == KeyType::ForeignKey) {
            tbox->setDisabled(false);
            gbox->setDisabled(false);
            start->setDisabled(false);
            step->setDisabled(false);
            length->setDisabled(false);
            ref_table->setEnabled(false);
            ref_attr->setEnabled(false);
            m_key_type = v;
            for (auto v : for_each_enum<AT>()) {
                if (type_gen_compatible(m_gen_type, v)) {
                    tbox->addItem(enum_to_string(v));
                }
            }
            for (auto v : for_each_enum<GT>()) {
                if (type_gen_compatible(v, m_attr_type)) {
                    gbox->addItem(enum_to_string(v));
                }
            }
        } else {
            m_key_type = v;
        }
        tbox->blockSignals(false);
        gbox->blockSignals(false);
    });
    // start
    QWidget* start_container = new QWidget{this};
    QHBoxLayout* start_cont_layout = new QHBoxLayout;
    start_container->setLayout(start_cont_layout);
    start = new QLineEdit{start_container};
    start->setText("0");
    start->setValidator(new QIntValidator{start});
    start_date = new QDateEdit{start_container};
    start_date->setHidden(true);
    start_cont_layout->addWidget(start);
    start_cont_layout->addWidget(start_date);

    // step
    QWidget* step_container = new QWidget{this};
    QHBoxLayout* step_cont_layout = new QHBoxLayout;
    step_container->setLayout(step_cont_layout);
    step = new QLineEdit{step_container};
    step->setText("1");
    step->setValidator(new QIntValidator{start});
    step_date = new QWidget{step_container};
    QGridLayout* dateStepLayout = new QGridLayout;
    step_date->setLayout(dateStepLayout);
    std::array<QString, 7> dateStepProps{"microseconds", "milliseconds", "seconds", "minutes", "hours", "days", "weeks"};
    int row_inner = 0;
    for (const QString& prop : dateStepProps) {
        QLabel* label = new QLabel{step_date};
        label->setText(prop);
        QLineEdit* edit = new QLineEdit{step_date};
        edit->setText(prop == "days" ? "1" : "0");
        edit->setValidator(new QIntValidator{edit});
        dateStepLayout->addWidget(label, row_inner, 0);
        dateStepLayout->addWidget(edit, row_inner, 1);
        ++row_inner;
    }
    step_date->setHidden(true);
    step_cont_layout->addWidget(step);
    step_cont_layout->addWidget(step_date);

    // length
    length = new QLineEdit{this};
    length->setText("10");
    length->setValidator(new QIntValidator{start});
    length->setEnabled(false);
    // references table
    // references attribute
    ref_table = new QLineEdit{this};
    ref_attr = new QLineEdit{this};
    ref_table->setDisabled(true);
    ref_attr->setDisabled(true);

    hl->addWidget(name_edit, row, 0);
    hl->addWidget(tbox, row, 1);
    hl->addWidget(kbox, row, 2);
    hl->addWidget(gbox, row, 3);
    hl->addWidget(start_container, row, 4);
    hl->addWidget(step_container, row, 5);
    hl->addWidget(length, row, 6);
    hl->addWidget(ref_table, row, 7);
    hl->addWidget(ref_attr, row, 8);
    hl->addWidget(delete_button, row, 9);
}

QJsonObject MockAttribute::to_json() const {
    std::array<QString, 7> dateStepProps{ "microseconds", "milliseconds", "seconds", "minutes", "hours", "days", "weeks"};
    QJsonObject obj{};
    obj.insert("name", m_name);
    if (m_key_type == KeyType::ForeignKey) {
        obj.insert("type", "foreign_key");
        QJsonObject references{};
        references.insert("table", ref_table->text());
        references.insert("attribute", ref_attr->text());
        obj.insert("references", references);
        return obj;
    }
    obj.insert("type", enum_to_string(m_attr_type));
    obj.insert("generation", enum_to_string(m_gen_type));
    if (m_attr_type == AT::Date) {
        auto date = start_date->date();
        QString date_format = QString::asprintf("%04d-%02d-%02d", date.year(), date.month(), date.day());
        obj.insert("start", date_format);
    } else {
        obj.insert("start", start->text());
    }
    if (m_attr_type == AT::Date) {
        QJsonObject date_step_obj{};
        int row = 0;
        for (const auto& key : dateStepProps) {
            auto* w = static_cast<QLineEdit*>(static_cast<QGridLayout*>(step_date->layout())->itemAtPosition(row++, 1)->widget());
            date_step_obj.insert(key, w->text().toInt());
        }
        obj.insert("step", date_step_obj);
    } else {
       obj.insert("step", step->text());
    }
    if (m_attr_type == AT::String) {
        obj.insert("length", length->text());
    }
    return obj;
}

void MockAttribute::setGenType(GenerationType type) {
    gbox->setCurrentIndex(correct_gbox_index(type, m_attr_type));
}
void MockAttribute::setStart(const QString& start) {
    if (m_attr_type == AttributeType::Date) {
        start_date->setDate(QDate::fromString(start, Qt::DateFormat::ISODate));
    } else {
        this->start->setText(start);
    }
}
void MockAttribute::setStep(const QJsonValue& step) {
    if (m_attr_type == AttributeType::Date && step.isObject()) {
        const auto& stepObj = step.toObject();
        auto* dlayout = static_cast<QGridLayout*>(step_date->layout());
        std::array<std::pair<QString, size_t>, 7> keys{
            {{"days", 5}, {"seconds", 3}, {"microseconds", 0}, {"milliseconds", 1}, {"minutes", 2}, {"hours", 4}, {"weeks", 6}}
        };
        for (const auto& [str, index] : keys) {
            auto* item = dlayout->itemAtPosition(index, 1);
            auto* dateEdit = static_cast<QLineEdit*>(item->widget());
            if (stepObj.contains(str)) {
                dateEdit->setText(stepObj[str].toString());
            } else {
                dateEdit->setText("0");
            }
        }
    } else if (step.isString()) {
        this->step->setText(step.toString());
    }
}
void MockAttribute::setLength(const QString& length) {

}
