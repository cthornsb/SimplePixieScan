#ifndef MAIN_HPP
#define MAIN_HPP

class TCanvas;
class TH1;
class TH2;

extern bool Process(TH2 *h_, TCanvas *can_);

bool GetProjectionX(TH1 *h1_, TH2 *h2_, const int &binY_);

void help(char * prog_name_);

int Execute(int argc, char *argv[]);

#endif
