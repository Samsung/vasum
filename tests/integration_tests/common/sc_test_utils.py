'''! Module containing utilities for domain-tests

@author Lukasz Kostyra (l.kostyra@samsung.com)
'''
import subprocess
import os



def launchProc(cmd):
    '''! Launch specified command as a subprocess.

    Launches provided command using Python's subprocess module. Returns output from stdout and
    stderr.

    @param cmd Command to be launched
    @return Output provided by specified command.
    @exception Exception When a process exits with error code, a Exception object containing message
                         with error data is raised.
    '''
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ret = p.wait()
    output = p.stdout.read()

    if ret != 0:
        raise Exception(cmd + " failed. error: " + os.strerror(ret) + ", output: " + output)

    return output



def mount(dir, opts = []):
    '''! Mounts specified directory with additional command line options.

    @param dir Directory to be mounted
    @param opts Additional command line options for mount. This argument is optional.
    @see launchProc
    '''
    launchProc(" ".join(["mount"] + opts + [dir]))



def umount(dir):
    '''! Unmounts specified directory.

    @param dir Directory to be umounted
    @see launchProc
    '''
    launchProc(" ".join(["umount"] + [dir]))



def isNumber(str):
    '''! Checks if provided String is a number.

    @param str String to be checked if is a number.
    @return True if string is a number, False otherwise.
    '''
    try:
        int(str)
        return True
    except ValueError:
        return False
