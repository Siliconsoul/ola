/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * LlaServerServiceImplTest.cpp
 * Test fixture for the LlaServerServiceImpl class
 * Copyright (C) 2005-2009 Simon Newton
 *
 * How this file is organized:
 *  We test each rpc method in LlaServerServiceImpl, for each method, we have a
 *  series of Check objects which validate the rpc response.
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>

#include <lla/ExportMap.h>
#include <llad/Universe.h>
#include "Client.h"
#include "DeviceManager.h"
#include "LlaServerServiceImpl.h"
#include "PluginLoader.h"
#include "UniverseStore.h"
#include <lla/Logging.h>
#include "common/rpc/SimpleRpcController.h"

using namespace lla;
using google::protobuf::Closure;
using google::protobuf::NewCallback;
using lla::Universe;
using lla::rpc::SimpleRpcController;


class LlaServerServiceImplTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LlaServerServiceImplTest);
  CPPUNIT_TEST(testGetDmx);
  CPPUNIT_TEST(testRegisterForDmx);
  CPPUNIT_TEST(testUpdateDmxData);
  CPPUNIT_TEST(testSetUniverseName);
  CPPUNIT_TEST(testSetMergeMode);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testGetDmx();
    void testRegisterForDmx();
    void testUpdateDmxData();
    void testSetUniverseName();
    void testSetMergeMode();

  private:
    void CallGetDmx(LlaServerServiceImpl *impl,
                    int universe_id,
                    class GetDmxCheck &check);
    void CallRegisterForDmx(LlaServerServiceImpl *impl,
                            int universe_id,
                            lla::proto::RegisterAction action,
                            class RegisterForDmxCheck &check);
    void CallUpdateDmxData(LlaServerServiceImpl *impl,
                           int universe_id,
                           const string &data,
                           class UpdateDmxDataCheck &check);
    void CallSetUniverseName(LlaServerServiceImpl *impl,
                             int universe_id,
                             const string &name,
                             class SetUniverseNameCheck &check);
    void CallSetMergeMode(LlaServerServiceImpl *impl,
                       int universe_id,
                       lla::proto::MergeMode merge_mode,
                       class SetMergeModeCheck &check);
};

CPPUNIT_TEST_SUITE_REGISTRATION(LlaServerServiceImplTest);

static const uint8_t SAMPLE_DMX_DATA[] = {1, 2, 3, 4, 5};

/*
 * The GetDmx Checks
 */
class GetDmxCheck {
  public:
    virtual void Check(SimpleRpcController *controller,
                       lla::proto::DmxData *reply) = 0;
};


/*
 * Assert that the data is all 0
 */
class GetDmxZeroDataCheck: public GetDmxCheck {
  public:
    void Check(SimpleRpcController *controller,
               lla::proto::DmxData *reply) {
      uint8_t test_data[DMX_UNIVERSE_SIZE];
      bzero(test_data, DMX_UNIVERSE_SIZE);
      CPPUNIT_ASSERT(!controller->Failed());
      CPPUNIT_ASSERT_EQUAL((size_t) DMX_UNIVERSE_SIZE, reply->data().length());
      CPPUNIT_ASSERT(!memcmp(reply->data().data(),
                             test_data,
                             DMX_UNIVERSE_SIZE));
    }
};


/*
 * Assert that the data matches the test data.
 */
class GetDmxValidDataCheck: public GetDmxCheck {
  public:
    void Check(SimpleRpcController *controller,
               lla::proto::DmxData *reply) {

      CPPUNIT_ASSERT(!controller->Failed());
      CPPUNIT_ASSERT_EQUAL(sizeof(SAMPLE_DMX_DATA), reply->data().length());
      CPPUNIT_ASSERT(!memcmp(reply->data().data(),
                             SAMPLE_DMX_DATA,
                             sizeof(SAMPLE_DMX_DATA)));
    }
};


/*
 * RegisterForDmxChecks
 */
class RegisterForDmxCheck {
  public:
    virtual void Check(SimpleRpcController *controller,
                       lla::proto::Ack *reply) = 0;
};


/*
 * UpdateDmxDataCheck
 */
class UpdateDmxDataCheck {
  public:
    virtual void Check(SimpleRpcController *controller,
                       lla::proto::Ack *reply) = 0;
};


/*
 * SetUniverseNameCheck
 */
class SetUniverseNameCheck {
  public:
    virtual void Check(SimpleRpcController *controller,
                       lla::proto::Ack *reply) = 0;
};


/*
 * SetMergeModeCheck
 */
class SetMergeModeCheck {
  public:
    virtual void Check(SimpleRpcController *controller,
                       lla::proto::Ack *reply) = 0;
};


/*
 * Assert that we got a missing universe error
 */
template<typename parent, typename reply>
class GenericMissingUniverseCheck: public parent {
  public:
    void Check(SimpleRpcController *controller,
               reply *r) {
      CPPUNIT_ASSERT(controller->Failed());
      CPPUNIT_ASSERT_EQUAL(string("Universe doesn't exist"),
                           controller->ErrorText());
    }
};


