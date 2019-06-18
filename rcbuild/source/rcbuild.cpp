/*! \file rcbuild.cpp
  * \brief A program to generate c++ source files for use with root dict generation.
  * \author Cory R. Thornsberry
  * \date June 11th, 2019
  */

#include <iostream>
#include <sstream>
#include <string.h>
#include <ctime>
#include <ctype.h> // toupper()

#include "rcbuild.hpp"
#include "optionHandler.hpp"

#define VERSION "1.0.7"
#define UPDATED "June 12, 2019"

bool SplitStr(const std::string &input_, std::string &out1, std::string &out2){
	out1 = "";
	out2 = "";
	bool left_side = true;
	for(size_t i = 0; i < input_.size(); i++){
		if(input_[i] == '\t'){ 
			left_side = false;
			continue;
		}
		
		if(left_side){ out1 += input_[i]; }
		else{ out2 += input_[i]; }
	}
	return true;
}

std::string Capitalize(const std::string &input_){
	std::string retval = input_;
	for(size_t i = 0; i < retval.size(); i++){
		if(retval[i] != '.')
			retval[i] = (char)toupper(retval[i]);
		else
			retval[i] = '_';
	}
	return retval;
}

DataType::DataType(const std::string &type_, const std::string &name_, const std::string &description_){
	is_vector = false;
	is_array = false;

	SetType(type_);
	name = name_; 
	descrip = description_;

	if(name.find("wave") != std::string::npos){ trace_value = true; }
	else{ trace_value = false; }
	
	if(name.find("mult") != std::string::npos){ mult_value = true; }
	else{ mult_value = false; }
	
	if(name.find("[") != std::string::npos){ is_array = true; }
	else{ is_array = false; }
}

DataType::DataType(const std::string &entry_, const char &delimiter){
	std::string v1 = "";
	name = "";
	descrip = "";
	
	unsigned int count = 0;
	for(size_t i = 0; i < entry_.size(); i++){
		if(entry_[i] == delimiter){
			if(++count == 4){ break; }
			continue;
		}
		else if(count == 0){ v1 += entry_[i]; }
		else if(count == 1){ name += entry_[i]; }
		else if(count == 2){ descrip += entry_[i]; }
		else{ break; }
	}

	if(name.find("wave") != std::string::npos){ trace_value = true; }
	else{ trace_value = false; }
	
	if(name.find("mult") != std::string::npos){ mult_value = true; }
	else{ mult_value = false; }

	if(name.find("[") != std::string::npos){ is_array = true; }
	else{ is_array = false; }
	
	SetType(v1);
}

void DataType::SetType(const std::string &input_){
	is_vector = false;
	
	type = "";
	decl = "";
	
	size_t index = input_.find("vector:");
	if(index != std::string::npos){
		is_vector = true;
		type = input_.substr(index + 7);
	}
	else{ type = input_; }
	
	index = type.find("u_");
	if(index != std::string::npos){ 
		type = "unsigned " + type.substr(index + 2);
	}
	
	if(is_vector){ decl = "std::vector<" + type + ">"; }
	else{ decl = type; }
}

void DataType::Print(){
	std::cout <<type << "\t" << decl << "\t" << name << "\t" << descrip << std::endl;
}

StructureEntry::StructureEntry(){
	name = "Temp"; 
	shorter = "Temp" ; 
	longer = "Temp";
}

void StructureEntry::push_back(const std::string &entry){
	DataType *data = new DataType(entry);
	
	structure_types.push_back(data);
}

std::string simpleDoxyDefinition(const std::string &methodName){
	std::stringstream stream;
	stream << "\t/** " << methodName << "\n";
	stream << "\t  */";
	return stream.str();
}

