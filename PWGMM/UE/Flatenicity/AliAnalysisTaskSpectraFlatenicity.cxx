/****************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 *                                                                        *
 * Original task: AliAnalysisTaskFlatenicity.cxx                          *
 * Author: Antonio Ortiz (antonio.ortiz@nucleares.unam.mx)                *
 * Anatask to compute flatenicity (arXiv:2204.13733)                      *
 * Modified by: Gyula Bencedi (bencedi.gyula@wigner.hu)                   *
 **************************************************************************/

class TTree;

class AliPPVsMultUtils;
class AliESDtrackCuts;

#include "AliAnalysisManager.h"
#include "AliAnalysisTask.h"
#include "AliAnalysisTaskESDfilter.h"
#include "AliAnalysisTaskSE.h"
#include "AliAnalysisUtils.h"
#include "AliCentrality.h"
#include "AliESDEvent.h"
#include "AliESDInputHandler.h"
#include "AliESDUtils.h"
#include "AliESDVZERO.h"
#include "AliESDtrack.h"
#include "AliESDtrackCuts.h"
#include "AliEventCuts.h"
#include "AliGenCocktailEventHeader.h"
#include "AliGenEventHeader.h"
#include "AliInputEventHandler.h"
#include "AliLog.h"
#include "AliMCEvent.h"
#include "AliMCEventHandler.h"
#include "AliMCParticle.h"
#include "AliMultEstimator.h"
#include "AliMultInput.h"
#include "AliMultSelection.h"
#include "AliMultVariable.h"
#include "AliMultiplicity.h"
#include "AliOADBContainer.h"
#include "AliOADBMultSelection.h"
#include "AliPPVsMultUtils.h"
#include "AliStack.h"
#include "AliVEvent.h"
#include "AliVTrack.h"
#include "AliVVertex.h"
#include "TCanvas.h"
#include "TChain.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "THnSparse.h"
#include "TLegend.h"
#include "TList.h"
#include "TMath.h"
#include "TParticle.h"
#include "TProfile.h"
#include "TVector3.h"
#include <AliAnalysisFilter.h>
#include <AliESDVertex.h>
#include <AliHeader.h>
#include <AliMultiplicity.h>
#include <Riostream.h>
#include <TBits.h>
#include <TDirectory.h>
#include <TMath.h>
#include <TRandom.h>
#include <TTree.h>
using std::cout;
using std::endl;

#include "AliAnalysisTaskSpectraFlatenicity.h"

const Int_t nPtbinsFlatSpecFlatSpec = 36;
Double_t PtbinsFlatSpec[nPtbinsFlatSpecFlatSpec + 1] = {
    0.0, 0.1, 0.15, 0.2,  0.25, 0.3,  0.35, 0.4,  0.45, 0.5,  0.6, 0.7, 0.8,
    0.9, 1.0, 1.25, 1.5,  2.0,  2.5,  3.0,  3.5,  4.0,  4.5,  5.0, 6.0, 7.0,
    8.0, 9.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 30.0, 40.0, 50.0};

const Int_t nCent = 9;
Double_t centClassFlatSpec[nCent + 1] = {0.0, 1.0, 5.0, 10.0, 20.0, 30.0, 40.0, 50.0, 70.0, 100.0};

using namespace std; // std namespace: so you can do things like 'cout' etc

ClassImp(AliAnalysisTaskSpectraFlatenicity) // classimp: necessary for root

    AliAnalysisTaskSpectraFlatenicity::AliAnalysisTaskSpectraFlatenicity()
    : AliAnalysisTaskSE(), 
    fESD(0), 
    fEventCuts(0x0), 
    fMCStack(0), 
    fMC(0),
    fUseMC(kFALSE), 
    fV0Mindex(-1), 
    fmultV0A(-1), 
    fmultV0C(-1), 
    fmultTPC(-1), 
    fmultV0Amc(-1), 
    fmultV0Cmc(-1), 
    fmultTPCmc(-1), 
    fRemoveTrivialScaling(kFALSE), 
    fnGen(-1), 
    fPIDResponse(0x0), 
    fTrackFilter(0x0), 
    fOutputList(0), 
    fEtaCut(0.8), 
    fPtMin(0.5), 
    fv0mpercentile(0), 
    fFlat(-1), 
    fFlatMC(-1), 
    fMultSelection(0x0), 
    hPtPrimIn(0), 
    hPtPrimOut(0), 
    hPtSecOut(0), 
    hPtOut(0), 
    hFlatenicity(0), 
    hFlatenicityMC(0), 
    hFlatResponse(0), 
    hFlatVsPt(0), 
    hFlatVsPtMC(0), 
    hFlatVsNch(0), 
    hFlatVsNchMC(0), 
    hNchV0M(0), 
    hNchV0MMC(0), 
    hNchMidRap(0), 
    hNchMidRapMC(0), 
    hNchV0a(0), 
    hNchV0c(0), 
    hNchV0aMC(0), 
    hNchV0cMC(0), 
    hFlatVsV0M(0), 
    hEta(0), 
    hEtamc(0), 
    hCounter(0)
{
    for (Int_t i_c = 0; i_c < nCent; ++i_c) {
        hFlatVsPtV0M[i_c] = 0;
        hFlatVsNchTPCV0M[i_c] = 0;
    }
    for (Int_t i_c = 0; i_c < nCent; ++i_c) {
        hFlatVsPtV0MMC[i_c] = 0;
        hFlatVsNchTPCV0MMC[i_c] = 0;
    }
}

