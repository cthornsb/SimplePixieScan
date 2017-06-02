#include <iostream>

#include "simpleGui.hpp"

#include "TSystem.h"
#include "TGWindow.h"
#include "TGFrame.h"
#include "TGLabel.h"

#define SLEEP_WAIT 1E4 // (in us).

///////////////////////////////////////////////////////////////////////////////
// class SimpleTextButton
///////////////////////////////////////////////////////////////////////////////

SimpleTextButton::SimpleTextButton(TGWindow *p, const char *s, const int &id, bool *action_) : TGTextButton(p, s, id), action(action_) { }

// Toggle the bool value this button points to.
void SimpleTextButton::Clicked(){ (*action) = !(*action); }

///////////////////////////////////////////////////////////////////////////////
// class SimpleButtonParams
///////////////////////////////////////////////////////////////////////////////

SimpleButtonParams::SimpleButtonParams(const int &id_, const std::string &name_, bool *ptr_, TGTextButton *button_, const bool &state_/*=false*/) : id(id_), name(name_), ptr(ptr_), button(button_) { 
	if(state_) Activate();
	else Deactivate();
}

SimpleButtonParams::~SimpleButtonParams(){ delete button; }

void SimpleButtonParams::Activate(){ 
	button->SetState(kButtonDown);
	(*ptr) = true; 
}

void SimpleButtonParams::Deactivate(){
	button->SetState(kButtonUp);
	(*ptr) = false; 
}

void SimpleButtonParams::CheckState(){ 
	if(button->GetState() == kButtonDown) Activate();
	else Deactivate();
}

///////////////////////////////////////////////////////////////////////////////
// class SimpleButtonGroup
///////////////////////////////////////////////////////////////////////////////

SimpleButtonGroup::SimpleButtonGroup(TGMainFrame *f_, const std::string &name_) : TGButtonGroup(f_, name_.c_str()), id(0), isClicked(false), name(name_) { 
	this->Show();
	this->SetRadioButtonExclusive();
}

TGCheckButton *SimpleButtonGroup::AddCheckbox(const std::string &name_, bool *ptr_, const bool &clicked_/*=false*/, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=109*/, const int &h_/*=20*/){
	TGCheckButton *checkbox = new TGCheckButton(this, name_.c_str(), id);
	SetProperties(checkbox);
	checkbox->MoveResize(x_, y_, w_, h_);

	if(clicked_) buttons.push_back(new SimpleButtonParams(id++, name_, ptr_, checkbox, true));
	else         buttons.push_back(new SimpleButtonParams(id++, name_, ptr_, checkbox, false));

	return checkbox;
}

TGRadioButton *SimpleButtonGroup::AddRadio(const std::string &name_, bool *ptr_, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=106*/, const int &h_/*=20*/){
	TGRadioButton *radio = new TGRadioButton(this, name_.c_str(), id);
	SetProperties(radio);
	radio->MoveResize(x_, y_, w_, h_);

	// Set the first radio button to be selected.
	if(id == 0) buttons.push_back(new SimpleButtonParams(id++, name_, ptr_, radio, true));
	else        buttons.push_back(new SimpleButtonParams(id++, name_, ptr_, radio, false));
	
	return radio;
}

TGTextButton *SimpleButtonGroup::AddButton(const std::string &name_, bool *ptr_, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=92*/, const int &h_/*=25*/){
	SimpleTextButton *button = new SimpleTextButton(this, name_.c_str(), id, ptr_);
	SetProperties(button);
	button->MoveResize(x_, y_, w_, h_);
	buttons.push_back(new SimpleButtonParams(id++, name_, ptr_, button, false));
	return button;
}

bool SimpleButtonGroup::CheckState(){
	if(isClicked){
		UpdateButtonStates();
		isClicked = false;
		return true;		
	} 
	return false;
}

void SimpleButtonGroup::ButtonClicked(){ isClicked = true; }

void SimpleButtonGroup::SetProperties(TGTextButton* ptr_){
	ptr_->SetTextJustify(36);
	ptr_->SetMargins(0, 0, 0, 0);
	ptr_->SetWrapLength(-1);
}

