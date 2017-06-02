#ifndef SIMPLEGUI_HPP
#define SIMPLEGUI_HPP

#include <string>
#include <vector>

#include "TGButton.h"
#include "TGButtonGroup.h"

class TGWindow;
class TGMainFrame;
class TGLabel;

///////////////////////////////////////////////////////////////////////////////
// class CTButton
///////////////////////////////////////////////////////////////////////////////

class SimpleTextButton : public TGTextButton {
  public:
	SimpleTextButton(TGWindow *p, const char *s, const int &id, bool *action_);

	// Toggle the bool value this button points to.
	virtual void Clicked();

  private:
	bool *action;
};

///////////////////////////////////////////////////////////////////////////////
// class SimpleButtonParams
///////////////////////////////////////////////////////////////////////////////

class SimpleButtonParams{
  public:
	SimpleButtonParams(const int &id_, const std::string &name_, bool *ptr_, TGTextButton *button_, const bool &state_=false);

	~SimpleButtonParams();

	void Activate();

	void Deactivate();

	void CheckState();

  private:
	int id;

	std::string name;

	bool *ptr;

	TGTextButton* button;
};

///////////////////////////////////////////////////////////////////////////////
// class SimpleButtonGroup
///////////////////////////////////////////////////////////////////////////////

class SimpleButtonGroup : public TGButtonGroup {
  public:
	SimpleButtonGroup(TGMainFrame *f_, const std::string &name_);

	TGCheckButton *AddCheckbox(const std::string &name_, bool *ptr_, const bool &clicked_=false, const int &x_=0, const int &y_=0, const int &w_=109, const int &h_=20);

	TGRadioButton *AddRadio(const std::string &name_, bool *ptr_, const int &x_=0, const int &y_=0, const int &w_=106, const int &h_=20);

	TGTextButton *AddButton(const std::string &name_, bool *ptr_, const int &x_=0, const int &y_=0, const int &w_=92, const int &h_=25);

	bool CheckState();

	virtual void ButtonClicked();

  private:
	int id;

	bool isClicked;

	std::string name;

	std::vector<SimpleButtonParams*> buttons;

	void SetProperties(TGTextButton* ptr_);

	void UpdateButtonStates();
};

///////////////////////////////////////////////////////////////////////////////
// class GuiWindow
///////////////////////////////////////////////////////////////////////////////

class GuiWindow : public TGMainFrame {
  public:
	GuiWindow();
	
	GuiWindow(const TGWindow *p, const std::string &firstName_="group1", const int &w=100, const int &h=100);
	
	TGCheckButton *AddCheckbox(const std::string &name_, bool *ptr_=NULL, const bool &clicked_=false, const int &x_=0, const int &y_=0, const int &w_=109, const int &h_=20);

	TGRadioButton *AddRadio(const std::string &name_, bool *ptr_=NULL, const int &x_=0, const int &y_=0, const int &w_=106, const int &h_=20);

	TGTextButton *AddButton(const std::string &name_, bool *ptr_=NULL, const int &x_=0, const int &y_=0, const int &w_=92, const int &h_=25);

	TGTextButton *AddQuitButton(const std::string &name_="Quit", const int &x_=0, const int &y_=0, const int &w_=92, const int &h_=25);

	TGLabel *AddLabel(const char *name_, const int &x_, const int &y_, const int &w_=57, const int &h_=19);

	void Update();

	void Wait(bool *ptr_=NULL);

	TGButtonGroup *GetCurrentGroup();

	void NewGroup(const std::string &name_);

	bool GetValue(const size_t &index_);

	bool IsQuitting(){ return isQuitting; }

	void PrintValues();

  private:
  	std::vector<TGLabel*> labels;
	std::vector<SimpleButtonGroup*> groups;
	
	SimpleButtonGroup* currentGroup;

	bool isWaiting;
	bool isQuitting;
	bool vals[100];

	size_t valsIndex;

	bool *GetNewBoolPointer();
};

#endif