//_____________________________________________________________________________
AliAnalysisTaskSpectraFlatenicity::AliAnalysisTaskSpectraFlatenicity(const char *name)
    : AliAnalysisTaskSE(name), 
    fESD(0), 
    fEventCuts(0x0), 
    fMCStack(0), 
    fMC(0),
    fUseMC(kFALSE), 
    fV0Mindex(-1), 
    fmultV0A(-1), 
    fmultV0C(-1), 
    fmultTPC(-1), 
    fmultV0Amc(-1), 
    fmultV0Cmc(-1), 
    fmultTPCmc(-1), 
    fRemoveTrivialScaling(kFALSE), 
    fnGen(-1), 
    fPIDResponse(0x0), 
    fTrackFilter(0x0), 
    fOutputList(0), 
    fEtaCut(0.8), 
    fPtMin(0.5), 
    fv0mpercentile(0), 
    fFlat(-1), 
    fFlatMC(-1), 
    fMultSelection(0x0), 
    hPtPrimIn(0), 
    hPtPrimOut(0), 
    hPtSecOut(0), 
    hPtOut(0), 
    hFlatenicity(0), 
    hFlatenicityMC(0), 
    hFlatResponse(0), 
    hFlatVsPt(0), 
    hFlatVsPtMC(0), 
    hFlatVsNch(0), 
    hFlatVsNchMC(0), 
    hNchV0M(0), 
    hNchV0MMC(0), 
    hNchMidRap(0), 
    hNchMidRapMC(0), 
    hNchV0a(0), 
    hNchV0c(0), 
    hNchV0aMC(0), 
    hNchV0cMC(0), 
    hFlatVsV0M(0), 
    hEta(0), 
    hEtamc(0), 
    hCounter(0)
{
    for (Int_t i_c = 0; i_c < nCent; ++i_c) {
        hFlatVsPtV0M[i_c] = 0;
        hFlatVsNchTPCV0M[i_c] = 0;
    }
    for (Int_t i_c = 0; i_c < nCent; ++i_c) {
        hFlatVsPtV0MMC[i_c] = 0;
        hFlatVsNchTPCV0MMC[i_c] = 0;
    }

  DefineInput(0, TChain::Class()); // define the input of the analysis: in this
                                   // case you take a 'chain' of events
  DefineOutput(1, TList::Class()); // define the ouptut of the analysis: in this
                                   // case it's a list of histograms
}

//_____________________________________________________________________________
AliAnalysisTaskSpectraFlatenicity::~AliAnalysisTaskSpectraFlatenicity() {
  // destructor
  if (fOutputList) {
    delete fOutputList; // at the end of your task, it is deleted from memory by
                        // calling this function
    fOutputList = 0x0;
  }
}

