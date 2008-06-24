echo -e "\n\n\E[31mRemoving CMake specific files and reconfiguring CMake...\n"
tput sgr0
find -name 'CMakeCache.txt' -exec rm {} +;
find -name 'CMakeFiles' -exec rm -f -r {} +;
find -name 'cmake_install.cmake' -exec rm -f -r {} +;
rm -r bin/*
cmake .