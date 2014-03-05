#!/usr/bin/env python

from xml.dom import minidom
from sc_test_parser import Logger, Parser
import subprocess
import argparse
import os

_defLaunchArgs = ["--report_format=XML",
                  "--catch_system_errors=no",
                  "--log_level=test_suite",
                  "--report_level=detailed",
                 ]

log = Logger()

def _checkIfBinExists(binary):
    paths = [s + "/" for s in os.environ["PATH"].split(os.pathsep)]
    exists = any([os.path.isfile(path + binary) and os.access(path + binary, os.X_OK)
                  for path in paths])

    if not exists:
        log.error(binary + " NOT FOUND.")

    return exists



def launchTest(cmd=[], externalToolCmd=[], parsing=True):
    """Default function used to launch test binary.

    Creates a new subprocess and parses it's output
    """
    if not _checkIfBinExists(cmd[0]):
        return
    if externalToolCmd and not _checkIfBinExists(externalToolCmd[0]):
        return

    log.info("Starting " + cmd[0] + "...")

    if parsing:
        parser = Parser()
        p = subprocess.Popen(" ".join(externalToolCmd + cmd + _defLaunchArgs),
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
        testResult = parser.parseOutputFromProcess(p)
        if testResult != "":
            domResult = minidom.parseString(testResult)
            log.XMLSummary(domResult)
    else:
        # Launching process without coloring does not require report in XML form
        # Avoid providing --report_format=XML, redirect std* by default to system's std*
        p = subprocess.Popen(" ".join(externalToolCmd + cmd + _defLaunchArgs[1:]),
                             shell=True)
        p.wait()

    log.info(cmd[0] + " finished.")



_valgrindCmd = ["valgrind"]
_gdbCmd = ["gdb", "--args"]

def main():
    argparser = argparse.ArgumentParser(description="Test binary launcher for security-containers.")
    group = argparser.add_mutually_exclusive_group()
    group.add_argument('--valgrind', action='store_true',
                        help='Launch test binary inside Valgrind (assuming it is installed).')
    group.add_argument('--gdb', action='store_true',
                        help='Launch test binary with GDB (assuming it is installed).')
    argparser.add_argument('binary', nargs=argparse.REMAINDER,
                        help='Binary to be launched using script.')

    args = argparser.parse_known_args()

    if args[0].binary:
        if args[0].gdb:
            launchTest(args[0].binary, externalToolCmd=_gdbCmd + args[1], parsing=False)
        elif args[0].valgrind:
            launchTest(args[0].binary, externalToolCmd=_valgrindCmd + args[1])
        else:
            launchTest(args[0].binary, parsing=True)
    else:
        log.info("Test binary not provided! Exiting.")



if __name__ == "__main__":
    main()