//_____________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::UserCreateOutputObjects() {

  // create track filters
  fTrackFilter = new AliAnalysisFilter("trackFilter");
  AliESDtrackCuts *fCuts = new AliESDtrackCuts();
  fCuts->SetAcceptKinkDaughters(kFALSE);
  fCuts->SetRequireTPCRefit(kTRUE);
  fCuts->SetRequireITSRefit(kTRUE);
  fCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD, AliESDtrackCuts::kAny);
  fCuts->SetDCAToVertex2D(kFALSE);
  fCuts->SetRequireSigmaToVertex(kFALSE);
  fCuts->SetEtaRange(-0.8, 0.8);
  fCuts->SetMinNCrossedRowsTPC(70);
  fCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);
  fCuts->SetMaxChi2PerClusterTPC(4);
  fCuts->SetMaxDCAToVertexZ(2);
  fCuts->SetCutGeoNcrNcl(3., 130., 1.5, 0.85, 0.7);
  fCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);
  fCuts->SetMaxChi2PerClusterTPC(4);
  fCuts->SetMaxDCAToVertexZ(2);
  fCuts->SetMaxChi2PerClusterITS(36);
  fCuts->SetMaxDCAToVertexXYPtDep("0.0105+0.0350/pt^1.1");
  fCuts->SetMaxChi2PerClusterITS(36);
  fTrackFilter->AddCuts(fCuts);

  // create output objects

  OpenFile(1);
  fOutputList = new TList(); // this is a list which will contain all of your histograms
  fOutputList->SetOwner(kTRUE); // memory stuff: the list is owner of all

  hFlatenicity = new TH1D("hFlatenicity", "counter", 200, 0., 1.);
  fOutputList->Add(hFlatenicity);

  hEta = new TH1D("hEta", "Eta rec; #eta; counts", 200, -1.0, 1.0); hEta->Sumw2();
  fOutputList->Add(hEta);      
  
  hFlatVsPt = new TH2D("hFlatVsPt", "Measured; Flatenicity; #it{p}_{T} (GeV/#it{c})", 200, 0., 1., nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
  fOutputList->Add(hFlatVsPt);

  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    hFlatVsPtV0M[i_c] = new TH2D(Form("hFlatVsPtV0M_c%d", i_c), Form("Measured %1.0f-%1.0f%%V0M; Flatenicity; #it{p}_{T} (GeV/#it{c})",centClassFlatSpec[i_c], centClassFlatSpec[i_c + 1]),200, 0., 1., nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hFlatVsPtV0M[i_c]);
    hFlatVsNchTPCV0M[i_c] = new TH2D(Form("hFlatVsNchTPCV0M_c%d", i_c), Form("Measured %1.0f-%1.0f%%V0M; Flatenicity; #it{N}_{ch}",centClassFlatSpec[i_c], centClassFlatSpec[i_c + 1]),200, 0., 1., 100, -0.5, 99.5);
    fOutputList->Add(hFlatVsNchTPCV0M[i_c]);    
  }

  hNchMidRap = new TH1D("hNchMidRap", ";Nch; counts", 400, -0.5, 399.5);
  hNchMidRap->Sumw2();
  fOutputList->Add(hNchMidRap);
  
  if (fUseMC) {
      
    hEtamc = new TH1D("hEtamc", "Eta mc.; #eta; counts", 200, -1.0, 1.0); hEtamc->Sumw2();
    fOutputList->Add(hEtamc);      
    
    for (Int_t i_c = 0; i_c < nCent; ++i_c) {
        hFlatVsPtV0MMC[i_c] = new TH2D( Form("hFlatVsPtV0MMC_c%d", i_c), Form("Measured %1.0f-%1.0f%%V0M; Flatenicity; #it{p}_{T} (GeV/#it{c})",centClassFlatSpec[i_c], centClassFlatSpec[i_c + 1]), 200, 0., 1., nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
        fOutputList->Add(hFlatVsPtV0MMC[i_c]);
        hFlatVsNchTPCV0MMC[i_c] = new TH2D( Form("hFlatVsNchTPCV0MMC_c%d", i_c), Form("Measured %1.0f-%1.0f%%V0M; Flatenicity; #it{N}_{ch}",centClassFlatSpec[i_c], centClassFlatSpec[i_c + 1]), 200, 0., 1., 100, -0.5, 99.5);
        fOutputList->Add(hFlatVsNchTPCV0MMC[i_c]);        
    }

    hPtPrimIn = new TH1D("hPtPrimIn", "Prim In; #it{p}_{T} (GeV/#it{c}; counts)", nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hPtPrimIn);

    hPtPrimOut = new TH1D("hPtPrimOut", "Prim Out; #it{p}_{T} (GeV/#it{c}; counts)", nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hPtPrimOut);

    hPtSecOut = new TH1D("hPtSecOut", "Sec Out; #it{p}_{T} (GeV/#it{c}; counts)", nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hPtSecOut);

    hPtOut = new TH1D("hPtOut", "all Out; #it{p}_{T} (GeV/#it{c}; counts)", nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hPtOut);

    hFlatenicityMC = new TH1D("hFlatenicityMC", "counter", 200, 0., 1.);
    fOutputList->Add(hFlatenicityMC);

    hFlatResponse = new TH2D("hFlatResponse", "; true flat; measured flat", 200, 0., 1., 200, 0., 1.);
    fOutputList->Add(hFlatResponse);

    hFlatVsPtMC = new TH2D("hFlatVsPtMC", "MC true; Flatenicity; #it{p}_{T} (GeV/#it{c})", 200, 0., 1., nPtbinsFlatSpecFlatSpec, PtbinsFlatSpec);
    fOutputList->Add(hFlatVsPtMC);

    hFlatVsNchMC = new TH2D("hFlatVsNchMC", "; true flat; true Nch", 200, 0., 1., 100, -0.5, 99.5);
    fOutputList->Add(hFlatVsNchMC);
    
    /// Added V0M multiplicity distribtion 
    hNchV0MMC = new TH1D("hNchV0MMC", ";true Nch; counts", 400, -0.5, 399.5);
    hNchV0MMC->Sumw2();
    fOutputList->Add(hNchV0MMC);

    hNchMidRapMC = new TH1D("hNchMidRapMC", ";true Nch; counts", 400, -0.5, 399.5);
    hNchMidRapMC->Sumw2();
    fOutputList->Add(hNchMidRapMC);

    hNchV0aMC = new TH1D("hNchV0aMC", ";rec Nch; counts", 400, -0.5, 399.5);
    hNchV0aMC->Sumw2();
    fOutputList->Add(hNchV0aMC);

    hNchV0cMC = new TH1D("hNchV0cMC", ";rec Nch; counts", 400, -0.5, 399.5);
    hNchV0cMC->Sumw2();
    fOutputList->Add(hNchV0cMC);  
  
  }

  hFlatVsNch = new TH2D("hFlatVsNch", "; rec flat; rec Nch", 200, 0., 1., 100, -0.5, 99.5);
  fOutputList->Add(hFlatVsNch);
  
  hFlatVsV0M = new TH2D("hFlatVsV0M", "", nCent, centClassFlatSpec, 200, 0., 1.);
  fOutputList->Add(hFlatVsV0M);

  hNchV0M = new TH1D("hNchV0M", ";rec Nch; counts", 400, -0.5, 399.5);
  hNchV0M->Sumw2();
  fOutputList->Add(hNchV0M);

  hNchV0a = new TH1D("hNchV0a", ";rec Nch; counts", 400, -0.5, 399.5);
  hNchV0a->Sumw2();
  fOutputList->Add(hNchV0a);

  hNchV0c = new TH1D("hNchV0c", ";rec Nch; counts", 400, -0.5, 399.5);
  hNchV0c->Sumw2();
  fOutputList->Add(hNchV0c);  
  
  hCounter = new TH1D("hCounter", "counter", 10, -0.5, 9.5);
  fOutputList->Add(hCounter);

  fEventCuts.AddQAplotsToList(fOutputList);
  PostData(1, fOutputList); // postdata will notify the analysis manager of
                            // changes / updates to the
}

