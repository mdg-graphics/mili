//started

#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

#include "fileClass.h"

fileClass::fileClass(){
  filename = "default.txt";
}

fileClass::fileClass(string in){
  filename = in;
}

bool fileClass::open(){
  //string filepath = string("");
  filepointer = fopen(filename.c_str(),"w");
  if(filepointer != NULL){
    return true;
  }
  return false;
}

void fileClass::write(){
  fprintf(filepointer, "\n", "");
}

void fileClass::write(int in){
  fprintf(filepointer, "%i\n", in);
}

void fileClass::write(double in){
  fprintf(filepointer, "%e\n", in);
}

void fileClass::write(string in){
  fprintf(filepointer, "%s\n", in.c_str());
}

void fileClass::write(int left, int center, double right){
  fprintf(filepointer, "%7i %7i %10.6f \n", left, center, right);
}

bool fileClass::close(){
  return fclose(filepointer);
}
