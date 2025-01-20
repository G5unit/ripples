# Build

Default C compiler used is "clang". If you wish to use a different compiler such
as "gcc" edit the Makefile and change the CC variable from "clang" to "gcc".
C standard used is gnu17.

To compile the application run ```make ripples```. This will build the binary
which will be in build/bin directory.

The following make options are available:

    Usage: make <command>

    Commands:

    help           Prints his help message.

    all            Builds all targets: ripples, ripples_debug, test, and doc.

    ripples        Builds the ripples application. Binary file will be build/bin/ripples.

    ripples_debug  Builds the ripples application with debug output compiled in.
                   See documentation for details on debug output.
                   Binary file will be build/bin/ripples_debug.

    test           Builds and runs unit tests in "test" directory. Binary for
                   unit tests is test/ctest. You can execute it as is or run
                   "test/ctests -h" to see options it supports.
                   Unit tests require criterion library to be installed on
                   the system. On Ubuntu Linux you can do so via:
                   apt install libcriterion-dev

    doc            Build documentation using doxygen. You need doxygen installed
                   on the system. Documentation will be in build/docs/html
                   directory. Once built, open index.html file in a browser.

    lfds           Build liblfds only.

    clean          Cleans all builds.

    clean_ripples  Cleans ripples build.

    clean_debug    Removes ripples_debug binary.

    clean_test     Removes test/ctests binary.

    clean_doc      Removes built documentation.
