#include "DataFormats/Common/interface/setIsMergeable.h"
#include "DataFormats/Provenance/interface/BranchType.h"
#include "DataFormats/Provenance/interface/EventSelectionID.h"
#include "DataFormats/Provenance/interface/History.h"
#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include "DataFormats/Provenance/interface/ParameterSetID.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/Parentage.h"
#include "DataFormats/Provenance/interface/ProductProvenance.h"
#include "DataFormats/Provenance/interface/StoredProductProvenance.h"
#include "DataFormats/Provenance/interface/ParentageRegistry.h"
// #include "FWCore/Catalog/interface/InputFileCatalog.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
// #include "FWCore/Services/src/SiteLocalConfigService.h"

#include "FWCore/Utilities/interface/Algorithms.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/propagate_const.h"

#include "TError.h"
#include "TFile.h"
#include "TTree.h"

#include "boost/program_options.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <vector>

typedef std::map<std::string, std::vector<edm::BranchDescription>> IdToBranches;
typedef std::map<std::pair<std::string, std::string>, IdToBranches> ModuleToIdBranches;

static std::ostream& prettyPrint(std::ostream& oStream,
                                 edm::ParameterSet const& iPSet,
                                 std::string const& iIndent,
                                 std::string const& iIndentDelta);

static std::string const triggerResults = std::string("TriggerResults");
static std::string const triggerPaths = std::string("@trigger_paths");
static std::string const source = std::string("source");
static std::string const input = std::string("@main_input");

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

    void print(std::ostream& os) const {
      os << config_.processName() << " (=============================) '" << config_.passID() << "' ' ============================" << config_.releaseVersion() << "' [ =======================)" << simpleId_
         << " =============================)]  (" << config_.parameterSetID() << "=============================)" << std::endl;
    }

    void printHistory(std::string const& iIndent = std::string("  ")) const;
    void printEventSetupHistory(ParameterSetMap const& iPSM,
                                std::vector<std::string> const& iFindMatch,
                                std::ostream& oErrorLog) const;     

    edm::ProcessConfigurationID configurationID() const { return config_.id(); }

     //    static bool sort_;

  private:
    edm::ProcessConfiguration config_;
    std::vector<HistoryNode> children_;
    unsigned int simpleId_;
  };

  std::ostream& operator<<(std::ostream& os, HistoryNode const& node) {
    node.print(os);
    return os;
  }
//  bool HistoryNode::sort_ = false;
}  // namespace

std::ostream& operator<<(std::ostream& os, edm::ProcessHistory& iHist) {
  std::string const indentDelta("  ");
  std::string indent = indentDelta;
  for (auto const& process : iHist) {
    os << indent << process.processName() << " ------- '" << process.passID() << "' -------------'" << process.releaseVersion() << "----------------' ("
       << process.parameterSetID() << "--------------------)" << std::endl;
    indent += indentDelta;
  }
  return os;
}

void HistoryNode::printHistory(std::string const& iIndent) const {
  std::string const indentDelta(" --- ");
  const std::string& indent = iIndent;
  for (auto const& item : *this) {
     std::cout << "rrrrrrrrrrrrrrrrrrrr"<< indent << item;
    item.printHistory(indent + indentDelta);
  }
}

std::string eventSetupComponent(char const* iType,
                                std::string const& iCompName,
                                edm::ParameterSet const& iProcessConfig,
                                std::string const& iProcessName) {

      
  std::ostringstream result;


  if (strcmp(iType, "ESSource")) return result.str();
  
  edm::ParameterSet const& pset = iProcessConfig.getParameterSet(iCompName);
  std::string name(pset.getParameter<std::string>("@module_label"));
  if (name.empty()) {
    name = pset.getParameter<std::string>("@module_type");
  }
  if (name != "GlobalTag" )
     return result.str();
  
  result << "AMT psc " << iType << ": " << name << " " << iProcessName << "\n"
         << " parameters: ";
  prettyPrint(result, pset, " ", " ");
  return result.str();
}

