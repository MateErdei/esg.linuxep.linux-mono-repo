// CertificateTracker.cpp. This file implements a singleton class that 
// is used in conjunction with an OpenSSL callback to track the names
// of any failed certificates during a verification.
//
//  20030909 RW version 1.0.0
//
//////////////////////////////////////////////////////////////////////
#include "CertificateTracker.h"
#include <string>
#include <vector>

namespace verify_exceptions {

   CertificateTracker& CertificateTracker::GetInstance()
   {
      static CertificateTracker TheInstance;
      return TheInstance;
   }

   void CertificateTracker::AddProblem( string TheProblem, string CertName ) 
   {
      m_CertProblems.push_back(TheProblem);
      if ( m_CertNames.length() > 0 ){
         m_CertNames.append(", ");
      }
      m_CertNames.append(CertName);
   }

   vector<string>::const_iterator CertificateTracker::GetProblems() 
   {
      return m_CertProblems.begin();
   }

   vector<string>::const_iterator CertificateTracker::GetEnd() 
   {
      return m_CertProblems.end();
   }

   void CertificateTracker::Clear()
   {
      m_CertProblems.clear();
      m_CertNames.clear();
   }
} //namespace