void StructureEntry::WriteHeader(std::ofstream *file_, const bool &newOutputMode, const std::string &date/*=""*/){
	if(!file_ || !file_->good() || file_->eof()){ return; }

	std::string suffix = "Structure";

	// Write the class description
	(*file_) << "\n/*! \\class " << name << suffix << "\n";
	(*file_) <<   " *  \\brief " << shorter << "\n";
	(*file_) <<   " *  \\author Cory R. Thornsbery\n";
	if(!date.empty())
		(*file_) <<   " *  \\date " << date << "\n";
	(*file_) <<   " *  \n";
	(*file_) <<   " *  " << longer << "\n";
	(*file_) <<   " */\n\n";

	(*file_) << "class " << name << suffix << " : public " << suffix << " {\n";
	(*file_) << "  public:\n";

	// Write the structure variables
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		(*file_) << "\t" << (*iter)->decl << " " << (*iter)->name << "; ///< " << (*iter)->descrip << std::endl;
	}

	(*file_) << "\n" << simpleDoxyDefinition("Default constructor") << "\n";
	(*file_) << "\t" << name << suffix << "();\n\n";

	if(!newOutputMode){
		(*file_) << simpleDoxyDefinition("Copy constructor") << "\n";
		(*file_) << "\t" << name << suffix << "(const " << name << suffix << " &other_);\n\n";
	}

	(*file_) << simpleDoxyDefinition("Destructor") << "\n";
	(*file_) << "\t~" << name << suffix << "(){}\n\n";

	int count;
	if(newOutputMode){
		(*file_) << "\t/** Set single entry data fields\n";
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (*iter)->is_vector){ continue; }
			(*file_) << "\t  * @param " << (*iter)->name << "_ " << (*iter)->descrip << "\n";
		}
		(*file_) << "\t  */\n";
		(*file_) << "\tvoid SetValues(";
		// Write the structure variables
		count = 0;
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (*iter)->is_vector){ continue; }
			if(count++ != 0) (*file_) << ", ";
			(*file_) << "const " << (*iter)->type << " &" << (*iter)->name << "_";
				
		}
		(*file_) << ");\n\n";
	}	

	(*file_) << "\t/** Push back with data\n";
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (newOutputMode && !(*iter)->is_vector)){ continue; }
		(*file_) << "\t  * @param " << (*iter)->name << "_ " << (*iter)->descrip << "\n";
	}
	(*file_) << "\t  */\n";
	(*file_) << "\tvoid Append(";
	// Write the structure variables
	count = 0;
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (newOutputMode && !(*iter)->is_vector)){ continue; }
		if(count++ != 0) (*file_) << ", ";
		(*file_) << "const " << (*iter)->type << " &" << (*iter)->name << "_";
	}
	(*file_) << ");\n\n";
	
	(*file_) << simpleDoxyDefinition("Zero all variables") << "\n";
	(*file_) << "\tvoid Zero();\n\n";

	if(!newOutputMode){
		(*file_) << simpleDoxyDefinition("Assignment operator") << "\n";
		(*file_) << "\t" << name << suffix << " &operator = (const " << name << suffix << " &other_);\n\n";

		(*file_) << simpleDoxyDefinition("Set all values using another object") << "\n";
		(*file_) << "\t" << name << suffix << " &Set(const " << name << suffix << " &other_);\n\n";

		(*file_) << simpleDoxyDefinition("Set all values using pointer to another object") << "\n";
		(*file_) << "\t" << name << suffix << " &Set(" << name << suffix << " *other_);\n\n";
	}

	(*file_) << "\t/// @cond DUMMY\n";
	(*file_) << "\tClassDef(" << name << suffix << ", 1); // " << name << "\n";
	(*file_) << "\t/// @endcond\n";
	(*file_) << "};\n";
}

