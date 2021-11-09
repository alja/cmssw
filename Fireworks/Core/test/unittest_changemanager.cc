// -*- C++ -*-
//
// Package:     Core
// Class  :     unittest_changemanager
//
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Fri Jan 18 10:19:07 EST 2008
//

// system include files
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include "sigc++/signal.h"


BOOST_AUTO_TEST_CASE(changemanager) {

/*
FWModelChangeSignal a;
FWModelChangeSignal b;
a.connect(b);
*/

/*
sigc::signal<void, const std::set<int> &> a;
sigc::signal<void, const std::set<int> &> b;
a.connect(b);
*/

sigc::signal<void, int> a;
sigc::signal<void, int> b;
a.connect(b);

BOOST_CHECK(true);
}