void HistoryNode::printEventSetupHistory(ParameterSetMap const& iPSM,
                                         std::vector<std::string> const& iFindMatch,
                                         std::ostream& oErrorLog) const {
  for (auto const& itH : *this) {
    //Get ParameterSet for process
    ParameterSetMap::const_iterator itFind = iPSM.find(itH.parameterSetID());
    if (itFind == iPSM.end()) {
      oErrorLog << "No ParameterSetID for " << itH.parameterSetID() << std::endl;
    } else {
       // std::cout << "AMT HistoryNode::printEventSetupHistory from  ParameterSetMap " << itFind->first << "std::endl";
       
      edm::ParameterSet processConfig(itFind->second.pset());
      std::vector<std::string> sourceStrings, moduleStrings;
      
      //get the sources
      std::vector<std::string> sources = processConfig.getParameter<std::vector<std::string>>("@all_essources");
      for (auto& itM : sources) {

         // printf("---going to call eventSetupComponent  AMT HistoryNode::printEventSetupHistory ddd ESSource printEventSetupHistory ESSource %s %s \n", itM.c_str(), itH.processName().c_str());
    
        std::string retValue = eventSetupComponent("ESSource", itM, processConfig, itH.processName());
         sourceStrings.push_back(std::move(retValue));
      }
      
      std::copy(sourceStrings.begin(), sourceStrings.end(), std::ostream_iterator<std::string>(std::cout, "  \n"));
    }
    // printf("AMT printEventSetupHistory match %s \n", iFindMatch.c_str());
    itH.printEventSetupHistory(iPSM, iFindMatch, oErrorLog);
  }
}

 // namespace
static std::ostream& prettyPrint(std::ostream& oStream,
                                 edm::ParameterSet const& iPSet,
                                 std::string const& iIndent,
                                 std::string const& iIndentDelta) {
  std::string newIndent = iIndent + iIndentDelta;

  //oStream << "{" << std::endl;
  for (auto const& item : iPSet.tbl()) {
    // indent a bit
     if (item.first == "globaltag")
    oStream << newIndent << item.first << ": " << item.second <<  "uuuuuu-0-u" << std::endl;
  }

  return oStream;
}

class ProvenanceDumper {
public:
  // It is illegal to call this constructor with a null pointer; a
  // legal C-style string is required.
  ProvenanceDumper(std::string const& filename,
                   bool showDependencies,
                   bool extendedAncestors,
                   bool extendedDescendants,
                   bool excludeESModules,
                   bool showAllModules,
                   bool showTopLevelPSets,
                   std::vector <std::string> const& findMatch,
                   bool dontPrintProducts,
                   std::string const& dumpPSetID);

  ProvenanceDumper(ProvenanceDumper const&) = delete;             // Disallow copying and moving
  ProvenanceDumper& operator=(ProvenanceDumper const&) = delete;  // Disallow copying and moving

  // Write the provenenace information to the given stream.
  void dump();
  void printErrors(std::ostream& os);
  int exitCode() const;

private:

  std::string filename_;
  TFile* inputFile_;
  int exitCode_;
  std::stringstream errorLog_;
  int errorCount_;
   //  edm::ProductRegistry reg_;
  edm::ProcessConfigurationVector phc_;
  edm::ProcessHistoryVector phv_;
  ParameterSetMap psm_;
  HistoryNode historyGraph_;
  bool showDependencies_;
  bool extendedAncestors_;
  bool extendedDescendants_;
  bool excludeESModules_;
  bool showOtherModules_;
  bool productRegistryPresent_;
  bool showTopLevelPSets_;
  std::vector<std::string> findMatch_;
  bool dontPrintProducts_;
  std::string dumpPSetID_;

  void work_();
  void dumpProcessHistory_();
   //  void dumpEventFilteringParameterSets_(TFile* file);
   //void dumpEventFilteringParameterSets(edm::EventSelectionIDVector const& ids);
   // void dumpParameterSetForID_(edm::ParameterSetID const& id);
};

