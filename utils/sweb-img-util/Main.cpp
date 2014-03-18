/**
 * Filename: Main.cpp
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#include <iostream>
#include <vector>

#include "Program.h"

int main(int argc, char** argv)
{
  if(argc <= 1)
  {
    // invalid usage
    std::cout << "usage: swb-img-util <options> or" << std::endl;
    std::cout << "swb-img-util <options> <image-file>" << std::endl;
    return 0;
  }

  // collecting the arguments
  std::vector<std::string> args;

  for(int i = 0; i < argc; i++)
  {
    args.push_back(argv[i]);
  }

  Program prog(args);
  if(!prog.run())
  {
    return -1;
  }

  return 0;
}