void StructureEntry::WriteSource(std::ofstream *file_, const bool &newOutputMode){
	if(!file_ || !file_->good() || file_->eof()){ return; }

	std::string suffix = "Structure";

	(*file_) << "\n///////////////////////////////////////////////////////////\n";
	(*file_) << "// " << name << suffix << "\n";
	(*file_) << "///////////////////////////////////////////////////////////\n\n";

	// Default constructor
	(*file_) << name << suffix << "::" << name << suffix << "() : " << suffix << "(\"" << name << suffix << "\") {\n";
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->is_array || (*iter)->is_vector) continue;
		(*file_) << "\t" << (*iter)->name << " = 0;\n";
	}
	(*file_) << "}\n\n";

	if(!newOutputMode){
		// Copy constructor
		(*file_) << name << suffix << "::" << name << suffix << "(const " << name << suffix << " &other_) : " << suffix << "(\"" << name << suffix << "\") {\n";
		(*file_) << "\tSet(other_);\n";
		(*file_) << "}\n\n";
	}

	// SetValues
	int count;
	if(newOutputMode){
		(*file_) << "void " << name << suffix << "::SetValues(";
		count = 0;
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (*iter)->is_vector){ continue; }
			if(count++ != 0) (*file_) << ", ";
			(*file_) << "const " << (*iter)->type << " &" << (*iter)->name << "_";
		}
		(*file_) << "){\n";
		count = 0;
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (*iter)->is_vector){ continue; }
			(*file_) << "\t" << (*iter)->name << " = " << (*iter)->name << "_;\n";
		}
		(*file_) << "}\n\n";
	}	

	// Append
	(*file_) << "void " << name << suffix << "::Append(";
	count = 0;
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->trace_value || (*iter)->mult_value || (*iter)->is_array || (newOutputMode && !(*iter)->is_vector)){ continue; }
		if(count++ != 0) (*file_) << ", ";
		(*file_) << "const " << (*iter)->type << " &" << (*iter)->name << "_";
	}
	(*file_) << "){\n";
	count = 0;
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->trace_value || (*iter)->is_array || (newOutputMode && !((*iter)->is_vector || (*iter)->mult_value))){ continue; }
		(*file_) << "\t" << (*iter)->name;
		if((*iter)->is_vector){ (*file_) << ".push_back(" << (*iter)->name << "_);\n"; }
		else if(!(*iter)->mult_value){ (*file_) << " = " << (*iter)->name << "_;\n"; }
		else{ (*file_) << "++;\n"; }
	}
	(*file_) << "}\n\n";
	
	// Zero
	(*file_) << "void " << name << suffix << "::Zero(){\n";
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->mult_value){ 
			(*file_) << "\tif(" << (*iter)->name << " == 0){ return; } // " << suffix << " is already empty\n";
			break;
		}
	}
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		if((*iter)->is_array) continue;
		if((*iter)->is_vector){ (*file_) << "\t" << (*iter)->name << ".clear();\n"; }
		else{ (*file_) << "\t" << (*iter)->name << " = 0;\n"; }
	}
	(*file_) << "}\n\n";

	if(!newOutputMode){
		// Assignment operator
		(*file_) << name << suffix << " &" << name << suffix << "::operator = (const " << name << suffix << " &other_){\n";
		(*file_) << "\treturn Set(other_);\n";
		(*file_) << "}\n\n";

		// Set (const reference)
		(*file_) << name << suffix << " &" << name << suffix << "::Set(const " << name << suffix << " &other_){\n";
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			(*file_) << "\t" << (*iter)->name << " = other_." << (*iter)->name << ";\n";
		}
		(*file_) << "\treturn *this;\n";
		(*file_) << "}\n\n";

		// Set (pointer)
		(*file_) << name << suffix << " &" << name << suffix << "::Set(" << name << suffix << " *other_){\n";
		for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
			(*file_) << "\t" << (*iter)->name << " = other_->" << (*iter)->name << ";\n";
		}
		(*file_) << "\treturn *this;\n";
		(*file_) << "}\n";
	}
}

void StructureEntry::WriteLinkDef(std::ofstream *file_){
	if(structure_types.size() > 0){
		(*file_) << "#pragma link C++ class " << name << "Structure+;\n";
	}
}