ProvenanceDumper::ProvenanceDumper(std::string const& filename,
                                   bool showDependencies,
                                   bool extendedAncestors,
                                   bool extendedDescendants,
                                   bool excludeESModules,
                                   bool showOtherModules,
                                   bool showTopLevelPSets,
                                   std::vector<std::string> const& findMatch,
                                   bool dontPrintProducts,
                                   std::string const& dumpPSetID)
    : filename_(filename),
      inputFile_(nullptr                   ),
      exitCode_(0),
      errorLog_(),
      errorCount_(0),
      showDependencies_(showDependencies),
      extendedAncestors_(extendedAncestors),
      extendedDescendants_(extendedDescendants),
      excludeESModules_(excludeESModules),
      showOtherModules_(showOtherModules),
      productRegistryPresent_(true),
      showTopLevelPSets_(showTopLevelPSets),
      findMatch_(findMatch),
      dontPrintProducts_(dontPrintProducts),
      dumpPSetID_(dumpPSetID) {inputFile_ = TFile::Open(filename.c_str());}

void ProvenanceDumper::dump() { work_(); }

void ProvenanceDumper::printErrors(std::ostream& os) {
  if (errorCount_ > 0)
    os << errorLog_.str() << std::endl;
}

int ProvenanceDumper::exitCode() const { return exitCode_; }

