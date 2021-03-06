#ifndef PSPMT_MAP_HPP
#define PSPMT_MAP_HPP

#include <vector>
#include <string>

class MapFile;

class PSPmtEvent{
  public:
	double dynTime;
	double dynLTQDC;
	double dynSTQDC;
	float xpos;
	float ypos;
	float anodes[4];
	bool channels[4];
	bool dynodeSet;

	PSPmtEvent(){ reset(); }

	void reset();

	void addDynode(const double &time, const double &L, const double &S);

	void addAnode(const float &anode, const size_t &index);

	void print() const ;

	bool allValuesSet();
};

class PSPmtMap{
  public:
	PSPmtMap();
	
	PSPmtMap(const int &dynodeL);
	
	PSPmtMap(const int &dynodeL, const int &dynodeR);

	int getLocation() const { return channels[0][0]; }

	bool getDoubleSided() const { return isDoubleSided; }
	
	PSPmtEvent *getEventL(){ return &pspmtEventL; }
	
	PSPmtEvent *getEventR(){ return &pspmtEventR; }
	
	int *getLeftChannels(){ return channels[0]; }
	
	int *getRightChannels(){ return channels[1]; }
	
	bool check(const int &location) const ;
	
	bool check(const int &location, bool &isDynode, bool &isRight, unsigned short &tqdcIndex) const ;
	
	bool checkLocations() const ;

	void setDynodes(const int &dynode);

	void setDynodes(const int &dynodeL, const int &dynodeR);
	
	void setAnodes(const int &se, const int &ne, const int &nw, const int &sw);	
	
	void setAnodes(const int &seL, const int &neL, const int &nwL, const int &swL, const int &seR, const int &neR, const int &nwR, const int &swR);
	
	void setLocationByTag(const int &location, const std::string &subtype, const std::string &tag);
	
	bool setNextLocation(const int &location);
	
	void print() const ;
	
	static bool readMapFile(MapFile *map, std::vector<PSPmtMap> &detMap);

  private:
	bool isDoubleSided;
	
	size_t index1;
	size_t index2;

	PSPmtEvent pspmtEventL;
	PSPmtEvent pspmtEventR;

	int channels[2][5];
};

#endif
