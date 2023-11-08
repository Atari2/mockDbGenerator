from structureReader.reader import parse_json_schema, InvalidSchema

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate SQL and CSV from JSON Schema")
    parser.add_argument("-f", "--file", help="JSON Schema file", required=True)
    parser.add_argument("-c", "--csv", help="Generate CSV", action="store_true")
    parser.add_argument("-s", "--sql", help="Generate SQL", action="store_true")
    parser.add_argument("-d", "--dialect", help="SQL dialect (supported are oracle and postgres)", default="postgres")
    args = parser.parse_args()
    VALID_DIALECTS = ["oracle", "postgres"]
    if args.sql and not args.dialect in VALID_DIALECTS:
        print(f"Invalid dialect {args.dialect} valid dialects are {', '.join(VALID_DIALECTS)}")
        return
    if args.dialect in VALID_DIALECTS and not args.sql:
        print(f"Cannot specify dialect without sql generation")
        return
    try:
        schema = parse_json_schema(args.file)
    except Exception as exc:
        print(f"{exc}")
        return
    print(f"Schema {args.file} was valid")
    if args.csv:
        schema.generate_csv()
    if args.sql:
        schema.generate_sql(args.dialect)


if __name__ == "__main__":
    main()
else:
    __all__ = ["parse_json_schema", "InvalidSchema"]