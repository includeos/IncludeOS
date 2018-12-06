#!/usr/bin/python3

import pygsheets
import sys
import csv

def read_csv(in_filename):

    with open(in_filename) as file:
        reader = csv.reader(file, delimiter=',', quotechar='|')

        rows = []
        for row in reader:
            rows.append(row)
        return rows

def main(in_filename, out_filename):

    gc = pygsheets.authorize(
        outh_file="client_secret.json",
        outh_nonlocal=True)

    all_sheets = gc.list_ssheets()
    all_names = [sheet['name'] for sheet in all_sheets]

    sheet_name = None

    if sheet_name is not None:
        gc.create(sheet_name)
        print sheet_name
    elif out_filename is not None and sheet_name is None:
        sheet_name = out_filename
    else:
        while True:
            sys.stdout.write("Name of Google Sheet: ")
            sheet_name = input().lower()
            if sheet_name != "" and sheet_name in all_names:
                break
            else:
                sys.stdout.write(
                    "Please respond with the name of the Google Sheet.\n")
    sh = gc.open(sheet_name)
    wks = sh.sheet1

    read_from_file = read_csv(in_filename)
    for row in read_from_file:
        rows = len(wks.get_col(2, returnas='cell', include_empty=False))
        wks.append_table(start=("A" + str(rows + 1)), end=None, values=row)


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Error: Wrong number of arguments\nUsage: python auth.py <CSV filename> <Google spreadsheet name>")
    elif len(sys.argv) == 2:
        # Input file: filename.csv, Output file: gsheet name
        main(sys.argv[1], None)
    else:
        # Input file: filename.csv, Output file: gsheet name
        main(sys.argv[1], sys.argv[2])
