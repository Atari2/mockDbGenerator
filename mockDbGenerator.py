from structureReader.reader import parse_json_schema, InvalidSchema

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate SQL and CSV from JSON Schema")
    parser.add_argument("-f", "--file", help="JSON Schema file", required=True)
    parser.add_argument("-c", "--csv", help="Generate CSV", action="store_true")
    parser.add_argument("-s", "--sql", help="Generate SQL", action="store_true")
    args = parser.parse_args()
    try:
        schema = parse_json_schema(args.file)
    except Exception as exc:
        print(f"{exc}")
        return
    print(f"Schema {args.file} was valid")
    if args.csv:
        schema.generate_csv()
    if args.sql:
        schema.generate_sql()


if __name__ == "__main__":
    main()
else:
    __all__ = ["parse_json_schema", "InvalidSchema"]