//_____________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::UserExec(Option_t *) {

  AliVEvent *event = InputEvent();
  if (!event) { Error("UserExec", "Could not retrieve event"); return; }

  fESD = dynamic_cast<AliESDEvent *>(event);
  if (!fESD) { Printf("%s:%d ESDEvent not found in Input Manager", (char *)__FILE__,__LINE__); this->Dump(); return; }

  if (fUseMC) {
    //      E S D
    fMC = dynamic_cast<AliMCEvent *>(MCEvent());
    if (!fMC) { Printf("%s:%d MCEvent not found in Input Manager", (char *)__FILE__,__LINE__); this->Dump(); return; }
    fMCStack = fMC->Stack();
  }

  AliHeader *headerMC;
  Bool_t isGoodVtxPosMC = kFALSE;

  if (fUseMC) {
    headerMC = fMC->Header();
    AliGenEventHeader *genHeader = headerMC->GenEventHeader();
    TArrayF vtxMC(3); // primary vertex  MC
    vtxMC[0] = 9999;
    vtxMC[1] = 9999;
    vtxMC[2] = 9999; // initialize with dummy
    if (genHeader) {
      genHeader->PrimaryVertex(vtxMC);
    }
    if (TMath::Abs(vtxMC[2]) <= 10)
      isGoodVtxPosMC = kTRUE;
  }

  // Trigger selection
  UInt_t fSelectMask = fInputHandler->IsEventSelected();
  Bool_t isINT7selected = fSelectMask & AliVEvent::kINT7;
  if (!isINT7selected)
    return;

  // Good events
  if (!fEventCuts.AcceptEvent(event)) {
    PostData(1, fOutputList);
    return;
  }

  // Good vertex
  Bool_t hasRecVertex = kFALSE;
  hasRecVertex = HasRecVertex();
  if (!hasRecVertex)
    return;

  // Multiplicity Estimation
  fv0mpercentile = -999;
  fMultSelection = (AliMultSelection *)fESD->FindListObject("MultSelection");
  if (!fMultSelection)
    cout << "------- No AliMultSelection Object Found --------" << fMultSelection << endl;
  fv0mpercentile = fMultSelection->GetMultiplicityPercentile("V0M");
  hCounter->Fill(1);

  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    if (fv0mpercentile >= centClassFlatSpec[i_c] && fv0mpercentile < centClassFlatSpec[i_c + 1]) {
      fV0Mindex = i_c;
    } else {
      continue;
    }
  }
