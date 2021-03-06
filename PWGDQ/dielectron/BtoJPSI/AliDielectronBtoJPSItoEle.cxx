/* Copyright(c) 1998-2009, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

//-------------------------------------------------------------------------
//                      Class AliDielectronBtoJPSItoEle
//                  Unbinned log-likelihood fit analysis class
//
//                             Origin: C.Di Giglio
//        Contact: Carmelo.Digiglio@ba.infn.it , giuseppe.bruno@ba.infn.it
//-------------------------------------------------------------------------
class TH1F ;
#include "TNtuple.h"
#include "TMath.h"

#include "AliDielectronBtoJPSItoEleCDFfitFCN.h"
#include "AliDielectronBtoJPSItoEleCDFfitHandler.h"
#include "AliDielectronBtoJPSItoEle.h"
#include "AliLog.h"

ClassImp(AliDielectronBtoJPSItoEle)

//_______________________________________________________________________________ 
AliDielectronBtoJPSItoEle::AliDielectronBtoJPSItoEle() :
fFCNfunction(0),
fMCtemplate(0),
fResType("FF")
{
  //
  // default constructor
  //
}
//___________________________________________________________________________________
AliDielectronBtoJPSItoEle::AliDielectronBtoJPSItoEle(const AliDielectronBtoJPSItoEle& source) :
TNamed(source),
fFCNfunction(source.fFCNfunction),
fMCtemplate(source.fMCtemplate),
fResType(source.fResType)
{
  //
  // copy constructor
  //
}
//_________________________________________________________________________________________________

AliDielectronBtoJPSItoEle &AliDielectronBtoJPSItoEle::operator=(const AliDielectronBtoJPSItoEle& source)
{
  //
  // assignment operator
  //
  if(&source == this) return *this;
  fFCNfunction = source.fFCNfunction;
  fMCtemplate = source.fMCtemplate;

  return *this;
}
//_________________________________________________________________________________________________
AliDielectronBtoJPSItoEle::~AliDielectronBtoJPSItoEle()
{
  //
  // destructor
  //
  delete fFCNfunction;
  delete fMCtemplate;
}
//_________________________________________________________________________________________________
Int_t AliDielectronBtoJPSItoEle::DoMinimization(Int_t step)
{
  //
  // performs the minimization
  //
  Int_t iret=fFCNfunction->DoMinimization(step);

  return iret;
}
//_________________________________________________________________________________________________
void AliDielectronBtoJPSItoEle::ReadCandidates(TNtuple* nt, Double_t* &pseudoproper, Double_t* &invmass, Double_t* &pt, Int_t * &typeCand, Int_t& ncand, Double_t massLow, Double_t massUp, Double_t ptLow, Double_t ptUp)
{
  //
  // Read N-tuple with X and M values
  //
  Float_t mJPSI = 0; Float_t x = 0; Float_t type = 0; Float_t transvMom = 0.;
  Int_t nentries = 0;
  ncand=0;
  TString arrType[] = {"SS","FS","FF"};
  nt->SetBranchAddress("Mass",&mJPSI);
  nt->SetBranchAddress("Xdecaytime",&x);
  nt->SetBranchAddress("Pt",&transvMom);
  //
  if(!nt->GetListOfBranches()->At(2)) {AliInfo("ERROR: branch with candidate type doesn't exist! \n"); return;}
  nt->SetBranchAddress("Type",&type);
  //
  nentries = (Int_t)nt->GetEntries();
  pseudoproper = new Double_t[nentries];
  invmass      = new Double_t[nentries];
  pt           = new Double_t[nentries];
  typeCand     = new Int_t[nentries];

  for(Int_t i = 0; i < nentries; i++) {
      nt->GetEntry(i);
      if(!fResType.Contains(arrType[(Int_t)type])) continue;
      if(massUp > massLow && massLow > 0) { if(mJPSI < massLow || mJPSI >massUp) continue; } 
      if(ptUp > ptLow && ptLow > 0) { if(transvMom < ptLow || transvMom > ptUp) continue; } 
      pseudoproper[ncand]=(Double_t)x;
      invmass[ncand]=(Double_t)mJPSI;
      pt[ncand]=(Double_t)transvMom;
      typeCand[ncand] = (Int_t)type;
      ncand++;
      }
 return; 
}
//_________________________________________________________________________________________________
void AliDielectronBtoJPSItoEle::SetCsiMC()
{
  //
  // Sets X distribution used as MC template for JPSI from B
  //
  fFCNfunction->LikelihoodPointer()->SetCsiMC(fMCtemplate);

  return;
}
//_________________________________________________________________________________________________
void AliDielectronBtoJPSItoEle::SetFitHandler(Double_t* x /*pseudoproper*/, Double_t* m /*inv mass*/, Double_t* pt /*pt*/, Int_t* type /*type*/, Int_t ncand /*candidates*/) 
{
  //
  // Create the fit handler object to play with different params of the fitting function
  //

  fFCNfunction = new AliDielectronBtoJPSItoEleCDFfitHandler(x,m,pt,type,ncand);
  if(!fFCNfunction) {

     AliInfo("fFCNfunction not istanziated  ---> nothing done");
     return;

     } 
}
