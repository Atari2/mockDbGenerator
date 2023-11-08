from __future__ import annotations
from enum import IntEnum
from io import TextIOWrapper
import json
from typeWrappers.types import DbType
from typing import Any
from dataGenerators.generators import value_generator_factory, GenerationMode
from datetime import datetime, timedelta
from csv import DictWriter
import os
import random

class SQLDialect(IntEnum):
    POSTGRES = 1
    ORACLE = 2

map_str_to_type = {
    "INTEGER": DbType.INTEGER,
    "STRING": DbType.STRING,
    "REAL": DbType.REAL,
    "DATE": DbType.DATE,
}
map_str_to_generation_type = {
    "RANDOM": GenerationMode.RANDOM,
    "INCREASING": GenerationMode.INCREASING,
    "DECREASING": GenerationMode.DECREASING,
    "REPEATING": GenerationMode.REPEATING,
    "NAMESURNAME": GenerationMode.NAMESURNAME,
    "EMAIL": GenerationMode.EMAIL,
    "PHONE": GenerationMode.PHONE,
    "NATURALTEXT": GenerationMode.NATURALTEXT,
}


def raise_invalid_type(*args):
    raise TypeError(f"Cannot have a starting value")


def construct_timedelta(object: dict[str, str]):
    keys = [
        "days",
        "seconds",
        "microseconds",
        "milliseconds",
        "minutes",
        "hours",
        "weeks",
    ]
    values = [int(object.get(key, 0)) for key in keys]
    return timedelta(*values)


def make_sql_type_from_type(type: DbType, length: int, dialect: SQLDialect):
    if dialect == SQLDialect.POSTGRES:
        match type:
            case DbType.INTEGER:
                return "INTEGER"
            case DbType.REAL:
                return "REAL"
            case DbType.STRING:
                return f"VARCHAR({length})"
            case DbType.DATE:
                return "DATE"
    elif dialect == SQLDialect.ORACLE:
        match type:
            case DbType.INTEGER | DbType.REAL:
                return "NUMBER"
            case DbType.STRING:
                return f"VARCHAR2({length})"
            case DbType.DATE:
                return "TIMESTAMP"


map_starting_to_cast = {
    DbType.INTEGER: int,
    DbType.REAL: float,
    DbType.DATE: lambda x: datetime.fromisoformat(x),
}
map_step_to_cast = {
    DbType.INTEGER: int,
    DbType.REAL: float,
    DbType.DATE: construct_timedelta,
}


class References:
    table: str | DbTable
    attribute: str | DbAttribute

    def __init__(self, table: str, attribute: str):
        self.table = table
        self.attribute = attribute

    def assign_actual_reference(self, table: DbTable, attribute: DbAttribute):
        self.table = table
        self.attribute = attribute


class InvalidAttribute(Exception):
    def __init__(self, message: str):
        super().__init__(message)


class InvalidTable(InvalidAttribute):
    def __init__(self, message: str, base_exception: InvalidAttribute | None = None):
        if base_exception:
            message += f"\n\t\t\u2514{base_exception}"
        super().__init__(message)


