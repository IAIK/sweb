#include "elf++.hh"
#include "dwarf++.hh"

#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <algorithm>

using namespace std;

struct MatchPathSeparator {
  bool operator()(char ch) const { return ch == '\\' || ch == '/'; }
};

string basename(string const &pathname) {
  return string(
      find_if(pathname.rbegin(), pathname.rend(), MatchPathSeparator()).base(),
      pathname.end());
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s elf-file dbg-file\n", argv[0]);
    return 2;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
    return 0;
  }
  FILE *d = fopen(argv[2], "wb");
  if(!d) return 0;

  try {
    elf::elf ef(elf::create_mmap_loader(fd));
    dwarf::dwarf dw(dwarf::elf::create_loader(ef));

    for (auto cu : dw.compilation_units()) {
      // printf("--- <%x>\n", (unsigned int)cu.get_section_offset());
      const dwarf::line_table &lt = cu.get_line_table();

      char buffer[128];
      
      // count entries
      uint32_t lines = 0;
      for (auto &line : lt) {(void)line; lines++;}
      if(!lines) continue;
      
      // save filename
      string file(basename(begin(lt)->file->path));
      int len = file.size();
      if(len > 127) len = 127;
      buffer[0] = (char)len;
      memcpy(buffer + 1, file.c_str(), len);
      fwrite(buffer, sizeof(char), len + 1, d);
      
      // save number of entries
      fwrite(&lines, sizeof(uint32_t), 1, d);
      
      // save lines
      for (auto &line : lt) {
        if (!line.end_sequence) {
          memcpy(buffer, &line.address, 8);
          memcpy(buffer + 8, &line.line, 4);
          fwrite(buffer, sizeof(char), 12, d);
        }
      }
    }
  } catch (...) {
  }

  fclose(d);
  return 0;
}
