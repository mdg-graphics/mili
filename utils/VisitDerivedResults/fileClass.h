//started
//#include <string>

//using namespace std;

#ifndef FILE_CLASS_H
#define FILE_CLASS_H

class fileClass
{
 private:

  FILE* filepointer;
  std::string filename;

 public:

  fileClass();
  fileClass(std::string in);
  bool open();
  void write();
  void write(int in);
  void write(double in);
  void write(std::string in);
  void write(int left, int center, double right);
  bool close(); 
};

#endif