/*
  // INEL>0 selection
	if( AliESDtrackCuts::GetReferenceMultiplicity(fESD, AliESDtrackCuts::kTracklets, 1.0) < 1. )
        return;
*/

  CheckMultiplicities();
  
  fFlat = GetFlatenicity();
  
  // DATA
  if (fFlat >= 0) {
    hFlatenicity->Fill(fFlat);
    if (fV0Mindex >= 0) {
      hFlatVsV0M->Fill(fv0mpercentile, fFlat);
      MakeDataanalysis();
    }
  }

  // MC
  if (fUseMC) {
    fFlatMC = GetFlatenicityMC();
    if (fFlatMC >= 0) {
        hFlatenicityMC->Fill(fFlatMC);
        hFlatResponse->Fill(fFlatMC, fFlat);
        MakeMCanalysis();
    }
    CheckMultiplicitiesMC();
  } // MC

  PostData(1, fOutputList); // stream the result of this event to the output
                            // manager which will write it to a file
}

//______________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::Terminate(Option_t *) {
}

//______________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::MakeDataanalysis() {

  // rec
  Int_t nTracks = fESD->GetNumberOfTracks();
  for (Int_t iT = 0; iT < nTracks; ++iT) {

    AliESDtrack *esdtrack = static_cast<AliESDtrack *>(fESD->GetTrack(iT)); // get a track (type AliesdTrack)
    if (!esdtrack)
      continue;
    if (!fTrackFilter->IsSelected(esdtrack))
      continue;
    if (TMath::Abs(esdtrack->Eta()) > fEtaCut)
      continue;
    if (esdtrack->Pt() < fPtMin)
      continue;
    
//     hFlatVsPt->Fill(fFlat, esdtrack->Pt());
//     hFlatVsPtV0M[fV0Mindex]->Fill(fFlat, esdtrack->Pt());
//     hEta->Fill(esdtrack->Eta());
//     hFlatVsNchTPCV0M[fV0Mindex]->Fill(fFlat, fmultTPC);
  }
}

//______________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::MakeMCanalysis() {

  for (Int_t i = 0; i < fMC->GetNumberOfTracks(); ++i) {

    AliMCParticle *particle = (AliMCParticle *)fMC->GetTrack(i);
    if (!particle)
      continue;
    if (!fMC->IsPhysicalPrimary(i))
      continue;
    if (TMath::Abs(particle->Eta()) > fEtaCut)
      continue;
    if (particle->Pt() < fPtMin)
      continue;
    if (TMath::Abs(particle->Charge()) < 0.1)
      continue;
    
    hFlatVsPtMC->Fill(fFlatMC, particle->Pt());
    hFlatVsPtV0MMC[fV0Mindex]->Fill(fFlatMC, particle->Pt());
    hPtPrimIn->Fill(particle->Pt());
    hEtamc->Fill(particle->Eta());
    hFlatVsNchTPCV0MMC[fV0Mindex]->Fill(fFlatMC, fmultTPCmc);
  }
  // rec
  Int_t nTracks = fESD->GetNumberOfTracks();
  for (Int_t iT = 0; iT < nTracks; ++iT) {

    AliESDtrack *esdtrack = static_cast<AliESDtrack *>(fESD->GetTrack(iT)); // get a track (type AliesdTrack)
    if (!esdtrack)
      continue;
    if (!fTrackFilter->IsSelected(esdtrack))
      continue;
    if (TMath::Abs(esdtrack->Eta()) > fEtaCut)
      continue;
    if (esdtrack->Pt() < fPtMin)
      continue;
    
    hPtOut->Fill(esdtrack->Pt());

    Int_t mcLabel = -1;
    mcLabel = TMath::Abs(esdtrack->GetLabel());
    
    if (fMC->IsPhysicalPrimary(mcLabel)) {
      hPtPrimOut->Fill(esdtrack->Pt());
    } else {
      hPtSecOut->Fill(esdtrack->Pt());
    }
  }
}