class DbAttribute:
    _type: DbType
    _name: str
    _start: Any  # not valid for strings
    _step: Any  # not valid for strings
    _length: None | int  # valid only for strings
    _generation: GenerationMode
    _data: None | list[Any]
    _references: None | References

    def __init__(self, attribute: dict[str, Any]):
        self._data = None
        self._name = attribute["name"]
        att_type = attribute["type"].upper()
        if att_type == "FOREIGN_KEY":
            if "references" not in attribute:
                raise InvalidAttribute(
                    f"attribute '{self._name}' is invalid because it is a foreign key but no references are given"
                )
            refs = attribute["references"]
            if not isinstance(refs, dict):
                raise InvalidAttribute(
                    f"attribute '{self._name}' is invalid because foreign key references must be an object with keys 'table' and 'attribute'"
                )
            try:
                self._references = References(refs["table"], refs["attribute"])
            except KeyError as exc:
                raise InvalidAttribute(
                    f"attribute '{self._name}' is invalid because its foreign key references is missing {exc} key"
                )
        else:
            self._references = None
            self._type = map_str_to_type[att_type]
        if self._references:
            return
        if self._type != DbType.STRING:
            self._start = map_starting_to_cast[self._type](attribute.get("start", "0"))
            self._step = map_step_to_cast[self._type](
                attribute.get(
                    "step", "1" if self._type != DbType.DATE else {"days": "1"}
                )
            )
        else:
            self._start = None
            self._step = None
        if self._type == DbType.STRING:
            self._length = int(attribute.get("length", "10"))
        else:
            self._length = None
        self._generation = map_str_to_generation_type[
            attribute.get("generation", "RANDOM").upper()
        ]

    @property
    def type(self):
        if self._references and isinstance(self._references.attribute, DbAttribute):
            return self._references.attribute._type
        return self._type

    @property
    def length(self):
        if self._references and isinstance(self._references.attribute, DbAttribute):
            return self._references.attribute._length
        return self._length

    @property
    def data(self):
        return self._data
    
    def _update_length_if_needed(self):
        GENMOD_TO_ADJUST = [GenerationMode.NATURALTEXT, GenerationMode.NAMESURNAME, GenerationMode.EMAIL, GenerationMode.PHONE]
        if self._data is None:
            raise ValueError("Attribute data is None, cannot update length")
        if not hasattr(self, "_generation"):
            return
        if self.type == DbType.STRING and self._generation in GENMOD_TO_ADJUST:
            self._length = max(map(len, self._data))
        else:
            return

    def sql_string(self, dialect: SQLDialect) -> str:
        return f"{self._name} {make_sql_type_from_type(self.type, self.length if self.length else 0, dialect)} NOT NULL"


class ForeignKey:
    _attribute: DbAttribute
    _fk_attribute: DbAttribute

    def __init__(self, attribute: DbAttribute, fk_attribute: DbAttribute):
        self._attribute = attribute
        self._fk_attribute = fk_attribute


