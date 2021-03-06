//! \file Hist.cxx
//! \brief Implements the histogram class member functions.
#include <algorithm>
#include <iostream>
#include <fstream>
#include "boost/dynamic_bitset.hpp"
#include "Hist.hxx"
#include "Formula.hxx"
#include "Rint.hxx"
#include "Signals.hxx"
#include "Rootbeer.hxx"
#include "mxml/mxml.hxx"

typedef std::vector<std::string> StringVector_t;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Utility functions & classes                           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
namespace
{
  // Name checking function
  inline std::string check_name(const char* name) {
    std::string ret = name;
    if(gROOT->FindObject(name)) {
      Int_t n = 1;
      while(1) {
	std::stringstream sstr;
	sstr << name << "_" << n++;
	ret = sstr.str();
	if(!gROOT->FindObject(ret.c_str())) break;
      }
      rb::err::Info("rb::hist::Base") << "The name " << name <<
	" is already in use, creating " << name << "_" << n-1 << " instead.";
    }
    return ret;
  }
  // Reverset the order of a vector
  template <typename T>
  void reverse_vector(std::vector<T>& vect) {
    std::vector<T> temp(vect.rbegin(), vect.rend());
    vect.clear();
    vect.assign(temp.begin(), temp.end());
  }
  // Set default title
  inline std::string default_title(const char* gate, const char* params) {
    std::stringstream out;
    out << params << " { " << gate << " }";
    return out.str();
  }
  // Break a string into tokens
  inline std::vector<std::string> tokenize(const char* str, char token) {
    std::istringstream iss(str);
    std::string entry;
    std::vector<std::string> out;
    while(std::getline(iss, entry, token))
      out.push_back(entry);
    return out;
  }
  inline StringVector_t parse_params(const char* param, UInt_t ndimensions) {
    StringVector_t par = tokenize(param, ':');
    reverse_vector(par);
    if(par.size() != ndimensions)
      rb::err::Throw() << "Invalid parameter specificaton: \"" << param << "\" for a(n) "
		   << ndimensions << " dimensional histogram.";
    return par;
  }
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Class                                                 //
// rb::hist::Base                                        //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

Bool_t rb::hist::Base::fgOverwrite = false;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (1d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Base::Base(const char* name, const char* title, const char* param, const char* gate,
		     hist::Manager* manager, Int_t event_code,
		     Int_t nbinsx, Double_t xlow, Double_t xhigh):
  kEventCode(event_code), kDimensions(1), fManager(manager), fHistogramClone(0), kInitialParams(param), fParams(0), fGate(0),
  fHistVariant(TH1D(name, title, nbinsx, xlow, xhigh))
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (2d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Base::Base(const char* name, const char* title, const char* param, const char* gate,
		     hist::Manager* manager, Int_t event_code,
		     Int_t nbinsx, Double_t xlow, Double_t xhigh,
		     Int_t nbinsy, Double_t ylow, Double_t yhigh):
  kEventCode(event_code), kDimensions(2), fManager(manager), fHistogramClone(0), kInitialParams(param), fParams(0), fGate(0),
  fHistVariant(TH2D(name, title, nbinsx, xlow, xhigh, nbinsy, ylow, yhigh))
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (3d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Base::Base(const char* name, const char* title, const char* param, const char* gate,
		     hist::Manager* manager, Int_t event_code,
		     Int_t nbinsx, Double_t xlow, Double_t xhigh,
		     Int_t nbinsy, Double_t ylow, Double_t yhigh,
		     Int_t nbinsz, Double_t zlow, Double_t zhigh):
  kEventCode(event_code), kDimensions(3), fManager(manager), fHistogramClone(0), kInitialParams(param), fParams(0), fGate(0),
  fHistVariant(TH3D(name, title, nbinsx, xlow, xhigh, nbinsy, ylow, yhigh, nbinsz, zlow, zhigh))
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Base::Init()                           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Base::Init(const char* name, const char* title, const char* param, const char* gate, Int_t event_code) {
  // Set name & title
  if(!fgOverwrite) fName = check_name(name).c_str();
	else {
		Base* hist_base = dynamic_cast<Base*>(gROOT->FindObject(name));
		if(hist_base) delete hist_base;
		fName = name;
	}
  kDefaultTitle = default_title(gate, param);
  kUseDefaultTitle = std::string(title).empty();
  fTitle = kUseDefaultTitle ? kDefaultTitle.c_str() : title;

	std::stringstream sstr;
	sstr << std::hex << "(rb::hist::Base*)" << this;
  visit::hist::DoMember(fHistVariant, &TH1::SetNameTitle, sstr.str().c_str() /*fName.Data()*/, fTitle.Data());

  // Set gate and parameters
  InitParams(param, event_code);
  InitGate(gate, event_code);

  // Add to ROOT container
  if(gDirectory) {
    fDirectory = gDirectory;
    fDirectory->Append(this, kTRUE);
		if(Rint::gApp()->GetHistSignals()) Rint::gApp()->GetHistSignals()->NewOrDeleteHist();
  }
  else {
    rb::err::Warning("Hist::Init") << "gDirectory == 0; not adding to any ROOT collections.";
    fDirectory = gDirectory;
  }
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Destructor                                            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Base::~Base() {
	fManager->Remove(this); // locks TTHREAD_GLOBAL_MUTEX while running
	if(Rint::gApp()->GetHistSignals()) Rint::gApp()->GetHistSignals()->NewOrDeleteHist();
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Base::InitParams()                     //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Base::InitParams(const char* param, Int_t event_code) {
  StringVector_t par = parse_params(param, kDimensions);
  fParams.reset(new rb::TreeFormulae(par, event_code));
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Base::InitGate()                       //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Base::InitGate(const char* gate, Int_t event_code) {
  StringVector_t gate_(1, gate);
  fGate.reset(new rb::TreeFormulae(gate_, event_code));
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::Regate                                //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Base::Regate(const char* newgate) {
  Bool_t success = fGate->Change(0, newgate);
  if(!success) return -1;

  // Change title if appropriate
  if(kUseDefaultTitle) {
    fTitle = default_title(fGate->Get(0).c_str(), kInitialParams.c_str()).c_str();
    visit::hist::DoMember(fHistVariant, &TH1::SetTitle, fTitle.Data());
  }
  return 0;
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::GetHist()                             //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
TH1* rb::hist::Base::GetHist() {
  hist::StopAddDirectory stop_add;
  visit::hist::Clone::Do(fHistVariant, fHistogramClone);
  return fHistogramClone.get();
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::DoFill() [virtual]                    //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Base::DoFill(const std::vector<Double_t>& params) {
  std::vector<Double_t> axes(params.begin(), params.end());
  for(Int_t i=axes.size(); i< 3; ++i) axes.push_back(0);
  return visit::hist::Fill::Do(fHistVariant, axes[0], axes[1], axes[2]);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::FillUnlocked()                        //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Base::FillUnlocked() {
  Double_t gate = fGate->EvalUnlocked(0);
  if(!Bool_t(gate)) return 0;
  std::vector<Double_t> axes;
  fParams->EvalAllUnlocked(axes);
  return DoFill(axes);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::Fill() [locked data]                  //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Base::Fill() {
	RB_LOG << "Filling...\n";
  Double_t gate = fGate->Eval(0);
  if(!Bool_t(gate)) return 0;
  std::vector<Double_t> axes;
  fParams->EvalAll(axes);
  return DoFill(axes);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::Write()                               //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Base::Write(const char* name, Int_t option, Int_t bufsize) {
	return visit::hist::Write::Do(fHistVariant, name, option, bufsize);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::WriteXML()                            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Base::WriteXML(rb::XmlWriter*) {
	rb::err::Warning("WriteXML")
		<< "Not implemented for class " << ClassName() << ", the histogram \""
		<< GetName() << "\" will not be saved.";
}

Bool_t rb::hist::Base::CompareTH1 (TH1* th1) {
	TH1* this_ = visit::hist::Cast::Do(fHistVariant);
	if(th1 == this_) return true;
	else return false;
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Class                                                 //
// rb::hist::Summary                                     //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor                                           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Summary::Summary (const char* name, const char* title, const char* param, const char* gate,
			    hist::Manager* manager, Int_t event_code,
			    Int_t nbins, Double_t low, Double_t high, Option_t* orientation):
  Base(name, title, param, gate, manager, event_code, 1, 0, 1, 1, 0, 1),
  fBins(nbins), fLow(low), fHigh(high), kOrientArg(orientation)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Summary::SetOrientation()              //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Summary::SetOrientation(Option_t* orientation) {
  TString orient(orientation);
  orient.ToLower();
  if(orient == "v") kOrientation = VERTICAL;
  else if(orient == "h") kOrientation = HORIZONTAL;
  else {
    rb::err::Warning("rb::hist::Summary::Summary")
      << "Orientation specification " << orientation
      << " is not understood. Defaulting to vertical.";
    kOrientation = VERTICAL;
  }
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Summary::InitParams() [virtual]        //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
namespace
{ // ==== Helper Functions ===== //
  inline Bool_t is_range(StringVector_t::iterator& it, long* pos) {
    pos[0] = it->find("["); pos[1] = it->find("-"), pos[2] = it->find("]");
    return (pos[0] < pos[1] && pos[1] < pos[2]);
  }
  inline Int_t str2int(const std::string& str) {
    std::stringstream sstr; sstr << str;
    Int_t out; sstr >> out; return out;
  }
  inline std::string subrange(long begin, long end, const std::string& str) {
    return str.substr(begin, end-begin);
  }
  inline void add_range(long* pos, StringVector_t::iterator& it, StringVector_t& pars) {
    Int_t lower = str2int(subrange(pos[0]+1, pos[1], *it));
    Int_t upper = str2int(subrange(pos[1]+1, pos[2], *it));
    if(lower > upper)	rb::err::Throw() << "Invalid parameter specification: lower index ("
				     << lower << ") > upper index (" << upper << ").";
    std::string base = subrange(0, pos[0], *it);
    for(Int_t i=lower; i <= upper; ++i) {
      std::stringstream param;
      param << base << "[" << i << "]";
      pars.push_back(param.str());
    }
  }
  inline StringVector_t parse_multiple_params(const char* param) {
    StringVector_t pars0 = tokenize(param, ';'), pars;
    for(StringVector_t::iterator it = pars0.begin(); it != pars0.end(); ++it) {
      long pos[3];
      if(is_range(it, pos)) add_range(pos, it, pars);
      else pars.push_back(*it);
    }
    return pars;
  }
}
// ===== InitParams ===== //
void rb::hist::Summary::InitParams(const char* param, Int_t event_code) {
	kParamArg = param;
  StringVector_t pars = parse_multiple_params(param);
  fParams.reset(new rb::TreeFormulae(pars, event_code));

  Int_t npar = pars.size();
  TAxis* paxis = 0;
  if(kOrientation == VERTICAL) {
    visit::hist::DoMember<void, HistVariant, TH1, Int_t, Double_t, Double_t, Int_t, Double_t, Double_t>
      (fHistVariant, &TH1::SetBins, npar, 0, npar, fBins, fLow, fHigh);
    paxis = visit::hist::DoConstMember(fHistVariant, &TH1::GetXaxis);
  }
  else {
    visit::hist::DoMember<void, HistVariant, TH1, Int_t, Double_t, Double_t, Int_t, Double_t, Double_t>
      (fHistVariant, &TH1::SetBins, fBins, fLow, fHigh, npar, 0, npar);
    paxis = visit::hist::DoConstMember(fHistVariant, &TH1::GetYaxis);
  }
  if(paxis) paxis->SetNdivisions(119);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Summary::DoFill() [virtual]                 //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Summary::DoFill(const std::vector<Double_t>& params) {
  Int_t ret = 0;
  for(UInt_t i=0; i< params.size(); ++i) {
    if(kOrientation == VERTICAL)
      ret += visit::hist::Fill::Do(fHistVariant, i, params[i], 0);
    else
      ret += visit::hist::Fill::Do(fHistVariant, params[i], i, 0);
  }
  return ret;
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Class                                                 //
// rb::hist::Gamma                                       //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (1d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Gamma::Gamma (const char* name, const char* title, const char* param, const char* gate,
			hist::Manager* manager, Int_t event_code,
			Int_t nbins, Double_t low, Double_t high):
  Base(name, title, param, gate, manager, event_code, nbins, low, high)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (2d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Gamma::Gamma (const char* name, const char* title, const char* param, const char* gate,
			hist::Manager* manager, Int_t event_code,
			Int_t nbinsx, Double_t xlow, Double_t xhigh,
			Int_t nbinsy, Double_t ylow, Double_t yhigh,
			Int_t nbinsz, Double_t zlow, Double_t zhigh):
  Base(name, title, param, gate, manager, event_code, nbinsx, xlow, xhigh, nbinsy, ylow, yhigh, nbinsz, zlow, zhigh)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (2d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Gamma::Gamma (const char* name, const char* title, const char* param, const char* gate,
			hist::Manager* manager, Int_t event_code,
			Int_t nbinsx, Double_t xlow, Double_t xhigh,
			Int_t nbinsy, Double_t ylow, Double_t yhigh):
  Base(name, title, param, gate, manager, event_code, nbinsx, xlow, xhigh, nbinsy, ylow, yhigh)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Gamma::InitParams() [virtual]          //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Gamma::InitParams(const char* params, Int_t event_code) {
  StringVector_t par0 = parse_params(params, kDimensions);
  StringVector_t pars;
  for(UInt_t i=0; i< kDimensions; ++i) {
    StringVector_t temp = parse_multiple_params(par0[i].c_str());
    pars.insert(pars.end(), temp.begin(), temp.end());
    fStops.push_back(temp.size());
    if(kDimensions > 0) {
      if(*(fStops.end()-1) != *(fStops.begin()))
	rb::err::Throw() << "Invalid parameter specification (\"" << params << "\").\n"
		     << "Multiple dimensional gamma histograms must consist of ordered pairs of parameters\n"
		     << "(i.e. you need to specify the same number of paramaters for each side of the ':').\n";
    }
  }
  fParams.reset(new rb::TreeFormulae(pars, event_code));
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Int_t rb::hist::Summary::DoFill() [virtual]           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Gamma::DoFill(const std::vector<Double_t>& params) {
  Int_t ret = 0;
  Double_t axes[3] = {0,0,0};
  for(Int_t i=0; i< fStops[0]; ++i) {
    for(UInt_t j=0; j< kDimensions; ++j) {
      axes[j] = params.at(i+fStops[0]*j);
    }
    ret += visit::hist::Fill::Do(fHistVariant, axes[0], axes[1], axes[2]);
  }
  return ret;
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Class                                                 //
// rb::hist::Bit                                         //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor (1d)                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Bit::Bit(const char* name, const char* title, const char* param, const char* gate,
		   hist::Manager* manager, Int_t event_code,
		   Int_t n_bits, Double_t ignored1, Double_t ignored2):
  Base(name, title, param, gate, manager, event_code, n_bits, 0, n_bits),
	kNumBits(n_bits)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Bit::InitParams() [virtual]            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Bit::InitParams(const char* params, Int_t event_code) {
  std::vector<std::string> par(1, params);
  fParams.reset(new rb::TreeFormulae(par, event_code));

  TAxis* paxis = visit::hist::DoConstMember(fHistVariant, &TH1::GetXaxis);
  if(paxis) paxis->SetNdivisions(119);
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Int_t rb::hist::Bit::DoFill() [virtual]               //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Bit::DoFill(const std::vector<Double_t>& params) {
  Int_t ret = 0;
  boost::dynamic_bitset<> bits(kNumBits, (unsigned long)params[0]);
  for(Int_t i=0; i< kNumBits; ++i) {
    if(bits[i]) {
      visit::hist::Fill::Do(fHistVariant, i, 0, 0);
      ++ret;
    }
  }
  return ret;
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Class                                                 //
// rb::hist::Scaler                                      //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Constructor                                           //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
rb::hist::Scaler::Scaler(const char* name, const char* title, const char* param, const char* gate,
												 hist::Manager* manager, Int_t event_code,
												 Int_t nbins, Double_t low, Double_t high):
  Base(name, title, param, gate, manager, event_code, nbins, low, high),
	fNumEvents(0)
{  }

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// void rb::hist::Scaler::Clear() [virtual]              //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Scaler::Clear(Option_t* option) {
	rb::hist::Base::Clear(option);
	fNumEvents = 0;
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Int_t rb::hist::Scaler::DoFill() [virtual]            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
Int_t rb::hist::Scaler::DoFill(const std::vector<Double_t>& params) {
	// this->Extend(1.5);
	visit::hist::SetBinContent::Do(fHistVariant, 1 + fNumEvents++, params[0]);
	return params[0];
}
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// Int_t rb::hist::Scaler::Extend() [private]            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
void rb::hist::Scaler::Extend(double factor) {
	rb::ScopedLock<rb::Mutex> LOCK (TTHREAD_GLOBAL_MUTEX);
	TH1* pHist = visit::hist::Cast::Do(fHistVariant);

	if (pHist->GetNbinsX() <= fNumEvents) {
		const double Max = pHist->GetBinLowEdge(pHist->GetNbinsX() + 1);
		const double Min = pHist->GetBinLowEdge(1);
		const double binWidth = (Max - Min) / pHist->GetNbinsX();

		const double newMax = Max*factor;
		const Int_t newBins = (newMax - Min) / binWidth;

		pHist->SetBins(newBins, Min, newMax);
	}
}




//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::XXXXX:WriteXML()                            //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

namespace {
TAxis* get_axis(rb::hist::Base* h, Int_t n) {
	switch(n) {
	case 0: return h->GetXaxis(); case 1: return h->GetYaxis(); case 2: return h->GetZaxis();
	default: return 0;
	}
}

const char* get_axis(Int_t n) {
	switch(n) {
	case 0: return "x"; case 1: return "y"; case 2: return "z";
	default: return 0;
	}
}

void write_attributes(rb::XmlWriter* w, rb::hist::Base* hist) {
	rb::mxml_write_attribute(w, "linecolor",   Form("%d", hist->GetHist()->GetLineColor()));
	rb::mxml_write_attribute(w, "linewidth",   Form("%d", hist->GetHist()->GetLineWidth()));
	rb::mxml_write_attribute(w, "linestyle",   Form("%d", hist->GetHist()->GetLineStyle()));
																												                                
	rb::mxml_write_attribute(w, "markercolor", Form("%d", hist->GetHist()->GetMarkerColor()));
	rb::mxml_write_attribute(w, "markersize",  Form("%f", hist->GetHist()->GetMarkerSize ()));
	rb::mxml_write_attribute(w, "markerstyle", Form("%d", hist->GetHist()->GetMarkerStyle()));
																												                                
	rb::mxml_write_attribute(w, "fillcolor",   Form("%d", hist->GetHist()->GetFillColor()));
	rb::mxml_write_attribute(w, "fillstyle",   Form("%d", hist->GetHist()->GetFillStyle()));
}

void write_xml(rb::XmlWriter* w, rb::hist::Base* h, Int_t ndim) {
	std::string title = h->UseDefaultTitle() ? "" : h->GetTitle();
	mxml_start_element(w, Form("rb_hist_D%d", ndim));
	mxml_write_attribute(w, "name", h->GetName());
	mxml_write_attribute(w, "title", title.c_str());

	for(Int_t i=0; i< ndim; ++i) {
		Int_t nbins = get_axis(h, i)->GetNbins();
		mxml_write_attribute(w, Form("%sbins",  get_axis(i)), Form("%d", nbins));
		mxml_write_attribute(w, Form("%slow",   get_axis(i)), Form("%f", get_axis(h, i)->GetBinLowEdge(1)));
		mxml_write_attribute(w, Form("%shigh",  get_axis(i)), Form("%f", get_axis(h, i)->GetBinLowEdge(nbins+1)));
	}

	mxml_write_attribute(w, "param", h->GetInitialParams());
	mxml_write_attribute(w, "gate",  h->GetGate().c_str());
	mxml_write_attribute(w, "event", Form("%d", h->GetEventCode()));

	write_attributes(w, h);
	mxml_end_element(w);
} }

void rb::hist::D1::WriteXML(rb::XmlWriter* w) {
	write_xml(w, this, 1);
}

void rb::hist::D2::WriteXML(rb::XmlWriter* w) {
	write_xml(w, this, 2);
}

void rb::hist::D3::WriteXML(rb::XmlWriter* w) {
	write_xml(w, this, 3);
}

void rb::hist::Summary::WriteXML(rb::XmlWriter* w) {
	std::string title = UseDefaultTitle() ? "" : GetTitle();

	Bool_t vertical = GetOrientation() == rb::hist::Summary::VERTICAL;
	const char* orient = vertical ? "v" : "h";
	TAxis* axis = vertical? GetYaxis() : GetXaxis();

	mxml_start_element(w, "rb_hist_Summary");
	mxml_write_attribute(w, "name", GetName());
	mxml_write_attribute(w, "title", title.c_str());

	mxml_write_attribute(w, "bins", Form("%d", axis->GetNbins()));
	mxml_write_attribute(w, "low",  Form("%f", axis->GetBinLowEdge(1)));
	mxml_write_attribute(w, "high", Form("%f", axis->GetBinLowEdge(1 + axis->GetNbins())));

	mxml_write_attribute(w, "param", GetInitialParams());
	mxml_write_attribute(w, "gate",  GetGate().c_str());
	mxml_write_attribute(w, "event", Form("%d", GetEventCode()));
	mxml_write_attribute(w, "orient", orient);

	write_attributes(w, this);
	mxml_end_element(w);
}

void rb::hist::Scaler::WriteXML(rb::XmlWriter* w) {
	std::string title = UseDefaultTitle() ? "" : GetTitle();

	mxml_start_element(w, "rb_hist_Scaler");
	mxml_write_attribute(w, "name", GetName());
	mxml_write_attribute(w, "title", title.c_str());

	mxml_write_attribute(w, "bins", Form("%d", GetXaxis()->GetNbins()));
	mxml_write_attribute(w, "low",  Form("%f", GetXaxis()->GetBinLowEdge(1)));
	mxml_write_attribute(w, "high", Form("%f", GetXaxis()->GetBinLowEdge(1 + GetXaxis()->GetNbins())));

	mxml_write_attribute(w, "param", GetInitialParams());
	mxml_write_attribute(w, "gate",  GetGate().c_str());
	mxml_write_attribute(w, "event", Form("%d", GetEventCode()));

	write_attributes(w, this);
	mxml_end_element(w);
}

void rb::hist::Bit::WriteXML(rb::XmlWriter* w) {
	std::string title = UseDefaultTitle() ? "" : GetTitle();

	mxml_start_element(w, "rb_hist_Bit");
	mxml_write_attribute(w, "name", GetName());
	mxml_write_attribute(w, "title", title.c_str());

	mxml_write_attribute(w, "bins", Form("%d", GetXaxis()->GetNbins()));

	mxml_write_attribute(w, "param", GetInitialParams());
	mxml_write_attribute(w, "gate",  GetGate().c_str());
	mxml_write_attribute(w, "event", Form("%d", GetEventCode()));

	write_attributes(w, this);
	mxml_end_element(w);
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//
// rb::hist::Base::ReadXML()                                  //
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\//

namespace {
template <int N>
rb::hist::Base* construct_hist(rb::XmlNode* node) {
	const char* name = mxml_get_attribute(node, "name");
	const char* title = mxml_get_attribute(node, "title");

	Int_t bins[N]; Double_t low[N], high[N];
	for(int i=0; i< N; ++i) {
		bins[i]  = atoi(mxml_get_attribute(node, Form("%sbins",   get_axis(i))));
		low[i]   = atof(mxml_get_attribute(node, Form("%slow",    get_axis(i))));
		high[i]  = atof(mxml_get_attribute(node, Form("%shigh",   get_axis(i))));
	}

	const char* param = mxml_get_attribute(node, "param");
	const char* gate  = mxml_get_attribute(node, "gate");
	Int_t event  = atoi(mxml_get_attribute(node, "event"));

	rb::hist::Base* hst = 0;
	switch(N) {
	case 1:
		hst = rb::hist::New(name, title, bins[0], low[0], high[0], param, gate, event);
		break;
	case 2:
		hst = rb::hist::New(name, title, bins[0], low[0], high[0], bins[1], low[1], high[1], param, gate, event);
		break;
	case 3:
		hst = rb::hist::New(name, title, bins[0], low[0], high[0], bins[1], low[1], high[1],
												bins[2], low[2], high[2], param, gate, event);
		break;
	default:
		break;
	}
	return hst;
}

template <Int_t N>
rb::hist::Base* construct_hist1(rb::XmlNode* node) {
	const char* name  = mxml_get_attribute(node, "name");
	const char* title = mxml_get_attribute(node, "title");
	Int_t bins     = atoi(mxml_get_attribute(node, "bins"));

	Double_t low, high;
	if(N==1 || N==2) {
		low   = atof(mxml_get_attribute(node, "low" ));
		high  = atof(mxml_get_attribute(node, "high"));
	}

	const char* param  = mxml_get_attribute(node, "param");
	const char* gate   = mxml_get_attribute(node, "gate");
	Int_t event   = atoi(mxml_get_attribute(node, "event"));
	const char* orient = 0;

	rb::hist::Base* hst;

	switch(N) {
	case 1:
		orient = mxml_get_attribute(node, "orient");
		hst = rb::hist::NewSummary(name, title, bins, low, high, param, gate, event, orient);
		break;
	case 2:
		hst = rb::hist::NewScaler(name, title, bins, low, high, param, gate, event);
		break;
	case 3:
		hst = rb::hist::NewBit(name, title, bins, param, gate, event);
		break;
	default:
		break;
	}

	return hst;
}

void set_attributes(rb::XmlNode* node, rb::hist::Base* hist) {
	const char* lc = rb::mxml_get_attribute(node, "linecolor");
	const char* lw = rb::mxml_get_attribute(node, "linewidth");
	const char* ls = rb::mxml_get_attribute(node, "linestyle");

	const char* mc = rb::mxml_get_attribute(node, "markercolor");
	const char* mw = rb::mxml_get_attribute(node, "markersize");
	const char* ms = rb::mxml_get_attribute(node, "markerstyle");
	
	const char* fc = rb::mxml_get_attribute(node, "fillcolor");
	const char* fs = rb::mxml_get_attribute(node, "fillstyle");

	if(lc) hist->SetLineColor(atoi(lc));
	if(lw) hist->SetLineWidth(atoi(lw));
	if(ls) hist->SetLineStyle(atoi(ls));

	if(mc) hist->SetMarkerColor(atoi(mc));
	if(mw) hist->SetMarkerSize (atof(mw));
	if(ms) hist->SetMarkerStyle(atoi(ms));

	if(fc) hist->SetFillColor(atoi(fc));
	if(fs) hist->SetFillStyle(atoi(fs));
}

typedef rb::hist::Base* (*XmlReader_t) (rb::XmlNode*);
}

rb::hist::Base* rb::hist::Base::ConstructXML(rb::XmlNode* node, Bool_t replace)
{
	static std::map<std::string, XmlReader_t> readers;
	if(readers.empty()) {
		readers.insert(std::make_pair("rb_hist_D1", construct_hist<1>));
		readers.insert(std::make_pair("rb_hist_D2", construct_hist<2>));
		readers.insert(std::make_pair("rb_hist_D3", construct_hist<3>));
		readers.insert(std::make_pair("rb_hist_Summary", construct_hist1<1>));
		readers.insert(std::make_pair("rb_hist_Scaler",  construct_hist1<2>));
		readers.insert(std::make_pair("rb_hist_Bit",     construct_hist1<3>));
	}

	rb::hist::Base* hist = 0;
	std::string type = mxml_get_name(node);
	std::map<std::string, XmlReader_t>::iterator it = readers.find(type);
	if(it != readers.end()) {
		if(replace) {
			const char* name = mxml_get_attribute(node, "name");
			if(name && gDirectory->Get(name) &&
				 gDirectory->Get(name)->InheritsFrom(rb::hist::Base::Class())) {
				gDirectory->Get(name)->Delete();
			}	
		}
		hist = it->second(node);
		set_attributes(node, hist);
	}

	return hist;
}