void ProvenanceDumper::dumpProcessHistory_() {
  std::cout << "Processing History:" << std::endl;
  std::map<edm::ProcessConfigurationID, unsigned int> simpleIDs;
  for (auto const& ph : phv_) {
    //loop over the history entries looking for matches
     std::cout << "AMT dumpProcessHistory 1:" << ph <<  std::endl;
    HistoryNode* parent = &historyGraph_;
    for (auto const& pc : ph) {
      if (parent->size() == 0) {
     std::cout << "AMT dumpProcessHistory 2:" << pc <<  std::endl;
        unsigned int id = simpleIDs[pc.id()];
        if (0 == id) {
          id = 1;
          simpleIDs[pc.id()] = id;
        }
        parent->addChild(HistoryNode(pc, id));
        parent = parent->lastChildAddress();
      } else {
     std::cout << "AMT dumpProcessHistory 3grep A:" << pc <<  std::endl;
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
  historyGraph_.printHistory();
}

void ProvenanceDumper::work_() {

   printf("ffffffffffffffffffffffffffffffffffffffffff 0000\n");
   TTree* meta = dynamic_cast<TTree*>(inputFile_->Get(edm::poolNames::metaDataTreeName().c_str()));
   assert(nullptr != meta);

   //  ParameterSetMap* pPsm = &psm_;
   TTree* psetTree = dynamic_cast<TTree*>(inputFile_->Get(edm::poolNames::parameterSetsTreeName().c_str()));
   assert(nullptr != psetTree);
   typedef std::pair<edm::ParameterSetID, edm::ParameterSetBlob> IdToBlobs;
   IdToBlobs idToBlob;
   IdToBlobs* pIdToBlob = &idToBlob;
   psetTree->SetBranchAddress(edm::poolNames::idToParameterSetBlobsBranchName().c_str(), &pIdToBlob);
    
   for (long long i = 0; i != psetTree->GetEntries(); ++i) {
      psetTree->GetEntry(i);
      psm_.insert(idToBlob);
   }
  
   edm::ProcessHistoryVector* pPhv = &phv_;
   meta->SetBranchAddress(edm::poolNames::processHistoryBranchName().c_str(), &pPhv);
 
 
   meta->GetEntry(0);
  
   edm::pset::Registry& psetRegistry = *edm::pset::Registry::instance();
   for (auto const& item : psm_) {
      edm::ParameterSet pset(item.second.pset());
      pset.setID(item.first);
      psetRegistry.insertMapped(pset);
   }
 
   if (!phv_.empty()) {
      for (auto const& history : phv_) {
         for (auto const& process : history) {
            phc_.push_back(process);
         }
      }
      edm::sort_all(phc_);
      phc_.erase(std::unique(phc_.begin(), phc_.end()), phc_.end());

   }

   dumpProcessHistory_();
   historyGraph_.printEventSetupHistory(psm_, findMatch_, errorLog_);
}

static char const* const kSortOpt = "sort";
static char const* const kSortCommandOpt = "sort,s";
static char const* const kDependenciesOpt = "dependencies";
static char const* const kDependenciesCommandOpt = "dependencies,d";
static char const* const kExtendedAncestorsOpt = "extendedAncestors";
static char const* const kExtendedAncestorsCommandOpt = "extendedAncestors,x";
static char const* const kExtendedDescendantsOpt = "extendedDescendants";
static char const* const kExtendedDescendantsCommandOpt = "extendedDescendants,c";
static char const* const kExcludeESModulesOpt = "excludeESModules";
static char const* const kExcludeESModulesCommandOpt = "excludeESModules,e";
static char const* const kShowAllModulesOpt = "showAllModules";
static char const* const kShowAllModulesCommandOpt = "showAllModules,a";
static char const* const kFindMatchOpt = "findMatch";
static char const* const kFindMatchCommandOpt = "findMatch,f";
static char const* const kDontPrintProductsOpt = "dontPrintProducts";
static char const* const kDontPrintProductsCommandOpt = "dontPrintProducts,p";
static char const* const kShowTopLevelPSetsOpt = "showTopLevelPSets";
static char const* const kShowTopLevelPSetsCommandOpt = "showTopLevelPSets,t";
static char const* const kHelpOpt = "help";
static char const* const kHelpCommandOpt = "help,h";
static char const* const kFileNameOpt = "input-file";
static char const* const kDumpPSetIDOpt = "dumpPSetID";
static char const* const kDumpPSetIDCommandOpt = "dumpPSetID,i";

int main(int argc, char* argv[]) {
  using namespace boost::program_options;

  std::string descString(argv[0]);
  descString += " [options] <filename>";
  descString += "\nAllowed options";
  options_description desc(descString);
  desc.add_options()(kHelpCommandOpt, "show help message")(kSortCommandOpt, "alphabetially sort EventSetup components")(
      kDependenciesCommandOpt, "print what data each EDProducer is directly dependent upon")(
      kExtendedAncestorsCommandOpt, "print what data each EDProducer is dependent upon including indirect dependences")(
      kExtendedDescendantsCommandOpt,
      "print what data depends on the data each EDProducer produces including indirect dependences")(
      kExcludeESModulesCommandOpt, "do not print ES module information")(
      kShowAllModulesCommandOpt, "show all modules (not just those that created data in the file)")(
      kShowTopLevelPSetsCommandOpt, "show all top level PSets")(
      kFindMatchCommandOpt,
      boost::program_options::value<std::vector<std::string>>(),
      "show only modules whose information contains the matching string (or all the matching strings, this option can "
      "be repeated with different strings)")(kDontPrintProductsCommandOpt, "do not print products produced by module")(
      kDumpPSetIDCommandOpt,
      value<std::string>(),
      "print the parameter set associated with the parameter set ID string (and print nothing else)");
  //we don't want users to see these in the help messages since this
  // name only exists since the parser needs it
  options_description hidden;
  hidden.add_options()(kFileNameOpt, value<std::string>(), "file name");

  //full list of options for the parser
  options_description cmdline_options;
  cmdline_options.add(desc).add(hidden);

  positional_options_description p;
  p.add(kFileNameOpt, -1);

  variables_map vm;
  try {
    store(command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    notify(vm);
  } catch (error const& iException) {
    std::cerr << iException.what();
    return 1;
  }
  std::string fileName;
  if (vm.count(kFileNameOpt)) {
    try {
      fileName = vm[kFileNameOpt].as<std::string>();
    } catch (boost::bad_any_cast const& e) {
      std::cout << e.what() << std::endl;
      return 2;
    }
  } else {
    std::cout << "Data file not specified." << std::endl;
    std::cout << desc << std::endl;
    return 2;
  }
  printf("ffffffffffffffffffffffffffffff 5555\n");
  std::vector<std::string> findMatch;
  std::string dumpPSetID;
  ProvenanceDumper dumper(fileName,false, false, false, false, false, false, findMatch, false, dumpPSetID);
  printf("ffffffffffffffffffffffffffffff 5555 000\n");
   dumper.dump();
}