/*
 * Assert that we got an ack
 */
template<typename parent>
class GenericAckCheck: public parent {
  public:
    void Check(SimpleRpcController *controller,
               lla::proto::Ack *r) {
      CPPUNIT_ASSERT(!controller->Failed());
    }
};


/*
 * Check that the GetDmx method works
 */
void LlaServerServiceImplTest::testGetDmx() {
  UniverseStore store(NULL, NULL);
  LlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

  GenericMissingUniverseCheck<GetDmxCheck, lla::proto::DmxData>
    missing_universe_check;
  GetDmxZeroDataCheck zero_data_check;
  GetDmxValidDataCheck valid_data_check;

  // test a universe that doesn't exist
  unsigned int universe_id = 0;
  CallGetDmx(&impl, universe_id, missing_universe_check);

  // test a new universe
  Universe *universe = store.GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);
  CallGetDmx(&impl, universe_id, zero_data_check);

  // Set the universe data
  universe->SetDMX(SAMPLE_DMX_DATA, sizeof(SAMPLE_DMX_DATA));
  CallGetDmx(&impl, universe_id, valid_data_check);

  // remove the universe and try again
  store.AddUniverseGarbageCollection(universe);
  store.GarbageCollectUniverses();
  CallGetDmx(&impl, universe_id, missing_universe_check);
}


/*
 * Call the GetDmx method
 * @param impl the LlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param check the GetDmxCheck class to use for the callback check
 */
