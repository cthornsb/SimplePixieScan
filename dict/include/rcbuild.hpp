#include <fstream>
#include <string>
#include <vector>

struct DataType{
	std::string type;
	std::string decl;
	std::string name;
	std::string descrip;
	
	bool trace_value;
	bool mult_value;
	bool is_vector;
	
	DataType(const std::string &type_, const std::string &name_, const std::string &description_);
	
	DataType(const std::string &entry_, const char &delimiter='\t');
	
	void SetType(const std::string &input_);
	
	void Print();
};

class StructureEntry{
  private:
	std::string name;
	std::string shorter;
	std::string longer;
	
	std::vector<DataType*> structure_types;
	std::vector<DataType*> waveform_types;
	
  public:
	StructureEntry();
	
	~StructureEntry(){ Clear(); }

	void push_back(const std::string &entry);
	
	void SetName(const std::string &name_){ name = name_; }
	
	void SetBrief(const std::string &brief_){ shorter = brief_; }
	
	void SetDescription(const std::string &descrip_){ longer = descrip_; }
	
	void WriteHeader(std::ofstream *file_);

	void WriteSource(std::ofstream *file_);

	void WriteLinkDef(std::ofstream *file_);
	
	void Clear();
};

class StructureFile{
  private:
	std::ofstream hppfile;
	std::ofstream cppfile;
	std::ofstream linkfile;
	
	std::string hpp_filename;
	std::string cpp_filename;
	std::string link_filename;
	
	bool init;
  
  public:
	StructureFile(){ init = false; }
  
	StructureFile(const std::string &prefix_){ Open(prefix_); }
	
	StructureFile(const std::string &hpp_fname_, const std::string &cpp_fname_, const std::string &link_fname_){ Open(hpp_fname_, cpp_fname_, link_fname_); }
	
	~StructureFile(){ Close(); }
	
	bool Open();

	bool Open(const std::string &prefix_);
	
	bool Open(const std::string &hpp_fname_, const std::string &cpp_fname_, const std::string &link_fname_);
	
	bool Process(const std::string &filename_);
	
	void Close();
};
