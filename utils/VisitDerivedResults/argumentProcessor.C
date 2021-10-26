
void
stressStrainClass::PrintArguments()
{
  fileClass printer = fileClass("argsOut.txt");
  printer.open();
  for(int i = 0; i < varnames.size(); i++)
    printer.write(varnames[i]);
  printer.close();
}

void
stressStrainClass::ParseArguments()
{
  int flags[4] = {0,0,0,0};
  fileClass printer = fileClass("argsOut.txt");
  printer.open();
  for(int i = 0; i < varnames.size(); i++){
    string holder = string(varnames[i]);
    if(holder.at(0) == ' '){
      //error
      printer.write("ERROR: input does not compute");
    }
    else{
      int found = holder.find(": ");
      std::string ident = holder.substr(0,found-1);
      
      if     (ident.compare("VAR1") == 0){
	flags[0] = atoi( ( holder.substr(found+1,holder.size()-1) ).c_str() );
      }
      else if(ident.compare("VAR2") == 0){
	flags[1] = atoi( ( holder.substr(found+1,holder.size()-1) ).c_str() );

      }
      else if(ident.compare("VAR3") == 0){
	flags[2] = atoi( ( holder.substr(found+1,holder.size()-1) ).c_str() );

      }
      else if(ident.compare("VAR4") == 0){
	flags[3] = atoi( ( holder.substr(found+1,holder.size()-1) ).c_str() );

      }
      else{// error
	//not a valid argument
	string message = string("were sorry but ____ is not a valid argument type");	
	printer.write(message);
      }
    }
  }
  printer.close();
}

void
stressStrainClass::ParseArguments(string* arguments,int count)
{
  int flags[4] = {0,0,0,0};
  fileClass printer = fileClass("argsOut.txt");
  printer.open();
  for(int i = 0; i < count; i++){
    string holder = arguments[i];
    if(holder.at(0) == ' '){
      //error
      printer.write("ERROR: input does not compute");
    }
    else{
      int found = holder.find(": ");
      string ident = holder.substr(0,found-1);
      
      if     (ident.compare("VAR1") == 0){
	flags[0] = atoi(holder.substr(found+1,(holder.size()-1)).c_str() );
      }
      else if(ident.compare("VAR2") == 0){
	flags[1] = atoi(holder.substr(found+1,(holder.size()-1)).c_str() );

      }
      else if(ident.compare("VAR3") == 0){
	flags[2] = atoi(holder.substr(found+1,(holder.size()-1)).c_str() );

      }
      else if(ident.compare("VAR4") == 0){
	flags[3] = atoi(holder.substr(found+1,(holder.size()-1)).c_str() );

      }
      else{// error
	//not a valid argument
	string message = string("were sorry but ____ is not a valid argument type");	
	printer.write(message);
      }
    }
  }
  printer.close();
}
