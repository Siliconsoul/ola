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
 * DmxBufferTest.cpp
 * Unittest for the DmxBuffer
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>
#include <lla/DmxBuffer.h>

using namespace lla;
using namespace std;

class DmxBufferTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxBufferTest);
  CPPUNIT_TEST(testGetSet);
  CPPUNIT_TEST(testStringGetSet);
  CPPUNIT_TEST(testAssign);
  CPPUNIT_TEST(testCopy);
  CPPUNIT_TEST(testMerge);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testGetSet();
    void testAssign();
    void testStringGetSet();
    void testCopy();
    void testMerge();
  private:
    static const uint8_t TEST_DATA[];
    static const uint8_t TEST_DATA2[];
    static const uint8_t TEST_DATA3[];
    static const uint8_t MERGE_RESULT[];
};

const uint8_t DmxBufferTest::TEST_DATA[] = {1, 2, 3, 4, 5};
const uint8_t DmxBufferTest::TEST_DATA2[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
const uint8_t DmxBufferTest::TEST_DATA3[] = {10, 11, 12};
const uint8_t DmxBufferTest::MERGE_RESULT[] = {10, 11, 12, 4, 5};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxBufferTest);

/*
 * Check that Get/Set works correctly
 */
void DmxBufferTest::testGetSet() {
  unsigned int fudge_factor = 10;
  unsigned int result_length = sizeof(TEST_DATA2) + fudge_factor;
  uint8_t *result = new uint8_t[result_length];
  unsigned int size = result_length;
  DmxBuffer buffer;
  string str_result;

  CPPUNIT_ASSERT(buffer.Set(TEST_DATA, sizeof(TEST_DATA)));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA), buffer.Size());
  buffer.Get(result, size);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA), size);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA, result, size));
  str_result = buffer.Get();
  CPPUNIT_ASSERT_EQUAL((size_t) sizeof(TEST_DATA), str_result.length());
  CPPUNIT_ASSERT(!memcmp(TEST_DATA, str_result.data(), str_result.length()));

  size = result_length;
  CPPUNIT_ASSERT(buffer.Set(TEST_DATA2, sizeof(TEST_DATA2)));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA2), buffer.Size());
  buffer.Get(result, size);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA2), size);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA2, result, size));
  str_result = buffer.Get();
  CPPUNIT_ASSERT_EQUAL((size_t) sizeof(TEST_DATA2), str_result.length());
  CPPUNIT_ASSERT(!memcmp(TEST_DATA2, str_result.data(), str_result.length()));
  delete[] result;
}


/*
 * Check that the string set/get methods work
 */
void DmxBufferTest::testStringGetSet() {
  const string data = "abcdefg";
  DmxBuffer buffer;
  uint8_t *result = new uint8_t[data.length()];
  unsigned int size = data.length();

  // Check that setting works
  CPPUNIT_ASSERT(buffer.Set(data));
  CPPUNIT_ASSERT_EQUAL(data.length(), (size_t) buffer.Size());
  CPPUNIT_ASSERT_EQUAL(data, buffer.Get());
  buffer.Get(result, size);
  CPPUNIT_ASSERT_EQUAL(data.length(), (size_t) size);
  CPPUNIT_ASSERT(!memcmp(data.data(), result, size));

  // Set with an empty string
  string data2;
  size = data.length();
  CPPUNIT_ASSERT(buffer.Set(data2));
  CPPUNIT_ASSERT_EQUAL(data2.length(), (size_t) buffer.Size());
  CPPUNIT_ASSERT_EQUAL(data2, buffer.Get());
  buffer.Get(result, size);
  CPPUNIT_ASSERT_EQUAL(data2.length(), (size_t) size);
  CPPUNIT_ASSERT(!memcmp(data2.data(), result, size));
}


/*
 * Check the copy and assignment operators work
 */