void StructureEntry::Clear(){
	for(std::vector<DataType*>::iterator iter = structure_types.begin(); iter != structure_types.end(); iter++){
		delete (*iter);
	}
	structure_types.clear();
}

bool StructureFile::Open(){
	if(init){ return false; }

	hppfile.open(hpp_filename.c_str());
	if(!hppfile.good()){ return false; }

	cppfile.open(cpp_filename.c_str());
	if(!cppfile.good()){ 
		hppfile.close();
		return false; 
	}
	
	linkfile.open(link_filename.c_str());
	if(!linkfile.good()){ 
		hppfile.close();
		cppfile.close();
		return false; 
	}

	std::string preProcessorFlag = Capitalize(hpp_filename_nopath);

	hppfile << "/*! \\file " << hpp_filename_nopath << "\n";
	hppfile << " *  \\brief Data structures for root output\n";
	hppfile << " *  \\author Cory R. Thornsbery\n";
	hppfile << " *  \\date " << UPDATED << "\n";	
	hppfile << " *\n";
	hppfile << " *  Special data types for Root output. Each individual processor which is\n";
	hppfile << " *  is used in the scan code should have its own Structure class. These classes\n";
	hppfile << " *  should contain simple C++ data types or vectors of simple C++ data types.\n";
	hppfile << " *  Vectors should be used for processors which are likely to have multiplicities\n";
	hppfile << " *  greater than one.\n";
	hppfile << " *  File automatically generated by\n";
	hppfile << " *   RootClassBuilder (v. " << VERSION << ") on " << UPDATED << "\n";
	hppfile << " */\n\n";
	hppfile << "#ifndef " << preProcessorFlag << "\n";
	hppfile << "#define " << preProcessorFlag << "\n\n";
	hppfile << "#include \"TObject.h\"\n\n";
	hppfile << "#include <vector>\n\n";
	
	// Write the class description
	hppfile << "/*! \\class Structure\n";
	hppfile << " *  \\brief Simple structure used to store experimental and simulated data\n";
	hppfile << " *  \\author Cory R. Thornsbery\n";
	hppfile << " *  \\date " << UPDATED << "\n";
	hppfile << " */\n\n";
	
	hppfile << "class Structure : public TObject {\n";
	hppfile << "  protected:\n";
	hppfile << "	std::string name; ///< The name of this data structure\n\n";
	hppfile << "  public:\n";
	hppfile << "	/** Default constructor\n";
	hppfile << "	  * @param name_ The name of this data structure\n";
	hppfile << "	  */\n";
	hppfile << "	Structure(const std::string &name_=\"\"){ name = name_; }\n\n";
	if(!newOutputMode){
		hppfile << simpleDoxyDefinition("Destructor") << "\n";
		hppfile << "	virtual ~Structure(){}\n\n";
		hppfile << simpleDoxyDefinition("Zero all variables") << "\n";
		hppfile << "	virtual void Zero(){}\n\n";
	}
	hppfile << "	/// @cond DUMMY\n";
	hppfile << "	ClassDef(Structure, 1); // Structure\n";
	hppfile << "	/// @endcond\n";
	hppfile << "};\n\n";
	
	// Write the class description
	hppfile << "/*! \\class Trace\n";
	hppfile << " *  \\brief Simple structure used to store vectors of values corresponding to digitizer traces\n";
	hppfile << " *  \\author Cory R. Thornsbery\n";
	hppfile << " *  \\date " << UPDATED << "\n";
	hppfile << " */\n\n";
	
	hppfile << "class Trace : public TObject {\n";
	hppfile << "  protected:\n";
	hppfile << "	std::string name; ///< The name of this trace object\n\n";
	hppfile << "  public:\n";
	hppfile << "	std::vector<unsigned short> wave; ///< Vector containing trace values\n";
	hppfile << "	unsigned int mult; ///< Multiplicity of the event\n\n";
	hppfile << "	/** Default constructor\n";
	hppfile << "	  * @param name_ The name of this data structure\n";
	hppfile << "	  */\n";
	hppfile << "	Trace(const std::string &name_=\"\");\n\n";
	hppfile << simpleDoxyDefinition("Destructor") << "\n";
	hppfile << "	~Trace(){}\n\n";
	hppfile << simpleDoxyDefinition("Zero all variables") << "\n";
	hppfile << "	void Zero();\n\n";
	hppfile << simpleDoxyDefinition("Set all values using another object") << "\n";
	hppfile << "	Trace &Set(const Trace &other_);\n\n";
	hppfile << simpleDoxyDefinition("Set all values using pointer to another object") << "\n";
	hppfile << "	Trace &Set(Trace *other_);\n\n";
	hppfile << "	/** Push back with an array of trace values\n";
	hppfile << "	  * @param arr_ Array containing trace values\n";
	hppfile << "      * @param size_ The number of values to copy from the input array\n";
	hppfile << "	  */\n";
	hppfile << "	void Append(unsigned short *arr_, const size_t &size_);\n\n";
	hppfile << "	/// @cond DUMMY\n";
	hppfile << "	ClassDef(Trace, 1); // Trace\n";
	hppfile << "	/// @endcond\n";
	hppfile << "};\n";
	
	cppfile << "#include \"" << hpp_filename_nopath << "\"\n\n";
	cppfile << "Trace::Trace(const std::string &name_/*=\"\"*/){\n";
	cppfile << "	name = name_;\n";
	cppfile << "	mult = 0;\n";
	cppfile << "}\n\n";
	cppfile << "void Trace::Zero(){\n";
	cppfile << "	wave.clear();\n";
	cppfile << "	mult = 0;\n";
	cppfile << "}\n\n";
	cppfile << "Trace &Trace::Set(const Trace &other_){\n";
	cppfile << "	wave = other_.wave;\n";
	cppfile << "	return *this;\n";
	cppfile << "}\n\n";
	cppfile << "Trace &Trace::Set(Trace *other_){\n";
	cppfile << "	wave = other_->wave;\n";
	cppfile << "	return *this;\n";
	cppfile << "}\n\n";
	cppfile << "void Trace::Append(unsigned short *arr_, const size_t &size_){\n";
	cppfile << "	wave.reserve(wave.size()+size_);\n";
	cppfile << "	for(size_t i = 0; i < size_; i++){\n";
	cppfile << "		wave.push_back(arr_[i]);\n";
	cppfile << "	}\n";
	cppfile << "	mult++;\n";
	cppfile << "}\n";

	linkfile << "#ifdef __CINT__\n\n";
	linkfile << "#include <vector>\n\n";
	linkfile << "#pragma link off all globals;\n";
	linkfile << "#pragma link off all classes;\n";
	linkfile << "#pragma link off all functions;\n\n";
	linkfile << "#pragma link C++ class Structure+;\n";
	linkfile << "#pragma link C++ class Trace+;\n\n";
	
	init = true;
	
	return true;
}