void SimpleButtonGroup::UpdateButtonStates(){
	// Update the state of all buttons in this group.
	for(std::vector<SimpleButtonParams*>::iterator iter = buttons.begin(); iter != buttons.end(); iter++){
		(*iter)->CheckState();
	}
}

///////////////////////////////////////////////////////////////////////////////
// class GuiWindow
///////////////////////////////////////////////////////////////////////////////

GuiWindow::GuiWindow() : TGMainFrame(gClient->GetRoot(), 100, 100, kMainFrame), isWaiting(false), valsIndex(0) { 
	NewGroup("group1");	
}

GuiWindow::GuiWindow(const TGWindow *p, const std::string &firstName_/*="group1"*/, const int &w/*=100*/, const int &h/*=100*/) : TGMainFrame(p, w, h, kMainFrame), isWaiting(false), valsIndex(0) {
	NewGroup(firstName_);
}

TGCheckButton *GuiWindow::AddCheckbox(const std::string &name_, bool *ptr_/*=NULL*/, const bool &clicked_/*=false*/, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=109*/, const int &h_/*=20*/){
	if(!ptr_) ptr_ = GetNewBoolPointer();
	return currentGroup->AddCheckbox(name_, ptr_, clicked_, x_, y_, w_, h_);
}

TGRadioButton *GuiWindow::AddRadio(const std::string &name_, bool *ptr_/*=NULL*/, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=106*/, const int &h_/*=20*/){
	if(!ptr_) ptr_ = GetNewBoolPointer();
	return currentGroup->AddRadio(name_, ptr_, x_, y_, w_, h_);
}

TGTextButton *GuiWindow::AddButton(const std::string &name_, bool *ptr_/*=NULL*/, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=92*/, const int &h_/*=25*/){
	if(!ptr_) ptr_ = GetNewBoolPointer();
	return currentGroup->AddButton(name_, ptr_, x_, y_, w_, h_);
}

TGTextButton *GuiWindow::AddQuitButton(const std::string &name_/*="Quit"*/, const int &x_/*=0*/, const int &y_/*=0*/, const int &w_/*=92*/, const int &h_/*=25*/){
	return currentGroup->AddButton(name_, &this->isWaiting, x_, y_, w_, h_);
}

TGLabel *GuiWindow::AddLabel(const char *name_, const int &x_, const int &y_, const int &w_/*=57*/, const int &h_/*=19*/){
	TGLabel *ptr = new TGLabel(currentGroup, name_);
	ptr->SetTextJustify(36);
	ptr->SetMargins(0, 0, 0, 0);
	ptr->SetWrapLength(-1);
	ptr->MoveResize(x_, y_, w_, h_);
	labels.push_back(ptr);
	return ptr;
}

void GuiWindow::Update(){
	Clear();
	for(std::vector<SimpleButtonGroup*>::iterator iter = groups.begin(); iter != groups.end(); iter++){
		AddFrame(*iter, new TGLayoutHints(kLHintsCenterX, 1, 1, 1, 1));
	}
	MapSubwindows();
	MapWindow();
}

void GuiWindow::Wait(bool *ptr_/*=NULL*/){
	if(ptr_ == NULL) ptr_ = &isWaiting;
	(*ptr_) = true;
	while((*ptr_)){
		gSystem->ProcessEvents();
		usleep(SLEEP_WAIT);

		for(std::vector<SimpleButtonGroup*>::iterator iter = groups.begin(); iter != groups.end(); iter++){
			(*iter)->CheckState();
		}
	}
}

TGButtonGroup *GuiWindow::GetCurrentGroup(){ return currentGroup; }

void GuiWindow::NewGroup(const std::string &name_){
	groups.push_back(new SimpleButtonGroup(this, name_));
	currentGroup = groups.back();
}

bool GuiWindow::GetValue(const size_t &index_){
	if(index_ >= 100) return false;
	return vals[index_];	
}

void GuiWindow::PrintValues(){
	for(size_t i = 0; i < valsIndex; i++){
		std::cout << vals[i];
	}
	std::cout << std::endl;
}

bool *GuiWindow::GetNewBoolPointer(){
	vals[valsIndex] = false;
	return &vals[valsIndex++];	
}