void LlaServerServiceImplTest::CallGetDmx(LlaServerServiceImpl *impl,
                                          int universe_id,
                                          GetDmxCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DmxReadRequest *request = new lla::proto::DmxReadRequest();
  lla::proto::DmxData *response = new lla::proto::DmxData();
  Closure *closure = NewCallback(
      &check,
      &GetDmxCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  impl->GetDmx(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the RegisterForDmx method works
 */
void LlaServerServiceImplTest::testRegisterForDmx() {
  UniverseStore store(NULL, NULL);
  LlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

  // Register for a universe that doesn't exist
  unsigned int universe_id = 0;
  unsigned int second_universe_id = 99;
  GenericAckCheck<RegisterForDmxCheck> ack_check;
  CallRegisterForDmx(&impl, universe_id, lla::proto::REGISTER, ack_check);

  // The universe should exist now and the client should be bound
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(universe);
  CPPUNIT_ASSERT(universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->ClientCount());

  // Try to register again
  CallRegisterForDmx(&impl, universe_id, lla::proto::REGISTER, ack_check);
  CPPUNIT_ASSERT(universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, universe->ClientCount());

  // Register a second universe
  CallRegisterForDmx(&impl, second_universe_id, lla::proto::REGISTER,
                     ack_check);
  Universe *second_universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(second_universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, second_universe->ClientCount());

  // Unregister the first universe
  CallRegisterForDmx(&impl, universe_id, lla::proto::UNREGISTER, ack_check);
  CPPUNIT_ASSERT(!universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->ClientCount());

  // Unregister the second universe
  CallRegisterForDmx(&impl, second_universe_id, lla::proto::UNREGISTER,
                     ack_check);
  CPPUNIT_ASSERT(!second_universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, second_universe->ClientCount());

  // Unregister again
  CallRegisterForDmx(&impl, universe_id, lla::proto::UNREGISTER, ack_check);
  CPPUNIT_ASSERT(!universe->ContainsClient(NULL));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, universe->ClientCount());

  store.DeleteAll();
}


/*
 * Call the RegisterForDmx method
 * @param impl the LlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param action the action to use REGISTER or UNREGISTER
 * @param check the RegisterForDmxCheck to use for the callback check
 */
void LlaServerServiceImplTest::CallRegisterForDmx(
    LlaServerServiceImpl *impl,
    int universe_id,
    lla::proto::RegisterAction action,
    RegisterForDmxCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::RegisterDmxRequest *request = new lla::proto::RegisterDmxRequest();
  lla::proto::Ack *response = new lla::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &RegisterForDmxCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_action(action);
  impl->RegisterForDmx(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the UpdateDmxData method works
 */
void LlaServerServiceImplTest::testUpdateDmxData() {
  UniverseStore store(NULL, NULL);
  LlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

  GenericMissingUniverseCheck<UpdateDmxDataCheck, lla::proto::Ack>
    missing_universe_check;
  GenericAckCheck<UpdateDmxDataCheck> ack_check;
  unsigned int universe_id = 0;
  string dmx_data = "this is a test";

  // Update a universe that doesn't exist
  CallUpdateDmxData(&impl, universe_id, dmx_data, missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Update a universe that exists
  universe = store.GetUniverseOrCreate(universe_id);
  CallUpdateDmxData(&impl, universe_id, dmx_data, ack_check);
  unsigned int returned_length;
  const uint8_t *universe_data = universe->GetDMX(returned_length);
  CPPUNIT_ASSERT_EQUAL(dmx_data.length(), (size_t) returned_length);
  CPPUNIT_ASSERT(!memcmp(dmx_data.data(), universe_data, returned_length));

  // Send a 0 sized update
  dmx_data = "";
  CallUpdateDmxData(&impl, universe_id, dmx_data, ack_check);
  universe_data = universe->GetDMX(returned_length);
  CPPUNIT_ASSERT_EQUAL(dmx_data.length(), (size_t) returned_length);
  CPPUNIT_ASSERT(!memcmp(dmx_data.data(), universe_data, returned_length));

  store.DeleteAll();
}


/*
 * Call the UpdateDmxDataCheck method
 * @param impl the LlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param data the data to use
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void LlaServerServiceImplTest::CallUpdateDmxData(
    LlaServerServiceImpl *impl,
    int universe_id,
    const string &data,
    UpdateDmxDataCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::DmxData *request = new
    lla::proto::DmxData();
  lla::proto::Ack *response = new lla::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &UpdateDmxDataCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_data(data);
  impl->UpdateDmxData(controller, request, response, closure);
  delete controller;
  delete request;
  delete response;
}


/*
 * Check the SetUniverseName method works
 */
void LlaServerServiceImplTest::testSetUniverseName() {
  UniverseStore store(NULL, NULL);
  LlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

  unsigned int universe_id = 0;
  string universe_name = "test 1";
  string universe_name2 = "test 1-2";

  GenericAckCheck<SetUniverseNameCheck> ack_check;
  GenericMissingUniverseCheck<SetUniverseNameCheck, lla::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetUniverseName(&impl, universe_id, universe_name, missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Check SetUniverseName works on an existing univserse
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetUniverseName(&impl, universe_id, universe_name, ack_check);
  CPPUNIT_ASSERT_EQUAL(universe_name, universe->Name());

  // Run it again with a new name
  CallSetUniverseName(&impl, universe_id, universe_name2, ack_check);
  CPPUNIT_ASSERT_EQUAL(universe_name2, universe->Name());
  store.DeleteAll();
}


/*
 * Call the SetUniverseName method
 * @param impl the LlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param name the name to use
 * @param check the SetUniverseNameCheck to use for the callback check
 */
void LlaServerServiceImplTest::CallSetUniverseName(
    LlaServerServiceImpl *impl,
    int universe_id,
    const string &name,
    SetUniverseNameCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::UniverseNameRequest *request = new
    lla::proto::UniverseNameRequest();
  lla::proto::Ack *response = new lla::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &SetUniverseNameCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_name(name);
  impl->SetUniverseName(controller, request, response, closure);

  delete controller;
  delete request;
  delete response;
}


/*
 * Check the SetMergeMode method works
 */
void LlaServerServiceImplTest::testSetMergeMode() {
  UniverseStore store(NULL, NULL);
  LlaServerServiceImpl impl(&store,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

  unsigned int universe_id = 0;

  GenericAckCheck<SetMergeModeCheck> ack_check;
  GenericMissingUniverseCheck<SetMergeModeCheck, lla::proto::Ack>
    missing_universe_check;

  // Check we get an error for a missing universe
  CallSetMergeMode(&impl, universe_id, lla::proto::HTP, missing_universe_check);
  Universe *universe = store.GetUniverse(universe_id);
  CPPUNIT_ASSERT(!universe);

  // Check SetUniverseName works
  universe = store.GetUniverseOrCreate(universe_id);
  CallSetMergeMode(&impl, universe_id, lla::proto::HTP, ack_check);
  CPPUNIT_ASSERT(Universe::MERGE_HTP == universe->MergeMode());

  // Run it again
  CallSetMergeMode(&impl, universe_id, lla::proto::LTP, ack_check);
  CPPUNIT_ASSERT(Universe::MERGE_LTP == universe->MergeMode());
  store.DeleteAll();
}


/*
 * Call the SetMergeMode method
 * @param impl the LlaServerServiceImpl to use
 * @param universe_id the universe_id in the request
 * @param mode the merge_mode to use
 * @param check the SetMergeModeCheck to use for the callback check
 */
void LlaServerServiceImplTest::CallSetMergeMode(
    LlaServerServiceImpl *impl,
    int universe_id,
    lla::proto::MergeMode merge_mode,
    SetMergeModeCheck &check) {
  SimpleRpcController *controller = new SimpleRpcController();
  lla::proto::MergeModeRequest *request = new
    lla::proto::MergeModeRequest();
  lla::proto::Ack *response = new lla::proto::Ack();
  Closure *closure = NewCallback(
      &check,
      &SetMergeModeCheck::Check,
      controller,
      response);

  request->set_universe(universe_id);
  request->set_merge_mode(merge_mode);
  impl->SetMergeMode(controller, request, response, closure);

  delete controller;
  delete request;
  delete response;
}