#!/usr/bin/env bash

# Grok AutoBuild Script for Travis CI
# (c) 2017 Charlton Trezevant
# MIT License

# Set to 1 to print platform, system, and language version information
PRINT_SYSINFO=1

# Set to 1 to enable static code analysis before building (via scan-build)
# Linting will be run if RUN_LINTER != 0, and scan-build is available on the system
RUN_LINTER=1

# List of subdirectories to blacklist (in our case, never build the Template project)
BLACKLIST_DIRS="Labs"

# How many commits back should we check for changes to files? ("lower bound", default 10 commits)
COMMIT_RANGE_LBOUND=10

if [ $PRINT_SYSINFO == 1 ]
then
  
  OS_INFO="$(uname -a)"
  
  # Semi-naive platform detection
  if [[ "$(uname -a | grep Linux)" == "" ]]
  then
    # Non-linux (Darwin, BSD)
  	CPU_INFO=$(sysctl -n machdep.cpu.brand_string | awk '{print "\t" $0}')
  else
    # Linux
  	CPU_INFO=$(lscpu | awk '{print "\t" $0}')
  fi

  # Retrieve version strings from desired languages/tooling
  GCC_VER="$(gcc --version 2>&1 | awk '{print "\t" $0}')" &> /dev/null 2>&1
  MAKE_VER=`make --version | grep "GNU Make" | awk '{print "\t" $0}'`
  #JAVA_VER="$(java -version 2>&1 | awk '{print "\t" $0}')"
  #PYTHON_VER="$(python --version 2>&1 | awk '{print "\t" $0}')"
  #RUBY_VER="$(ruby --version 2>&1 | awk '{print "\t" $0}')"
  #NODE_VER="$(node --version 2>&1 | awk '{print "\t" $0}')"

echo -e "

----------------------------------
Grok AutoBuild Script for Travis CI
  (c) 2017 Charlton Trezevant
`date +'%A %b %d %Y %r'`
----------------------------------

System information:
    OS:
      $OS_INFO
    CPU:
      $CPU_INFO

Language versions:
    C:
      GCC:
      $GCC_VER
      Make:
      $MAKE_VER
"
fi

# Find subdirectories with new changes and makefiles (which we can then build and test), excluding the blacklist
# CHANGED_DIRS will collect the list of new directories that contain new changes (in the past $COMMIT_RANGE_LBOUND commits) to .c source files
# This will ensure that Travis attempts to build only the latest changes, which is what we want
CHANGED_DIRS=`git diff --name-only HEAD~$COMMIT_RANGE_LBOUND..HEAD '*.c' | cut -d '/' -f1 | sed -e 's/^/.\//'`

# We'll keep track of the number of build failures here
FAILED_BUILDS=""

# Only attempt to run builds if there are changes
# This happens by default, but this fixes a bug where the script
# would error out if CHANGED_DIRS is empty.
if [ "$CHANGED_DIRS" != "" ]
then
  # BUILDABLE_DIRS will contain the list of directories with new/changed files that *also* have makefiles (and can therefore be built)
  BUILDABLE_DIRS=`find $CHANGED_DIRS -name '*akefile' | grep -v $BLACKLIST_DIRS | awk '{gsub(/[^\/]*$/,"");print}'`

  for SUBDIR in $BUILDABLE_DIRS;
  do

    cd $SUBDIR

    MAKEFILE=`cat *akefile`

    echo -e "\n/////////////////////////////////////////////////////////\n"
    echo -e "\t *** NOW BUILDING: $SUBDIR ***"

      # Run scan-build, for static analysis (if available on the system)
      # This will analyze code for any syntactical errors etc, using gcc
      # In the future, you may also prepend scan-build to make, which will run
      # scan-build while simultaneously building your project
      if [ $RUN_LINTER == 1 ] && [ `which scan-build` != "" ]
      then
        echo -e "\n>>> Running: static analysis (scan-build)"
        scan-build gcc -std=c99 -c src/*
        LINT_PASS=$?
        echo -e ">>> FINISHED: static analysis (scan-build)"
      else
        echo -e "\n>>> !!SKIPPING: static analysis (RUN_LINTER is false or scan-build is not available)"
        LINT_PASS=0
      fi

      TESTED=0
      # Run make test, if rule exists
      if [ "$(echo "$MAKEFILE" | awk '$0 ~ /^test[\t ]*:/ {print}')" != "" ]
      then
        echo -e "\n>>> Running: make test"
        make test
        MAKE_PASS=$?
        TESTED=1
        echo -e ">>> FINISHED: make test"
      fi

      # run make all, but only if the test rule isn't defined.
      # By default make test will build the project and run tests, as well, so
      # we don't need to build the project twice (once for all, again for tests)
      if [ "$(echo "$MAKEFILE" | awk '$0 ~ /^all[\t ]*:/ {print}')" != "" ] && [ $TESTED == 0 ]
      then
        echo -e "\n>>> Running: make all"
        make
        MAKE_PASS=$?
        echo -e ">>> FINISHED: make all"
      fi

      # Run make clean, if rule exists
      if [ "$(echo "$MAKEFILE" | awk '$0 ~ /^clean[\t ]*:/ {print}')" != "" ]
      then
        echo -e "\n>>> Running: make clean"
        make clean
        echo -e ">>> FINISHED: make clean"
      fi

      if [ $(($LINT_PASS + $MAKE_PASS)) == 0 ]
      then
          echo -e "\n\t *** BUILD SUCCESS: $SUBDIR ***"
      else
          echo -e "\n\t !!! BUILD FAILURE: $SUBDIR !!!"
          FAILED_BUILDS=${FAILED_BUILDS}" $SUBDIR"
      fi

    echo -e "\n/////////////////////////////////////////////////////////\n"

    unset MAKEFILE
    unset LINT_PASS
    unset MAKE_PASS

    cd ..

  done
else
  echo -e "\n/////////////////////////////////////////////////////////\n"
  echo -e "                   * Nothing to Build *                      "
  echo -e "\n/////////////////////////////////////////////////////////\n"
fi

# Finally, display a summary of all builds performed, and how successful those were.
# If any of our builds failed, we'll return the appropriate status code so Travis will know
echo -e "\n--------------REPORT--------------\n"
if [ -z $FAILED_BUILDS ]
then
  echo -e " All projects built successfully!"
  EXCODE=0
else
  echo -e " Some projects failed to build :("
  echo -e " Projects with build failures: $FAILED_BUILDS"
  EXCODE=1
fi
echo -e "\n----------------------------------\n"

exit $EXCODE
