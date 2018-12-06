#!/usr/bin/python
import csv
import datetime
import subprocess
import json
import update
import re
import ntpath
import multiprocessing
import os

#sub_test_stats = subTestStats()

class statOps:
    def __init__(self): # date, #testname #git commit # machinary config
        dt = datetime.datetime.utcnow()
        self.now = dt.strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        self.last_git_commit = subprocess.check_output(['git','log', '--date=iso-strict-local','-n 1','--pretty=format: %h, %ad']).strip()
        self.latest_git_tag = subprocess.check_output(['git', 'describe', '--abbrev=0', '--tags']).strip()
        # git describe --tags $(git rev-list --tags --max-count=1)
        self.test_results = {}
        self.final_time = None

    def clean_csv(self, filename):
        subprocess.call(["rm %s" % filename], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)

    def append_stat_to_list(self, test_name, test_time, test_status, cpu_usage, memory_usage, machine):
        key = ntpath.basename(test_name) + os.path.split(test_name)[0]
        if key not in self.test_results:
            self.test_results[key] = []

        result_list = test_name, test_time[-2:-1], self.now, test_status, cpu_usage, memory_usage, machine
        self.test_results[key].append('%s' % ', '.join(map(str, result_list)))

    def register_all_test_stats(self, filename):
        keys = sorted(self.test_results.keys())
        with open("%s" % filename, "wb+") as csv_file:
            writer = csv.writer(csv_file, delimiter="\t", quoting=csv.QUOTE_NONE)
            writer.writerows(self.test_results.values())

        sheet_name = "Includeos-Sub-Test-Stats"
        update.main(filename, sheet_name)

    # fetch and register test name and time taken by test
    def save_stats_csv(self):
        filename = 'TestOverview_{0}.csv'.format(self.now)
        self.register_all_test_stats(filename)
        self.clean_csv(filename)

    def register_final_stats(self, final_time, test_description, skipped, test_status, fail_count): # name # time (end - start)
        sheet_name = "IncludeOS-Test-Stats" #"IncludeOS-testing-stats"
        filename = "TestStats.csv"
    #    sheet_choice = "sh.sheet1"
        num_cpus = int(multiprocessing.cpu_count())
        machine = os.uname()[3]#.replace(" ", "_")
        total_test_data = [self.now, final_time[:-1], test_description, skipped, test_status, self.latest_git_tag, "%s" % ''.join(self.last_git_commit), num_cpus, machine, fail_count]
        with open("%s" % filename,'wb+') as csv_file:
            writer = csv.writer(csv_file,lineterminator='\n')
            writer.writerow(total_test_data)
        update.main(filename, sheet_name)
        self.clean_csv(filename)
