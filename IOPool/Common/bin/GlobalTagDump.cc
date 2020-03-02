
#include "DataFormats/Provenance/interface/History.h"
#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include "DataFormats/Provenance/interface/ParameterSetID.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"

#include "FWCore/Utilities/interface/Algorithms.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "TError.h"
#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <map>
#include <vector>
#include <sstream>


namespace {
  typedef std::map<edm::ParameterSetID, edm::ParameterSetBlob> ParameterSetMap;

  class HistoryNode {
  public:
    HistoryNode() : config_(), simpleId_(0) {}

    HistoryNode(edm::ProcessConfiguration const& iConfig, unsigned int iSimpleId)
        : config_(iConfig), simpleId_(iSimpleId) {}

    void addChild(HistoryNode const& child) { children_.push_back(child); }

    edm::ParameterSetID const& parameterSetID() const { return config_.parameterSetID(); }

    std::string const& processName() const { return config_.processName(); }

    std::size_t size() const { return children_.size(); }

    HistoryNode* lastChildAddress() { return &children_.back(); }

     typedef std::vector<HistoryNode>::const_iterator const_iterator;
    typedef std::vector<HistoryNode>::iterator iterator;

    iterator begin() { return children_.begin(); }
    iterator end() { return children_.end(); }

     const_iterator begin() const { return children_.begin(); }
     const_iterator end() const { return children_.end(); }

     //void printHistory(std::string const& iIndent = std::string("  ")) const;
    void printEventSetupHistory(ParameterSetMap const& iPSM) const;     

     edm::ProcessConfigurationID configurationID() const { return config_.id(); }

  private:
     edm::ProcessConfiguration config_;
     std::vector<HistoryNode> children_;
     unsigned int simpleId_;
  };

void HistoryNode::printEventSetupHistory(ParameterSetMap const& iPSM) const {
   for (auto const& itH : *this)
   {
      //Get ParameterSet for process
      ParameterSetMap::const_iterator itFind = iPSM.find(itH.parameterSetID());
      if (itFind == iPSM.end()) {
         std::cout << "No ParameterSetID for " << itH.parameterSetID() << std::endl;
      } else {
       
         edm::ParameterSet processConfig(itFind->second.pset());
         std::vector<std::string> sourceStrings, moduleStrings;
      
         //get the sources
         std::vector<std::string> sources = processConfig.getParameter<std::vector<std::string>>("@all_essources");
         for (auto& itM : sources)
         {            
            edm::ParameterSet const& pset = processConfig.getParameterSet(itM);            
            std::string name(pset.getParameter<std::string>("@module_label"));
            if (name.empty()) {
               name = pset.getParameter<std::string>("@module_type");
            }            
            if (name != "GlobalTag" )
               continue;            

            std::cout << "GlobalTag for processName "  << itH.processName() << "\n";

            for (auto const& item : pset.tbl()) {
               if (item.first == "globaltag")
                  std::cout << " =====" << item.first << ": " << item.second << std::endl;
            }            
         }      
      }
      itH.printEventSetupHistory(iPSM);
   }
}
}  // namespace

//______________________________________________________________________________
//==============================================================================
//==============================================================================


int main(int argc, char* argv[]) {
   printf("inputFile %s\n", argv[1]);

   TFile* inputFile_ = TFile::Open(argv[1]);
   TTree* meta = dynamic_cast<TTree*>(inputFile_->Get("MetaData"));
   assert(nullptr != meta);

   typedef std::map<edm::ParameterSetID, edm::ParameterSetBlob> ParameterSetMap;
   ParameterSetMap psm_;
   TTree* psetTree = dynamic_cast<TTree*>(inputFile_->Get("ParameterSets"));
   typedef std::pair<edm::ParameterSetID, edm::ParameterSetBlob> IdToBlobs;
   IdToBlobs idToBlob;
   IdToBlobs* pIdToBlob = &idToBlob;
   psetTree->SetBranchAddress("IdToParameterSetsBlobs", &pIdToBlob);
   for (long long i = 0; i != psetTree->GetEntries(); ++i) {
      psetTree->GetEntry(i);
      psm_.insert(idToBlob);
   }
  
   edm::ProcessHistoryVector phv_;
   edm::ProcessHistoryVector* pPhv = &phv_;
   meta->SetBranchAddress("ProcessHistory", &pPhv);
 
 
   meta->GetEntry(0);
  
   edm::pset::Registry& psetRegistry = *edm::pset::Registry::instance();
   for (auto const& item : psm_) {
      edm::ParameterSet pset(item.second.pset());
      pset.setID(item.first);
      psetRegistry.insertMapped(pset);
   }

   //==============================================================================
   //==============================================================================
   // ProvenanceDumper::dumpProcessHistory_
   HistoryNode historyGraph_;
   std::map<edm::ProcessConfigurationID, unsigned int> simpleIDs;
   for (auto const& ph : phv_) {
      //loop over the history entries looking for matches
      //  std::cout << "AMT loop ProcessHistory :" << ph <<  std::endl;
      HistoryNode* parent = &historyGraph_;
      
      for (auto const& pc : ph) {
         if (parent->size() == 0) {
            // std::cout << "AMT dumpProcessHistory 2:" << pc <<  std::endl;
            unsigned int id = simpleIDs[pc.id()];
            if (0 == id) {
               id = 1;
               simpleIDs[pc.id()] = id;
            }
            parent->addChild(HistoryNode(pc, id));
            parent = parent->lastChildAddress();
         }
         else {
            // std::cout << "AMT dumpProcessHistory 3:" << pc <<  std::endl;
            //see if this is unique
            bool isUnique = true;
            for (auto& child : *parent) {
               if (child.configurationID() == pc.id()) {
                  isUnique = false;
                  parent = &child;
                  break;
               }
            }
            if (isUnique) {
               simpleIDs[pc.id()] = parent->size() + 1;
               parent->addChild(HistoryNode(pc, simpleIDs[pc.id()]));
               parent = parent->lastChildAddress();
            }
         }
      }
   }
  
   historyGraph_.printEventSetupHistory(psm_);
}
