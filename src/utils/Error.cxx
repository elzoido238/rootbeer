#include <TGMsgBox.h>
#include "Error.hxx"



namespace {
bool have_gui() { 
	return gClient && gClient->GetRoot();
} }

rb::err::Gui::Gui(const char* what, const char* where):
	fWhat(what), fWhere(where) { ; }


rb::err::Gui::~Gui() {
	if(have_gui()) {
		std::stringstream msg;
		msg << "In <" << fWhere << ">: " << fBuffer.str();
		
		new TGMsgBox(gClient->GetRoot(), 0, fWhat.c_str(), msg.str().c_str());
	}
	else {
		Strm(fWhat.c_str(), fWhere.c_str()) << fBuffer.str();
	}
}


rb::err::InteractiveGui::InteractiveGui(const char* what, const char* where, Int_t* buttonpressed):
	fWhat(what), fWhere(where), fButtonPressed(buttonpressed) { ; }

rb::err::InteractiveGui::~InteractiveGui() {
	if(have_gui()) {
		std::stringstream msg;
		msg << "In <" << fWhere << ">: " << fBuffer.str();
		
		new TGMsgBox(gClient->GetRoot(), 0, fWhat.c_str(), msg.str().c_str(), 0, kMBYes | kMBNo, fButtonPressed);
	}
	else {
		Strm(fWhat.c_str(), fWhere.c_str()) << fBuffer.str() << " (y/n)";
		std::string answer;
		std::cin >> answer;
		if(answer == "y" || answer == "Y") *fButtonPressed = 1;
		else *fButtonPressed = 2;
	}
}

