#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[]){
	if(argc < 2){
		std::cout << "unregister: Error! Invalid number of arguments passed to unregister. Expected 1, received " << argc-1 << ".\n";
		std::cout << "unregister:  SYNTAX: unregister <filename>\n";
		return -1;	
	}

	const std::string searchString = "TROOT::RegisterModule";

	std::ifstream ifile(argv[1]);
	if(!ifile.good()){
		std::cout << "unregister: Error! Failed to open input file \"" << argv[1] << "\"!\n";
		return -1;
	}

	std::string ofname = std::string(argv[1]);
	ofname.insert(ofname.find_last_of('.'), "_mod");
	//std::cout << " debug: ofname=" << ofname << std::endl;

	std::ofstream ofile(ofname.c_str());
	if(!ofile.good()){
		std::cout << "unregister: Error! Failed to open output file \"" << ofname << "\"!\n";
		ifile.close();
		return -1;
	}

	int openParaCount = 0;
	int closeParaCount = 0;
	size_t index;
	bool searching = false;
	std::string line;
	//std::string tempStr;
	while(true){
		std::getline(ifile, line);
		if(ifile.eof()) break;
	
		// Search for the RegisterModule call.	
		if(!searching){
			index = line.find(searchString);
			if(index == std::string::npos){
				ofile << line << std::endl;
				continue;
			}
			searching = true;
		}
		
		// Found the RegisterModule call.
		for(size_t i = 0; i < line.length(); i++)
			if(line[i] == '(') openParaCount++;
		for(size_t i = 0; i < line.length(); i++)
			if(line[i] == ')') closeParaCount++;

		//tempStr += line.substr(0, line.find('\n'));

		// Check if all the parentheses have been closed.
		if(openParaCount == closeParaCount) searching = false;
	}

	// Remove whitespace.
	/*std::string functionCall;
	for(size_t i = 0; i < tempStr.length(); i++){
		if(tempStr[i] != ' ' && tempStr[i] != '\t') functionCall += tempStr[i];
	}

	std::cout << " debug: functionCall=\"" << functionCall << "\"\n";*/

	ifile.close();
}
