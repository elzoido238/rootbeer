//! \file Event.hxx
//! \brief Defines a base class for rootbeer event processors.
#ifndef EVENT_HXX
#define EVENT_HXX
#include <set>
#include <memory>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <TClass.h>
#include <TTree.h>
#include <TTreeFormula.h>
#include "hist/Manager.hxx"
#include "utils/LockingPointer.hxx"
#include "utils/Critical.hxx"
#include "utils/Error.hxx"
#include "utils/nocopy.h"
#include "utils/boost_scoped_ptr.h"
#include "utils/boost_shared_ptr.h"


class TH1;
class TFile;
class TDirectory;
namespace rb
{
typedef boost::scoped_ptr<volatile TTreeFormula> FormulaPtr_t;

class Rint;
class TreeFormulae;
namespace data { template <class T> class Wrapper; }
namespace hist { class Base; }

/// \brief Abstract base class for event processors.
class Event
{
private:
	 //! Pointer to the event tree
	 boost::scoped_ptr<volatile TTree> fTree;

	 //! Manages histograms associated with the event
	 hist::Manager fHistManager;

public:
	 //! Start saving the output to a root tree on disk.
	 void StartSave(boost::shared_ptr<TFile> file, const char* name, const char* title, Bool_t save_hists = false);

	 //! Stop saving the output to a root tree on disk.
	 void StopSave();

	 //! Return a pointer to fHistManager
	 hist::Manager* const GetHistManager();

	 //! \brief Public interface for processing an event.
	 //! \details The real work for actually doing something with the event data
	 //! is done in the virtual member DoProcess(). This function just takes care
	 //! of behind-the-scenes stuff like filling histograms and mutex locking.
	 //! \param addr Address of the beginning of the event.
	 //! \param [in] nchar length of the event in bytes.
	 void Process(const void* event_address, Int_t nchar);

	 //! \brief Singleton instance function.
	 //! \details Each derived class is a singleton, with only one instance allowed.
	 //!  Use this function to get a pointer to the single instance of derived class <i>Derived</i>.
	 //! \tparam Derived The type of the derived class you want an instance of.
	 //! \returns Pointer to the single instance of <i>Derived</i>
	 template <typename Derived> static Derived*& Instance();

	 //! Get name and class name for all top level branches in fTree
	 //! \returns vector containing pairs of <name, class_name> for each top-level
	 //! branch in fTree.
	 std::vector< std::pair<std::string, std::string> >	 GetBranchList();

	 //! Search for a histogram by it's fHistogram address
	 rb::hist::Base* FindHistogram(TH1* th1);

	 //! Search for a histogram by it's name
	 rb::hist::Base* FindHistogram(const char* name, TDirectory* owner);

	 //! For compile-time checks whether or not a template argument is derived from rb::Event
   void CheckEventDerived() { }
protected:
	 //! Initialize data members
	 Event();
	 //! Nothing to do in the base class, all data is self-destructing.
	 virtual ~Event();

private:
	 //! \brief Does the work of processing an event.
	 //! \details Users shold implement this in derived classes to instruct the program
	 //!  on how to unpack event data into their classes.
	 //! \returns Error code: true upon successful unpack, false otherwise.
	 virtual Bool_t DoProcess(const void* event_address, Int_t nchar) = 0;

	 //! \brief Defines what should be done upon failure to successfully process an event.
	 //! \details Users may want/need to handle bad events differently, e.g. by throwing an exception,
	 //!  printing/logging an error message, aborting the program, etc. Since this is pure virtual, they get to choose.
	 virtual void HandleBadEvent() = 0;

	 //! \brief Actions to be completed at the beginning of a run.
	 //! \details This function is called any time we attach to a new data source. It is given a "null" implementation
	 //! here but can optionally be overridden in derived classes.
	 virtual void BeginRun() { };

public:
	 /// Adds a branch to the event tree.
	 class BranchAdd
	 {
      /// Perform the branch adding
      //! \returns true upon successful branch creation, false otherwise
      static Bool_t Operate(rb::Event* const event, const char* name, const char* classname, void** address, Int_t bufsize);
    
      /// Give access to rb::data::Wrapper
      template <class T> friend class rb::data::Wrapper;
	 };

	 /// Deletes an event and sets the pointer to NULL.
	 class Destructor
	 {
      /// Perform the destruction
      static void Operate(rb::Event*& event);

      /// Give access to rb::Rint (called from Terminate())
      friend class rb::Rint;
      friend class Event;
	 };

	 /// Initialize a TTreeFormula
	 class InitFormula
	 {
      /// Perform the initialization
      //! \param [in] formula_arg String specifying the formula argument
      static TTreeFormula* Operate(rb::Event* const event, const char* formula_arg);

      /// Give access to rb::TreeFormulae
      friend class rb::TreeFormulae;
	 };

	 /// For saving event data into a disk-resident tree.
	 class Save
	 {
			//! Pointer to rb::Event
			rb::Event* fEvent;
			//! Tells whether save is active or not
			Bool_t fIsActive;
			//! Tells whether or not to save histograms
			Bool_t fSaveHistograms;
			//! Shared pointer to file
			boost::shared_ptr<TFile> fFile;
			//! Tree pointer for saving output
			TTree* fTree;
			//! Vector of branch addresses (for fSaveTree)
			std::vector<void**> fBranchAddr;
	 public:
			//! Start saving
			void Start(boost::shared_ptr<TFile> file, const char* name, const char* title, Bool_t save_hists = false);
			//! Stop saving
			void Stop();
			//! Fill fTree (if active)
			void Fill();
			//! Constructor
			Save(rb::Event* event): fEvent(event), fIsActive(false), fSaveHistograms(false), fTree(0), fBranchAddr(0) { }
			//! Destructor
			~Save() { Stop(); }
	 };

	 /// \brief Functor class to for calling BeginRun()
	 /// \details Used in Buffer.cxx when attaching to a new data source, as an argument
	 /// to std::for_each
	 struct RunBegin
	 {
			/// operator(), used in std::for_each on a vector of std::pair<event code (int), event name (string)>
			void operator() (const std::pair<Int_t, std::string>&);
	 };

private:
	 boost::scoped_ptr<volatile Save> fSave;

#ifndef __MAKECINT__
	 friend class rb::Event::Save;
	 friend class rb::Event::RunBegin;
	 friend void Destructor::Operate(Event*&);
	 friend TTreeFormula* InitFormula::Operate(Event* const, const char*);
	 friend Bool_t BranchAdd::Operate(Event* const, const char*, const char*, void**, Int_t);
#endif
};
} // namespace rb



#ifndef __MAKECINT__

// ======== Inlined Function Implementations ========= //

inline rb::Event::~Event() {
}

inline rb::hist::Manager* const rb::Event::GetHistManager() {
  return &fHistManager;
}

inline void rb::Event::Destructor::Operate(rb::Event*& event) {
  delete event;
  event = 0;
}

template <typename Derived> Derived*& rb::Event::Instance() {
  static Derived* derived = 0;
  if(!derived) derived = new Derived();
	derived->CheckEventDerived();
  return derived;  
}

#endif

#endif
