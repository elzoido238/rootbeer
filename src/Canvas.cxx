//! \file Canvas.cxx
//! \brief Implements canvas updating functions.
//! \details Also defines a number of internal functions to be called by the
//! user ones.
#include <TCanvas.h>
#include <TArrayL.h>
#include <TArrayL64.h>
#include "Rint.hxx"
#include "Rootbeer.hxx"
#include "hist/Hist.hxx"
#include "utils/Timer.hxx"
#include "utils/Error.hxx"



//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Internal constants, data, functions, and classes.     //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
namespace
{
// Maximum update rate, currently set to one second.
const Int_t kMaxRate = 1;

// The rate at which canvases are updated (in seconds).
Int_t gUpdateRate = 0;

// Update whatever histograms are on the current canvas/pad,
// including any sub-pads owned by this one.
void RecursiveUpdatePad(TVirtualPad* pad) {
	{
		if(!pad || !dynamic_cast<TPad*>(pad)) {
			std::string cl = pad ? pad->ClassName() : "N/A";
			rb::err::Error("RecursiveUpdatePad")
				 << "Passed an invalid TVirtualPad* pointer: "
				 << pad << ", Class Name: " << cl << ERR_FILE_LINE;
			return;
		}

		pad->cd();
		TList* primitives = pad->GetListOfPrimitives();
		for(Int_t i=0; i< primitives->GetEntries(); ++i) {
			TVirtualPad* subpad = dynamic_cast<TVirtualPad*>(primitives->At(i));
			if(subpad) {
				RecursiveUpdatePad(subpad);
			}
		}
	}
	pad->Modified();
	pad->Update();
}

// Clear whatever histograms are on the current canvas/pad,
// including any sub-pads owned by this one.
void RecursiveClearPad(TVirtualPad* pad) {
	if(!pad || !dynamic_cast<TPad*>(pad)) {
		std::string cl = pad ? pad->ClassName() : "N/A";
		rb::err::Error("RecursiveClearPad")
			 << "Passed an invalid TVirtualPad* pointer: "
			 << pad << ", Class Name: " << cl << ERR_FILE_LINE;
		return;
	}

	pad->cd();
	TList* primitives = pad->GetListOfPrimitives();
	for(Int_t i=0; i< primitives->GetEntries(); ++i) {
		TVirtualPad* subpad = dynamic_cast<TVirtualPad*>(primitives->At(i));
		if(subpad) RecursiveClearPad(subpad);
	}
	rb::canvas::ClearCurrent();
}

// Class inheriting from TTimer to perform auto-refresh.
// Overrides the Notify() method to perform the actions we want
// (here, refreshing all canvases).
class UpdateTimer: public TTimer
{
public:
	 // Constructor, mirrors that of TTimer
	 UpdateTimer(Long_t timeout = 5000, Bool_t mode = true) :
		 TTimer(timeout, mode) { }

	 // Override of the Notify() function
	 // Gets called whenever the timer times out (i.e. every _rate_ seconds).
	 // NOTE: Must call the Reset() function at the end, otherwise this just keeps executing forever.
	 Bool_t Notify() {
		 rb::canvas::UpdateAll();
		 this->Reset();
		 return true;
	 }
};

// Global (file scope) canvas timer instance
UpdateTimer gUpdateTimer;

} // namespace 



//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Implementation of functions in the                    //
// rb::canvas namespace                                  //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::UpdateCurrent()                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::canvas::UpdateCurrent() {
  if(gPad) {
    gPad->Modified();
    gPad->Update();
  }
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::UpdateAll()                          //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::canvas::UpdateAll() {
  TPad* pInitial = dynamic_cast<TPad*>(gPad);
  TPad* pad;
  for(Int_t i=0; i< gROOT->GetListOfCanvases()->GetEntries(); ++i) {
    pad = dynamic_cast<TPad*>(gROOT->GetListOfCanvases()->At(i));
    if(pad) RecursiveUpdatePad(pad);
  }
  if(pInitial) pInitial->cd();
	if(gPad) {
		gPad->Modified();
		gPad->Update();
	}
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::StopUpdate()                         //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::canvas::StopUpdate() {
	if(gUpdateRate) {
		gUpdateTimer.Stop();
		gUpdateRate = 0;
		if(gApp()->GetSignals()) {
			gApp()->GetSignals()->StoppingUpdate();
		}
	}
  return gUpdateRate;
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::StartUpdate()                        //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::canvas::StartUpdate(Int_t rate) {
  if(rate < kMaxRate) {
		rb::err::Error("StartUpdate") << "Passed an invalid update rate: " << rate
															<< ". Must be >= " << kMaxRate << " second(s).";
    return -1;
  }
  else {
    StopUpdate();
    gUpdateRate = rate;
		gUpdateTimer.Start(rate*1000);
		if(gApp()->GetSignals()) {
			gApp()->GetSignals()->StartingUpdate(rate);
		}
    return 0;
  }
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::GetUpdateRate()                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::canvas::GetUpdateRate() {
  return gUpdateRate;
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::ClearCurrent()                       //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::canvas::ClearCurrent() {
  if(gPad) {
    for(Int_t i = 0; i < gPad->GetListOfPrimitives()->GetEntries(); ++i) {
      TH1*    hst = dynamic_cast<TH1*>    (gPad->GetListOfPrimitives()->At(i));
			TArray* arr = dynamic_cast<TArray*> (gPad->GetListOfPrimitives()->At(i));
      if(hst && arr) {
				for(Int_t i=0; i< arr->fN; ++i) arr->SetAt(0., i);
			}
      gPad->Modified();
      gPad->Update();
    }
  }
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::canvas::ClearAll()                           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::canvas::ClearAll() {
  TPad* pInitial = dynamic_cast<TPad*>(gPad);
  TPad* pad = 0;
  for(Int_t i=0; i< gROOT->GetListOfCanvases()->GetEntries(); ++i) {
    pad = dynamic_cast<TPad*>(gROOT->GetListOfCanvases()->At(i));
    if(pad) RecursiveClearPad(pad);
  }
  if(pInitial) pInitial->cd();
		if(gPad) {
		gPad->Modified();
		gPad->Update();
	}
}

