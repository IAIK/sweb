printf "\n\n\E[31mRemoving CMake specific files...\n"
tput sgr0
printf "\nTo regenerate your makefiles run 'cmake .'\n\n"
find . -name 'CMakeCache.txt' -exec rm {} +;
find . -name 'Makefile' -exec rm {} +;
find . -name 'CMakeFiles' -exec rm -f -r {} +;
find . -name 'cmake_install.cmake' -exec rm -f -r {} +;