//______________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::CheckMultiplicitiesMC() {

  fmultV0Amc = 0;
  fmultV0Cmc = 0;
  fmultTPCmc = 0;

  for (Int_t i = 0; i < fMC->GetNumberOfTracks(); ++i) {

    AliMCParticle *particle = (AliMCParticle *)fMC->GetTrack(i);
    if (!particle)
      continue;
    if (!fMC->IsPhysicalPrimary(i))
      continue;
    if (particle->Pt() <= 0.0)
      continue;
    if (TMath::Abs(particle->Charge()) < 0.1)
      continue;

    Double_t eta_a = particle->Eta();
    if (eta_a >= 2.8 && eta_a < 4.5) { // v0a acceptance (excluding first ring)
      fmultV0Amc++;
    }
    if (eta_a >= -3.7 && eta_a < -1.7) { // v0c
      fmultV0Cmc++;
    }
    if (TMath::Abs(eta_a) < 0.8) { // adc
      fmultTPCmc++;
    }
  }
  
  hNchMidRapMC->Fill(fmultTPCmc);
  hNchV0aMC->Fill(fmultV0Amc);
  hNchV0cMC->Fill(fmultV0Cmc);
  
}

//______________________________________________________________________________
void AliAnalysisTaskSpectraFlatenicity::CheckMultiplicities() {

  fmultTPC = 0;
  Int_t nTracks = fESD->GetNumberOfTracks();
  for (Int_t iT = 0; iT < nTracks; ++iT) {
    AliESDtrack *esdtrack = static_cast<AliESDtrack *>(fESD->GetTrack(iT)); // get a track (type AliesdTrack)
    if (!esdtrack)
      continue;
    if (!fTrackFilter->IsSelected(esdtrack))
      continue;
    if (TMath::Abs(esdtrack->Eta()) > fEtaCut)
      continue;
    if (esdtrack->Pt() < fPtMin)
      continue;
    fmultTPC++;
  }

  AliVVZERO *lVV0 = 0x0;
  AliVEvent *lVevent = 0x0;
  lVevent = dynamic_cast<AliVEvent *>(InputEvent());
  if (!lVevent) {
    AliWarning("ERROR: ESD / AOD event not available \n");
    return;
  }
  // Get VZERO Information for multiplicity later
  lVV0 = lVevent->GetVZEROData();
  if (!lVV0) {
    AliError("AliVVZERO not available");
    return;
  }

  const Int_t nChannels = 64;
  fmultV0C = 0;
  fmultV0A = 0;
  for (Int_t iCh = 0; iCh < nChannels; iCh++) {
    Float_t mult = lVV0->GetMultiplicity(iCh);
    if (iCh < 32) { // V0C
      fmultV0C += mult;
    } else if (iCh >= 32 &&
               iCh < 40) { // exclude first ring to avoid overlap with ADA
      continue;
    } else { // V0A
      fmultV0A += mult;
    }
  }
  
  hNchMidRap->Fill(fmultTPC);
  hNchV0a->Fill(fmultV0A);
  hNchV0c->Fill(fmultV0C);
}

//______________________________________________________________________________
Double_t AliAnalysisTaskSpectraFlatenicity::GetFlatenicity() {

  AliVVZERO *lVV0 = 0x0;
  AliVEvent *lVevent = 0x0;
  lVevent = dynamic_cast<AliVEvent *>(InputEvent());
  if (!lVevent) {
    AliWarning("ERROR: ESD / AOD event not available \n");
    return -1;
  }
  // Get VZERO Information for multiplicity later
  lVV0 = lVevent->GetVZEROData();
  if (!lVV0) {
    AliError("AliVVZERO not available");
    return -1;
  }
  // Flatenicity calculation
  const Int_t nRings = 4;
  const Int_t nSectors = 8;
  Float_t minEtaV0C[nRings] = {-3.7, -3.2, -2.7, -2.2};
  Float_t maxEtaV0C[nRings] = {-3.2, -2.7, -2.2, -1.7};
  Float_t maxEtaV0A[nRings] = {5.1, 4.5, 3.9, 3.4};
  Float_t minEtaV0A[nRings] = {4.5, 3.9, 3.4, 2.8};
  
  // Grid
  const Int_t nCells = nRings * 2 * nSectors;
  Float_t RhoLattice[nCells];
  Float_t multLattice[nCells];
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    RhoLattice[iCh] = 0.0;
    multLattice[iCh] = 0.0;
  }

  Int_t nringA = 0;
  Int_t nringC = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    Float_t detaV0 = -1;
    Float_t mult = lVV0->GetMultiplicity(iCh);
    if (iCh < 32) { // V0C
      if (iCh < 8) {
        nringC = 0;
      } else if (iCh >= 8 && iCh < 16) {
        nringC = 1;
      } else if (iCh >= 16 && iCh < 24) {
        nringC = 2;
      } else {
        nringC = 3;
      }
      detaV0 = maxEtaV0C[nringC] - minEtaV0C[nringC];
    } else { // V0A
      if (iCh < 40) {
        nringA = 0;
      } else if (iCh >= 40 && iCh < 48) {
        nringA = 1;
      } else if (iCh >= 48 && iCh < 56) {
        nringA = 2;
      } else {
        nringA = 3;
      }
      detaV0 = maxEtaV0A[nringA] - minEtaV0A[nringA];
    }
    RhoLattice[iCh] = mult / detaV0; // needed to consider the different eta coverage
    multLattice[iCh] = mult;
  }

  Float_t mRho = 0;
  Float_t multRho = 0;
  Float_t flatenicity = -1;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    mRho    += RhoLattice[iCh];
    multRho += multLattice[iCh];
  }
  Float_t multV0Mdeta = mRho;
  Float_t multV0M = multRho;

  // average activity per cell
  mRho /= (1.0 * nCells);
  
  // get sigma
  Double_t sRho_tmp = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    sRho_tmp += TMath::Power(1.0 * RhoLattice[iCh] - mRho, 2);
  }
  sRho_tmp /= (1.0 * nCells * nCells);
  Float_t sRho = TMath::Sqrt(sRho_tmp);
  if (mRho > 0) {
    if (fRemoveTrivialScaling) {
      flatenicity = TMath::Sqrt(multV0M) * sRho / mRho; // scaling by absolute tot mult
    } else {
      flatenicity = sRho / mRho;
    }
  } else {
    flatenicity = -1;
  }
  
  hFlatVsNch->Fill(flatenicity, multV0M);
  hNchV0M->Fill(multV0M);

  return flatenicity;
}

