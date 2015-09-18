#!/usr/bin/env python

from xml.dom import minidom
from vsm_test_parser import Logger, Parser
import subprocess
import argparse
import os
import re

_defLaunchArgs = ["--report_format=XML",
                  "--catch_system_errors=no",
                  "--log_level=test_suite",
                  "--report_level=detailed",
                 ]

log = Logger()

def _checkIfBinExists(binary):
    # Check if binary is accessible through PATH env and with no additional paths
    paths = [s + "/" for s in os.environ["PATH"].split(os.pathsep)]
    paths.append('')
    existsInPaths = any([os.path.isfile(path + binary) and os.access(path + binary, os.X_OK)
                  for path in paths])

    if not existsInPaths:
        log.error(binary + " NOT FOUND.")

    return existsInPaths



def launchTest(cmd=[], externalToolCmd=[], parsing=True):
    """Default function used to launch test binary.

    Creates a new subprocess and parses it's output
    """
    if not _checkIfBinExists(cmd[0]):
        return
    if externalToolCmd and not _checkIfBinExists(externalToolCmd[0]):
        return

    log.info("Starting " + cmd[0] + " ...")

    if parsing:
        parser = Parser()
        commandString = " ".join(externalToolCmd + cmd + _defLaunchArgs)
        log.info("Invoking `" + commandString + "`")
        p = subprocess.Popen(externalToolCmd + cmd + _defLaunchArgs,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        testResult = parser.parseOutputFromProcess(p)
        if testResult != "":
            domResult = minidom.parseString(testResult)
            log.XMLSummary(domResult)
            log.failedTestSummary(cmd[0])
        if p.returncode < 0:
            log.terminatedBySignal(" ".join(cmd), -p.returncode)
    else:
        # Launching process without coloring does not require report in XML form
        # Avoid providing --report_format=XML, redirect std* by default to system's std*
        commandString = " ".join(externalToolCmd + cmd + _defLaunchArgs[1:])
        log.info("Invoking `" + commandString + "`")
        p = subprocess.Popen(externalToolCmd + cmd + _defLaunchArgs[1:])
        p.wait()

    log.info(cmd[0] + " finished.")



_valgrindCmd = ["valgrind"]
_gdbCmd = ["gdb", "--args"]

def main():
    argparser = argparse.ArgumentParser(description="Test binary launcher for vasum.")
    group = argparser.add_mutually_exclusive_group()
    group.add_argument('--valgrind', action='store_true',
                        help='Launch test binary inside Valgrind (assuming it is installed).')
    group.add_argument('--gdb', action='store_true',
                        help='Launch test binary with a tool specified by $VSM_DEBUGGER variable. '
                            +'Defaults to gdb.')
    argparser.add_argument('binary', nargs=argparse.REMAINDER,
                        help='Binary to be launched using script.')

    args = argparser.parse_known_args()

    if args[0].binary:
        if args[0].gdb:
            debuggerVar = os.getenv("VSM_DEBUGGER")
            if (debuggerVar):
                _customDebuggerCmd = debuggerVar.split()
            else:
                _customDebuggerCmd = _gdbCmd
            launchTest(args[0].binary, externalToolCmd=_customDebuggerCmd + args[1], parsing=False)
        elif args[0].valgrind:
            launchTest(args[0].binary, externalToolCmd=_valgrindCmd + args[1])
        else:
            launchTest(args[0].binary, parsing=True)
    else:
        log.info("Test binary not provided! Exiting.")



if __name__ == "__main__":
    main()
