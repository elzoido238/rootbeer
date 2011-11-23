/*! \file Unpack.hxx
 *  \brief Defined the class which
 *  attaches to and upacks data buffers.
 */
#ifndef __UNPACKER__
#define __UNPACKER__
#include <string>
#include "TTree.h"
#include "TThread.h"
#include "TRandom3.h"
#define   USER_INCLUDES
#include "Skeleton.hh"


/// Buffer size
/// \todo Make this easier to set for users.
static const Int_t BUFFER_SIZE = 4096;

namespace rb
{
  /// \brief "Public" interface to unpacking routines.
  /*! Functions:
   *  -# Grab buffers from a souce (typically online or a file).
   *  -# Unpack buffers into user data classes, event-by-event (user defined code).
   *  -# Fill all \c ROOTBEER Histogram objects.
   *
   *  This class generally runs as thread, using <tt>TThread</tt>, to allow
   *  the user access to the \c CINT command line while processing online data.
   *  Generally, the locking of shared objects (e.g. Histograms) is performed by the
   *  object itself, not by the Unpacker.
   *  \todo Change this from a class to functions in a namespace.
   *  \todo Make the data buffer smarter and safer, i.e. have the size and type
   *  be options (maybe use boost::variant for the type and vector to manage
   *  the size. Or at least check that it's valid, etc.
   */
  namespace unpack
  {
    /// Initialization function    
    extern void Initialize();

    /// Cleanup function
    extern void Cleanup();

    /// Unpack buffer into user structs
    /*! Should be user-defined, and call FillHistograms at completion.
     \todo Implement skeleton-type user interface. */
    extern void UnpackBuffer();

  }

}


#endif