bool StructureFile::Open(const std::string &hpp_fname_, const std::string &cpp_fname_, const std::string &link_fname_){
	hpp_filename = hpp_fname_;

	size_t index = hpp_filename.find_last_of('/');
	if(index != std::string::npos){
		hpp_filename_nopath = hpp_filename.substr(index+1);
	}
	else{
		hpp_filename_nopath = hpp_filename;
	}

	cpp_filename = cpp_fname_;
	link_filename = link_fname_;
	return Open();
}

bool StructureFile::Process(const std::string &filename_){
	if(!init){ return false; }

	std::ifstream entry_file(filename_.c_str());
	if(!entry_file.good()){ return false; }

	StructureEntry *entry = new StructureEntry();

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime (&rawtime);

	std::string class_name;
	std::string brief_descrip;
	std::string long_descrip;
	date = asctime(timeinfo);
	
	size_t index = date.find_last_of('\n');
	if(index != std::string::npos)
		date = date.substr(0, index);

	bool running = true;
	std::string line, name, arg;
	while(running){
		getline(entry_file, line);
		if(entry_file.eof()){ break; }
		if(line == "" || line[0] == '#'){ continue; }
		
		SplitStr(line, name, arg);
		
		if(name == "BEGIN_CLASS"){
			class_name = arg;
		}
		else if(name == "SHORT"){
			brief_descrip = arg;
		}
		else if(name == "LONG"){
			long_descrip = arg;
		}
		else if(name == "BEGIN_TYPES"){
			while(true){
				getline(entry_file, line);
				if(entry_file.eof()){ 
					running = false;
					break; 
				}
				if(line == "" || line[0] == '#'){ continue; }
	
				if(line == "END_TYPES"){ break; }
	
				if(running){
					entry->push_back(line);
				}
			}
		}
		else if(name == "END_CLASS"){
			entry->SetName(class_name);
			entry->SetBrief(brief_descrip);
			entry->SetDescription(long_descrip);
		
			entry->WriteHeader(&hppfile, newOutputMode, date);
			entry->WriteSource(&cppfile, newOutputMode);
			entry->WriteLinkDef(&linkfile);
			entry->Clear();
		}
		else{ continue; }
	}
	
	delete entry;
	
	entry_file.close();
	
	return true;
}