//______________________________________________________________________________
Double_t AliAnalysisTaskSpectraFlatenicity::GetFlatenicityMC() {

  // Flatenicity calculation
  const Int_t nRings = 8;
  Float_t maxEta[nRings] = {-3.2, -2.7, -2.2, -1.7, 5.1, 4.5, 3.9, 3.4};
  Float_t minEta[nRings] = {-3.7, -3.2, -2.7, -2.2, 4.5, 3.9, 3.4, 2.8};

  const Int_t nSectors = 8;
  Float_t PhiBins[nSectors + 1];
  Float_t deltaPhi = (2.0 * TMath::Pi()) / (1.0 * nSectors);
  for (int i_phi = 0; i_phi < nSectors + 1; ++i_phi) {
    PhiBins[i_phi] = 0;
    if (i_phi < nSectors) {
      PhiBins[i_phi] = i_phi * deltaPhi;
    } else {
      PhiBins[i_phi] = 2.0 * TMath::Pi();
    }
  }

  // Grid
  const Int_t nCells = nRings * nSectors;
  Float_t RhoLattice[nCells];
  Float_t multLattice[nCells];
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    RhoLattice[iCh] = 0.0;
    multLattice[iCh] = 0.0;
  }

  Int_t nMult = 0;
  for (Int_t i = 0; i < fMC->GetNumberOfTracks(); ++i) {

    AliMCParticle *particle = (AliMCParticle *)fMC->GetTrack(i);
    if (!particle)
      continue;
    if (!fMC->IsPhysicalPrimary(i))
      continue;
    if (particle->Pt() <= 0.0)
      continue;
    if (TMath::Abs(particle->Charge()) < 0.1)
      continue;
    
    Double_t phi = particle->Phi();
    Double_t eta = particle->Eta();

    Int_t i_segment = 0;
    for (int i_eta = 0; i_eta < nRings; ++i_eta) {

      for (int i_phi = 0; i_phi < nSectors; ++i_phi) {

        if (eta >= minEta[i_eta] && eta < maxEta[i_eta] &&
            phi >= PhiBins[i_phi] && phi < PhiBins[i_phi + 1]) {
          nMult++;
          RhoLattice[i_segment] += 1.0;
          multLattice[i_segment] += 1.0;
        }
        i_segment++;
      }
    }
  }

  Int_t i_seg = 0;
  for (int i_eta = 0; i_eta < nRings; ++i_eta) {
    for (int i_phi = 0; i_phi < nSectors; ++i_phi) {
      Float_t deltaEta = TMath::Abs(maxEta[i_eta] - minEta[i_eta]);
      RhoLattice[i_seg] /= deltaEta;
      i_seg++;
    }
  }

  Float_t mRho = 0;
  Float_t multRho = 0;
  Float_t flatenicity = -1;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    mRho    += RhoLattice[iCh];
    multRho += multLattice[iCh];
  }
  Float_t multiplicityV0M = mRho;
  Float_t multV0M = multRho;

  // average activity per cell
  mRho /= (1.0 * nCells);

  // get sigma
  Float_t sRho_tmp = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    sRho_tmp += TMath::Power(1.0 * RhoLattice[iCh] - mRho, 2);
  }
  sRho_tmp /= (1.0 * nCells * nCells);
  Float_t sRho = TMath::Sqrt(sRho_tmp);
  if (mRho > 0) {
    if (fRemoveTrivialScaling) {
      flatenicity = TMath::Sqrt(1.0 * multV0M) * sRho / mRho;
    } else {
      flatenicity = sRho / mRho;
    }
  } else {
    sRho = -1;
  }
  
  hFlatVsNchMC->Fill(flatenicity, nMult);
  hNchV0MMC->Fill(nMult);
  
  return flatenicity;
}

