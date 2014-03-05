from xml.dom import minidom
import sys



BLACK = "\033[90m"
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
MAGENTA = "\033[95m"
CYAN = "\033[96m"
WHITE = "\033[97m"
BOLD = "\033[1m"
ENDC = "\033[0m"



class Logger(object):
    # Create summary of test providing DOM object with parsed XML
    def info(self, msg):
        print BOLD + msg + ENDC

    def infoTitle(self, msg):
        print CYAN + BOLD + msg + ENDC

    def error(self, msg):
        print RED + BOLD + msg + ENDC

    def success(self, msg):
        print GREEN + BOLD + msg + ENDC


    __indentChar = "    "
    def testCaseSummary(self, testName, testResult, recLevel):
        msg = self.__indentChar * recLevel + BOLD + "{:<50}".format(testName + ":")

        if testResult == "passed":
            msg += GREEN
        else:
            msg += RED

        print msg + testResult + ENDC

    def testSuiteSummary(self, suite, recLevel=0, summarize=False):
        indPrefix = self.__indentChar * recLevel

        self.infoTitle(indPrefix + suite.attributes["name"].value + " results:")

        for child in suite.childNodes:
            if child.nodeName == "TestSuite":
                self.testSuiteSummary(child, recLevel=recLevel + 1)
            elif child.nodeName == "TestCase":
                self.testCaseSummary(child.attributes["name"].value,
                                     child.attributes["result"].value,
                                     recLevel=recLevel + 1)

        if summarize:
            self.infoTitle(indPrefix + suite.attributes["name"].value + " summary:")
            self.info(indPrefix + "Passed tests:  " + suite.attributes["test_cases_passed"].value)
            self.info(indPrefix + "Failed tests:  " + suite.attributes["test_cases_failed"].value)
            self.info(indPrefix + "Skipped tests: " + suite.attributes["test_cases_skipped"].value +
                      "\n")

    def XMLSummary(self, dom):
        self.info("\n=========== SUMMARY ===========\n")

        for result in dom.getElementsByTagName("TestResult"):
            for child in result.childNodes:
                if child.nodeName == "TestSuite":
                    self.testSuiteSummary(child, summarize=True)



class Colorizer(object):
    # Add new types of errors/tags for parser here
    lineTypeDict = {'[ERROR]': RED + BOLD,
                    '[WARN ]': YELLOW + BOLD,
                    '[INFO ]': BLUE + BOLD,
                    '[DEBUG]': GREEN,
                    '[TRACE]': BLACK
                    }

    # Looks for lineTypeDict keywords in provided line and paints such line appropriately
    def paintLine(self, line):
        for key in self.lineTypeDict.iterkeys():
            if key in line:
                return self.lineTypeDict[key] + line + ENDC

        return line



class Parser(object):
    _testResultBegin = "<TestResult>"
    _testResultEnd = "</TestResult>"

    colorizer = Colorizer()

    def __parseAndWriteLine(self, line):
        result = ""
        # Entire XML is kept in one line, if line begins with <TestResult>, extract it
        if self._testResultBegin in line:
            result += line[line.find(self._testResultBegin):
                           line.find(self._testResultEnd) + len(self._testResultEnd)]
            line = line[0:line.find(self._testResultBegin)] + line[line.find(self._testResultEnd) +
                                                                   len(self._testResultEnd):]
        sys.stdout.write(str(self.colorizer.paintLine(line)))
        sys.stdout.flush()

        return result


    def parseOutputFromProcess(self, p):
        """Parses stdout from given subprocess p - colors printed lines and looks for test results.
        """
        testResult = ""
        # Dump test results
        while True:
            outline = p.stdout.readline()
            testResult += self.__parseAndWriteLine(outline)
            # If process returns a value, leave loop
            if p.poll() != None:
                break

        # Sometimes the process might exit before we finish reading entire stdout
        # Split ending of stdout in lines and finish reading it before ending the function
        stdoutEnding = [s + '\n' for s in p.stdout.read().split('\n')]
        for outline in stdoutEnding:
            testResult += self.__parseAndWriteLine(outline)

        return testResult

