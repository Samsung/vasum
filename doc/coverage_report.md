Generating Code Coverage Report
===============================

Requirements
------------
 - [**gcc**](gcc.gnu.org) - provides `gcov` which generates coverage summary
 - [**gcovr**](gcovr.com) - recursively runs `gcov` and generates HTML report.
    [*PyPI*](pypi.python.org/pypi/gcovr) should provide the newest version.
    You can use `pip` command (available in most repositories):
    ```bash
    sudo pip install gcovr
    ```

Instructions
------------

All command should be run from within your build directory. Replace following expressions:
 - **repodir** - with a path to your repository root.
 - **covdir** - with a path you'd like to place your coverage report

-------------------------------------------------------------------------------

1. Generate your build using CCOV profile
```bash
cmake -DCMAKE_BUILD_TYPE=CCOV repodir
```

2. Compile, install and run tests in order to generate coverage files (.gcda extension)
```bash
make
sudo make install
sudo vsm_all_tests.py
```
   **Make sure that no other Vasum binaries are being used after installation
   except from those used by tests!**

3. Generate HTML report
```bash
gcovr -e tests -s -v -r repodir --html --html-details -o covdir/index.html
```

4. Open *index.html* in a web browser to see generated documentation. Click on any source file
   to see line coverage visualization.