//______________________________________________________________________________
Bool_t AliAnalysisTaskSpectraFlatenicity::HasRecVertex() {

  float fMaxDeltaSpdTrackAbsolute = 0.5f;
  float fMaxDeltaSpdTrackNsigmaSPD = 1.e14f;
  float fMaxDeltaSpdTrackNsigmaTrack = 1.e14;
  float fMaxResolutionSPDvertex = 0.25f;
  float fMaxDispersionSPDvertex = 1.e14f;

  Bool_t fRequireTrackVertex = true;
  unsigned long fFlag;
  fFlag = BIT(AliEventCuts::kNoCuts);

  const AliVVertex *vtTrc = fESD->GetPrimaryVertex();
  bool isTrackV = true;
  if (vtTrc->IsFromVertexer3D() || vtTrc->IsFromVertexerZ())
    isTrackV = false;
  const AliVVertex *vtSPD = fESD->GetPrimaryVertexSPD();

  if (vtSPD->GetNContributors() > 0)
    fFlag |= BIT(AliEventCuts::kVertexSPD);

  if (vtTrc->GetNContributors() > 1 && isTrackV)
    fFlag |= BIT(AliEventCuts::kVertexTracks);

  if (((fFlag & BIT(AliEventCuts::kVertexTracks)) || !fRequireTrackVertex) &&
      (fFlag & BIT(AliEventCuts::kVertexSPD)))
    fFlag |= BIT(AliEventCuts::kVertex);

  const AliVVertex *&vtx =
      bool(fFlag & BIT(AliEventCuts::kVertexTracks)) ? vtTrc : vtSPD;
  AliVVertex *fPrimaryVertex = const_cast<AliVVertex *>(vtx);
  if (!fPrimaryVertex)
    return kFALSE;

  /// Vertex quality cuts
  double covTrc[6], covSPD[6];
  vtTrc->GetCovarianceMatrix(covTrc);
  vtSPD->GetCovarianceMatrix(covSPD);
  double dz = bool(fFlag & AliEventCuts::kVertexSPD) &&
                      bool(fFlag & AliEventCuts::kVertexTracks)
                  ? vtTrc->GetZ() - vtSPD->GetZ()
                  : 0.; /// If one of the two vertices is not available this cut
                        /// is always passed.
  double errTot = TMath::Sqrt(covTrc[5] + covSPD[5]);
  double errTrc =
      bool(fFlag & AliEventCuts::kVertexTracks) ? TMath::Sqrt(covTrc[5]) : 1.;
  double nsigTot = TMath::Abs(dz) / errTot, nsigTrc = TMath::Abs(dz) / errTrc;
  /// vertex dispersion for run1, only for ESD, AOD code to be added here
  const AliESDVertex *vtSPDESD = dynamic_cast<const AliESDVertex *>(vtSPD);
  double vtSPDdispersion = vtSPDESD ? vtSPDESD->GetDispersion() : 0;
  if ((TMath::Abs(dz) <= fMaxDeltaSpdTrackAbsolute &&
       nsigTot <= fMaxDeltaSpdTrackNsigmaSPD &&
       nsigTrc <=
           fMaxDeltaSpdTrackNsigmaTrack) && // discrepancy track-SPD vertex
      (!vtSPD->IsFromVertexerZ() ||
       TMath::Sqrt(covSPD[5]) <= fMaxResolutionSPDvertex) &&
      (!vtSPD->IsFromVertexerZ() ||
       vtSPDdispersion <= fMaxDispersionSPDvertex) /// vertex dispersion cut for
                                                   /// run1, only for ESD
      ) // quality cut on vertexer SPD z
    fFlag |= BIT(AliEventCuts::kVertexQuality);

  Bool_t hasVtx = (TESTBIT(fFlag, AliEventCuts::kVertex)) &&
                  (TESTBIT(fFlag, AliEventCuts::kVertexQuality));

  return hasVtx;
}
