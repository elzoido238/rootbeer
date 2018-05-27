//! \file Error.hxx
//! \brief Defines C++ style error printing classes.
#ifndef __MAKECINT__
#ifndef ROOTBEER_ERROR_HXX
#define ROOTBEER_ERROR_HXX
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>


#define OP__(STRM, CLASS, ARGTYPE) CLASS& operator<< (ARGTYPE arg) {	\
    STRM << arg; return *this;																				\
  }
#define OPS__(STRM, CL)													\
  OP__(STRM, CL, const char*)										\
  OP__(STRM, CL, long long)											\
  OP__(STRM, CL, long)													\
  OP__(STRM, CL, int)														\
  OP__(STRM, CL, short)													\
  OP__(STRM, CL, char)													\
  OP__(STRM, CL, unsigned long long)						\
  OP__(STRM, CL, unsigned long)									\
  OP__(STRM, CL, unsigned int)									\
  OP__(STRM, CL, unsigned short)								\
  OP__(STRM, CL, unsigned char)									\
  OP__(STRM, CL, bool)													\
  OP__(STRM, CL, double)												\
  OP__(STRM, CL, float)													\
  OP__(STRM, CL, const std::string&)

namespace rb
{
namespace err
{
class Strm
{
protected:
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);
public:
    Strm(const char* what, const char* where) {
        std::cerr << what << " in <" << where << ">: ";
    }
    OPS__(std::cerr, Strm)
    Strm& operator<<(StandardEndLine manip) {
        manip(std::cerr);
        return *this;
    }
    virtual ~Strm() { std::endl(std::cerr); }
};
struct Info: public Strm { Info(const char* where) : Strm("Info", where) {} };
struct Error: public Strm { Error(const char* where) : Strm("Error", where) {} };
struct Warning: public Strm { Warning(const char* where) : Strm("Warning", where) {} };

class Gui
{
private:
    std::string fWhat, fWhere;
    std::stringstream fBuffer;
protected:
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);
public:
    Gui(const char* what, const char* where);
    virtual ~Gui();

    OPS__(fBuffer, Gui)
    Gui& operator<<(StandardEndLine manip) {
        manip(fBuffer);
        return *this;
    }
};

struct GuiInfo: public Gui { GuiInfo(const char* where) : Gui("Info", where) {} };
struct GuiError: public Gui { GuiError(const char* where) : Gui("Error", where) {} };
struct GuiWarning: public Gui { GuiWarning(const char* where) : Gui("Warning", where) {} };


class InteractiveGui
{
private:
    std::string fWhat, fWhere;
    std::stringstream fBuffer;
    Int_t* fButtonPressed;
protected:
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);
public:
    InteractiveGui(const char* what, const char* where, Int_t* buttonpressed);
    virtual ~InteractiveGui();

    OPS__(fBuffer, InteractiveGui)
    InteractiveGui& operator<<(StandardEndLine manip) {
        manip(fBuffer);
        return *this;
    }
};

struct InteractiveGuiInfo: public InteractiveGui
{ InteractiveGuiInfo(const char* where, Int_t* ret) : InteractiveGui("Info", where, ret) {} };
struct InteractiveGuiError: public InteractiveGui
{ InteractiveGuiError(const char* where, Int_t* ret) : InteractiveGui("Error", where, ret) {} };
struct InteractiveGuiWarning: public InteractiveGui
{ InteractiveGuiWarning(const char* where, Int_t* ret) : InteractiveGui("Warning", where, ret) {} };

class Throw
{
protected:
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);
    std::stringstream fStrm;
public:
    Throw() { }
    Throw(const char* where) {
        fStrm << "Error in <" << where << ">: ";
    }
    OPS__(fStrm, Throw);
    Throw& operator<<(StandardEndLine manip) {
        manip(fStrm);
        return *this;
    }
    virtual ~Throw() {
        throw(std::invalid_argument(fStrm.str().c_str()));
    }
};
}

}

#define ERR_FILE_LINE "\nFile, line: " << __FILE__ << ", " << __LINE__ << "."


#undef OP__
#undef OPS__
#endif
#endif