class DbTable:
    _name: str
    _attributes: dict[str, DbAttribute]
    _quantity: int
    _keys: list[DbAttribute]

    def __init__(self, table: dict[str, Any]):
        self._name = table["name"]
        self._quantity = int(table["rows"])
        self._attributes = {}
        self._keys = []
        if "attributes" not in table:
            raise InvalidTable(f"table '{self._name}' must have an 'attribute' array")
        for attribute in table["attributes"]:
            try:
                attr_name = attribute["name"]
            except KeyError:
                raise InvalidAttribute(
                    f"table '{self._name}' is invalid because an attribute is missing required key 'name'"
                )
            try:
                self._attributes[attr_name] = DbAttribute(attribute)
            except KeyError as ke:
                raise InvalidTable(
                    f"table '{self._name}' is invalid because attribute '{attr_name}' is missing required key {ke}"
                )
            except InvalidAttribute as exc:
                raise InvalidTable(f"table '{self._name}' is invalid", exc)
        if "primary_keys" in table:
            for key in table["primary_keys"]:
                self._keys.append(self._attributes[key])

    def _get_foreign_attribute(self, attribute: DbAttribute) -> DbAttribute | None:
        if attribute._references and isinstance(
            attribute._references.attribute, DbAttribute
        ):
            return attribute._references.attribute
        return None

    def __hash__(self):
        return hash(self._name)
    
    def _generate_oracle_sql(self, f: TextIOWrapper):
        sql = f"CREATE TABLE {self._name} (\n"
        for attribute in self._attributes.values():
            sql += f"\t{attribute.sql_string(SQLDialect.ORACLE)}, \n"
        if len(self._keys) > 0:
            sql += f'\tCONSTRAINT pk_{self._name} PRIMARY KEY ({", ".join(map(lambda x: x._name, self._keys))})\n);\n'
        else:
            sql = sql[:-3] + "\n);\n"
        f.write(sql) # type: ignore

    def _generate_postgres_sql(self, f: TextIOWrapper):
        sql = f"CREATE TABLE {self._name} (\n"
        for attribute in self._attributes.values():
            sql += f"\t{attribute.sql_string(SQLDialect.POSTGRES)}, \n"
        if len(self._keys) > 0:
            sql += f'\tCONSTRAINT pk_{self._name} PRIMARY KEY ({", ".join(map(lambda x: x._name, self._keys))})\n);\n'
        else:
            sql = sql[:-3] + "\n);\n"
        f.write(sql) # type: ignore

    def generate_sql(self, f: TextIOWrapper, dialect: SQLDialect):
        if dialect == SQLDialect.POSTGRES:
            self._generate_postgres_sql(f)
        elif dialect == SQLDialect.ORACLE:
            self._generate_oracle_sql(f)
        else:
            raise ValueError(f"Invalid dialect {dialect}")

    def generate_insertion_sql(self, f: TextIOWrapper, dialect: SQLDialect):
        for i in range(self._quantity):
            sql = f'INSERT INTO {self._name}({", ".join(self._attributes)}) VALUES ('
            for attribute in self._attributes.values():
                if not attribute.data:
                    raise ValueError(f"Attribute {attribute._name} has no data")
                if attribute.type == DbType.STRING:
                    attribute_str = f"'{attribute.data[i]}'"
                elif attribute.type == DbType.DATE:
                    if dialect == SQLDialect.POSTGRES:
                        attribute_str = f"'{attribute.data[i]}'"
                    elif dialect == SQLDialect.ORACLE:
                        attribute_str = (
                            f"TO_TIMESTAMP('{attribute.data[i]}', 'YYYY-MM-DD HH24:MI:SS')"
                        )
                    else:
                        raise ValueError(f"Invalid dialect {dialect}")
                else:
                    attribute_str = str(attribute.data[i])
                sql += f"{attribute_str}, "
            sql = sql[:-2] + ");\n"
            f.write(sql)

    def _generate_single_attr_data(self, attribute: DbAttribute):
        if fk_attr := self._get_foreign_attribute(attribute):
            if fk_attr.data:
                return fk_attr.data
            gen = value_generator_factory(
                fk_attr.type,
                self._quantity,
                attribute._generation,
                attribute._start,
            ).generate(
                attribute._step
                if attribute.type != DbType.STRING
                else attribute._length
            )
            return list(gen)
        else:
            gen = value_generator_factory(
                attribute.type,
                self._quantity,
                attribute._generation,
                attribute._start,
            ).generate(
                attribute._step
                if attribute.type != DbType.STRING
                else attribute._length
            )
            return list(gen)

    def write_to_csv(self, directory):
        filename = f"{self._name}.csv"
        with open(os.path.join(directory, filename), "w", newline="") as file:
            writer = DictWriter(
                file, fieldnames=[attribute for attribute in self._attributes]
            )
            writer.writeheader()
            for i in range(self._quantity):
                row = {}
                for name, attr in self._attributes.items():
                    if attr.data is None:
                        raise ValueError("Attribute data is None")
                    if attr._references:
                        # if attribute is foreign key, then it's data is the same as
                        # the data of the attribute it references
                        # we index at random to add some variance to the data
                        row[name] = attr.data[random.randint(0, self._quantity - 1)]
                    else:
                        row[name] = attr.data[i]
                writer.writerow(row)

    def generate_data(self, directory: str):
        for attribute in self._attributes.values():
            attribute._data = self._generate_single_attr_data(attribute)
            attribute._update_length_if_needed()


