# mockDbGenerator
A simple tool to generate mock data for your database.
It is currently in its very early stages of development and is **definitely** not ready for any kind of serious use.

I made this is an afternoon to help me generate data to run queries for my university course to see if they made any sense.

To use it, you need to have a JSON file which specifies the tables and attributes you want to generate. You can simply run `py mockDbGenerator.py -f <file>.json [-c] [-s]`. 

The data can be outputted in 2 different formats: CSV and SQL. The SQL format is currently only supported for Oracle SQL. 

By default the program will only parse the schema and not generate any data. 

To generate the data you need to pass the -c flag for CSV and -s for SQL. Both can be passed at the same time.

When generating CSV data, a folder will be created with the schema name and a single CSV file for each table will be created inside it.

When generating SQL data, a single file with schema_name.sql will be created. This file will contain all the SQL statements to create the tables and insert the data.
## JSON Specification for mockDbGenerator
The JSON spec currently only needs a top level object named `tables` which contains an array of table objects.
Each table object must have the following properties:
* `name` - The name of the table
* `attributes` - An array of attribute objects
* `rows` - The number of rows to generate for this table
Each table object can also have the following optional properties:
* `primary_keys` - A primary key array of attribute names

Each attribute object must have the following properties:
* `name` - The name of the attribute
* `type` - The type of the attribute

Each attribute object can also have the following optional properties:
* `generation` - The algorithm to use to generate the values for this attribute
* `start` - The starting value for the attribute
* `step` - The step value for the attribute, this is also used for the increment/decrement value for the `increment` generation type
* `length` - The length of the attribute, this is used for the `string` type to indicate their length

4 types can currently be generated:
* `string` - A string of random alphanumeric characters
* `integer` - An integer
* `float` - A float
* `date` - A timestamp without timezone

Additionally, an attribute can also be indicated of type `foreign_key` which will generate a foreign key to another table. `foreign_key` attributes must have a property called `references` which is an object with the following properties:
* `table` - The name of the table to reference
* `attribute` - The name of the attribute to reference

The following generation types can be used:
* `random` - A random value is generated for each row (this is the default and it valid for all types)
* `increment` - The value is incremented by the step value for each row (this is not valid for string types)
* `decrement` - The value is decremented by the step value for each row (this is not valid for string types)
* `repeating` - The values are generated and then repeated every `step` times (this is not valid for date types)

The `date` type needs to be accurately specified `start` and `step` values. The `start` value must be a string in the format `YYYY-MM-DD` and the `step` value must be an object which can have the following properties:
* `days`
* `seconds`
* `microseconds`
* `milliseconds`
* `minutes`
* `hours`
* `weeks`

Any combination of the properties is valid. If left empty, the default is `days: 1`.


