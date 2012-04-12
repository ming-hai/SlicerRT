//   Copyright (C) 2009, Vaclav Haisman. All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without modifica-
//   tion, are permitted provided that the following conditions are met:
//
//   1. Redistributions of  source code must  retain the above copyright  notice,
//      this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//   FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//   APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//   INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//   DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//   OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//   ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//   (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef DCMTK_LOG4CPLUS_CONFIG_HXX
#define DCMTK_LOG4CPLUS_CONFIG_HXX

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#if defined (_WIN32)
#  include "dcmtk/oflog/config/win32.h"
#elif (defined(__MWERKS__) && defined(__MACOS__))
#  include "dcmtk/oflog/config/macosx.h"
#else
#  include "dcmtk/oflog/config/defines.h"
#endif

#if !defined(_WIN32)
#  if !defined(DCMTK_LOG4CPLUS_SINGLE_THREADED)
#    define DCMTK_LOG4CPLUS_USE_PTHREADS
#  endif
#  if defined (DCMTK_INSIDE_LOG4CPLUS)
#    define DCMTK_LOG4CPLUS_EXPORT DCMTK_LOG4CPLUS_DECLSPEC_EXPORT
#  else
#    define DCMTK_LOG4CPLUS_EXPORT DCMTK_LOG4CPLUS_DECLSPEC_IMPORT
#  endif // defined (DCMTK_INSIDE_LOG4CPLUS)
#endif // !_WIN32

#include "dcmtk/oflog/helpers/threadcf.h"


#endif // DCMTK_LOG4CPLUS_CONFIG_HXX