class DbSchema:
    _tables: list[DbTable]
    _name: str

    def __init__(self, name: str, schema: dict[str, Any]):
        self._name = name
        self._tables = []
        for table in schema["tables"]:
            try:
                self._tables.append(DbTable(table))
            except KeyError as e:
                raise InvalidSchema(
                    f"Schema is invalid => table is missing required key: {e}"
                )
            except InvalidTable as e:
                raise InvalidSchema(f"Schema is invalid", e)
        for table in self._tables:
            for attr in filter(lambda x: x._references, table._attributes.values()):
                referenced_table = next(filter(lambda x: x._name == attr._references.table, self._tables))  # type: ignore
                referenced_attribute = referenced_table._attributes[attr._references.attribute]  # type: ignore
                attr._references.assign_actual_reference(referenced_table, referenced_attribute)  # type: ignore
        self._generate_data()

    def _generate_data(self):
        for table in self._tables:
            table.generate_data(self._name)

    def generate_csv(self):
        os.makedirs(self._name, exist_ok=True)
        for table in self._tables:
            table.write_to_csv(self._name)

    def _gen_oracle_foreign_key(self, table: DbTable, foreign_key: DbAttribute, attr: DbAttribute) -> str:
        return f"ALTER TABLE {table._name} ADD CONSTRAINT FOREIGN KEY fk_{table._name} ({foreign_key._name}) REFERENCES {foreign_key._references.table._name}({attr._name});\n" # type: ignore

    def _gen_postgres_foreign_key(self, table: DbTable, foreign_key: DbAttribute, attr: DbAttribute) -> str:
        return f"ALTER TABLE {table._name} ADD CONSTRAINT fk_{table._name} FOREIGN KEY ({foreign_key._name}) REFERENCES {foreign_key._references.table._name}({attr._name}) ON UPDATE NO ACTION ON DELETE NO ACTION;\n" # type: ignore

    def _gen_foreign_key(self, table: DbTable, foreign_key: DbAttribute, attr: DbAttribute, dialect: SQLDialect) -> str:
        if dialect == SQLDialect.POSTGRES:
            return self._gen_postgres_foreign_key(table, foreign_key, attr)
        elif dialect == SQLDialect.ORACLE:
            return self._gen_oracle_foreign_key(table, foreign_key, attr)
        else:
            raise ValueError(f"Invalid dialect {dialect}")

    def generate_sql(self, dialect: str = "postgres"):
        dialect_mapping = {
            "postgres": SQLDialect.POSTGRES,
            "oracle": SQLDialect.ORACLE,
        }
        if dialect not in dialect_mapping:
            raise ValueError(f"Invalid dialect {dialect}")
        sql_dialect = dialect_mapping[dialect]
        with open(f"{self._name}.sql", "w") as f:
            for table in self._tables:
                if sql_dialect == SQLDialect.POSTGRES:
                    f.write(f"DROP TABLE IF EXISTS {table._name} CASCADE;\n") 
                elif sql_dialect == SQLDialect.ORACLE:
                    f.write(f"DROP TABLE {table._name} CASCADE CONSTRAINTS;\n")
                else:
                    raise ValueError(f"Invalid dialect {dialect}")
            for table in self._tables:
                table.generate_sql(f, dialect=sql_dialect)
            for table in self._tables:
                for foreign_key in filter(
                    lambda x: x._references, table._attributes.values()
                ):
                    attr = table._get_foreign_attribute(foreign_key)
                    if attr is None:
                        raise ValueError("Attribute for foreign key is None")
                    if foreign_key._references is None:
                        raise ValueError("Foreign key references is None")
                    if isinstance(foreign_key._references.table, str):
                        raise ValueError("Foreign key references table is string")
                    f.write(
                        self._gen_foreign_key(table, foreign_key, attr, sql_dialect)
                    )
            for table in self._tables:
                table.generate_insertion_sql(f, dialect=sql_dialect)


class InvalidSchema(Exception):
    def __init__(self, message: str, base_exception: InvalidTable | None = None):
        if base_exception:
            space_str = " " * (len(message) - 1)
            message += f"\n\t\u2514{base_exception}"
        super().__init__(message)


def parse_json_schema(filename: str) -> DbSchema:
    schemaname = os.path.splitext(os.path.basename(filename))[0]
    with open(filename, "r") as file:
        schema = json.load(file)
    try:
        return DbSchema(schemaname, schema)
    except InvalidTable as e:
        raise InvalidSchema(f"Schema {filename} is invalid", e)
    except KeyError as v:
        raise InvalidSchema(f"Schema {filename} is invalid⬎\n\tkey {v} is missing")