void StructureFile::Close(){
	if(!init){ return; }

	hppfile << "\n#endif\n";
	linkfile << "\n#endif\n";
	
	hppfile.close();
	cppfile.close();
	linkfile.close();
}

int main(int argc, char *argv[]){
	optionHandler handler;
	handler.add(optionExt("input", required_argument, NULL, 'i', "<filename>", "Specify the filename of the input data file"));
	handler.add(optionExt("dir", required_argument, NULL, 'd', "<path>", "Specify the top-level directory (default=./)"));
	handler.add(optionExt("prefix", required_argument, NULL, 'P', "<prefix>", "Specify the filename prefix (default=\"Structures\")"));
	handler.add(optionExt("verbose", no_argument, NULL, 'V', "", "Enable verbose output"));
	handler.add(optionExt("new-mode", no_argument, NULL, 'n', "", "Enable new version output"));

	if(!handler.setup(argc, argv)){
		return 1;
	}

	bool verbose = false;
	if(handler.getOption(3)->active){
		verbose = true;	
	}

	bool newOutputMode = false;
	if(handler.getOption(4)->active){
		newOutputMode = true;	
	}

	std::string ifname;
	if(!handler.getOption(0)->active){
		if(verbose) std::cout << " ERROR: No input filename specified!\n";
		return 2;
	}
	else{
		ifname = handler.getOption(0)->argument;
	}

	std::string dict_dir = "./";
	if(handler.getOption(1)->active){
		dict_dir = handler.getOption(1)->argument;
	}	

	std::string outputFilenamePrefix = "Structures";	
	if(handler.getOption(2)->active){
		outputFilenamePrefix = handler.getOption(2)->argument;
	}

	if(verbose) std::cout << " " << argv[0] << ": Generating root data structure file... ";

	StructureFile sfile;
	if(newOutputMode) sfile.SetNewOutputMode(true);
	if(!sfile.Open(dict_dir+"/"+outputFilenamePrefix+".hpp", dict_dir+"/"+outputFilenamePrefix+".cpp", dict_dir+"/LinkDef.h")){ 
		if(verbose) std::cout << "failed\n  ERROR: Failed to open output files in directory \"" << dict_dir << "\"!\n";
		return 3;
	}
	else if(!sfile.Process(ifname)){
		if(verbose) std::cout << "failed\n  ERROR: Failed to load input definitions file \"" << ifname << "\"!\n";
		return 4;
	}
	
	if(verbose) std::cout << "done\n";

	return 0;
}