void DmxBufferTest::testAssign() {
  unsigned int fudge_factor = 10;
  unsigned int result_length = sizeof(TEST_DATA) + fudge_factor;
  uint8_t *result = new uint8_t[result_length];
  DmxBuffer buffer;
  DmxBuffer assignment_buffer, assignment_buffer2;

  CPPUNIT_ASSERT(buffer.Set(TEST_DATA, sizeof(TEST_DATA)));
  CPPUNIT_ASSERT(assignment_buffer.Set(TEST_DATA3, sizeof(TEST_DATA3)));

  // assigning to ourself does nothing
  buffer = buffer;

  // assinging to a previously init'ed buffer
  unsigned int size = result_length;
  assignment_buffer = buffer;
  assignment_buffer.Get(result, size);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA),
                       assignment_buffer.Size());
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA), size);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA, result, size));
  CPPUNIT_ASSERT(assignment_buffer == buffer);

  // assigning to a non-init'ed buffer
  assignment_buffer2 = buffer;
  size = result_length;
  assignment_buffer2.Get(result, result_length);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA),
                       assignment_buffer2.Size());
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA), result_length);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA, result, result_length));
  CPPUNIT_ASSERT(assignment_buffer2 == buffer);

  // now try assigning an unitialized buffer
  DmxBuffer uninitialized_buffer;
  DmxBuffer assignment_buffer3;

  assignment_buffer3 = uninitialized_buffer;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, assignment_buffer3.Size());
  size = result_length;
  assignment_buffer3.Get(result, result_length);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, result_length);
  CPPUNIT_ASSERT(assignment_buffer3 == uninitialized_buffer);
  delete[] result;
}


/*
 * Check that the copy constructor works
 */
void DmxBufferTest::testCopy() {
  DmxBuffer buffer;
  CPPUNIT_ASSERT(buffer.Set(TEST_DATA2, sizeof(TEST_DATA2)));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA2), buffer.Size());

  DmxBuffer copy_buffer(buffer);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA2), copy_buffer.Size());

  unsigned int result_length = sizeof(TEST_DATA2);
  uint8_t *result = new uint8_t[result_length];
  copy_buffer.Get(result, result_length);
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA2), result_length);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA2, result, result_length));
  CPPUNIT_ASSERT(copy_buffer == buffer);
  delete[] result;
}


/*
 * Check that HTP Merging works
 */
void DmxBufferTest::testMerge() {
  DmxBuffer buffer1, buffer2, merge_result;
  DmxBuffer uninitialized_buffer, uninitialized_buffer2;
  CPPUNIT_ASSERT(buffer1.Set(TEST_DATA, sizeof(TEST_DATA)));
  CPPUNIT_ASSERT(buffer2.Set(TEST_DATA3, sizeof(TEST_DATA3)));
  CPPUNIT_ASSERT(merge_result.Set(MERGE_RESULT, sizeof(MERGE_RESULT)));
  const DmxBuffer test_buffer(buffer1);
  const DmxBuffer test_buffer2(buffer2);

  // merge into an empty buffer
  CPPUNIT_ASSERT(uninitialized_buffer.HTPMerge(buffer2));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(TEST_DATA3), buffer2.Size());
  CPPUNIT_ASSERT(test_buffer2 == uninitialized_buffer);

  // merge from an empty buffer
  CPPUNIT_ASSERT(buffer2.HTPMerge(uninitialized_buffer2));
  CPPUNIT_ASSERT(buffer2 == test_buffer2);

  // merge two buffers (longer into shorter)
  buffer2 = test_buffer2;
  CPPUNIT_ASSERT(buffer2.HTPMerge(buffer1));
  CPPUNIT_ASSERT(buffer2 == merge_result);

  // merge shorter into longer
  buffer2 = test_buffer2;
  CPPUNIT_ASSERT(buffer1.HTPMerge(buffer2));
  CPPUNIT_ASSERT(buffer1 == merge_result);
}