// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <lest.hpp>
#include <string>
#include "../bucket.hpp"


struct TestModel {
  size_t key;
  std::string str;

  TestModel(std::string&& str)
    : str{str}
  {}
}

using namespace bucket;

using TestBucket = Bucket<TestModel>;

const lest::test test_bucket[] =
{
  SCENARIO("Capture/spawning, pick up and abandoning")
  {
    GIVEN("An empty Bucket<TestModel>")
    {
      TestBucket bucket{10};



      WHEN("One TestModel is captured")
      {
        TestModel model{"test"};
        size_t index = 0;
        EXPECT_NO_THROWS( index = bucket.capture(model) );

        THEN("Model now has key updated and is stored")
        {
          EXPECT( model.key == 1 );
          EXPECT_NO_THROWS( bucket.pick_up(model.key) );
        }
      }
      WHEN("One TestModel is spawned")
      {
        TestModel model{"test"};
        size_t index = 0;

        THEN("Model now has key updated and is stored")
        {
          EXPECT( model.key == 1 );
          EXPECT_NO_THROWS( bucket.pick_up(model.key) );
        }
      }
    }
  },

  SCENARIO("Operating with limit and duplicates")
  {
    GIVEN("An empty Bucket<TestModel> with a limit of 10")
    {

    }
  },

  SCENARIO("Constraints, indexing and look up")
  {
    GIVEN("An Bucket<TestModel> with UNIQUE constraint on str and one model")
    {
      Bucket<TestModel> bucket{10};

      bucket.add_index<std::string>("str",
      [](const TestModel& t)->const auto&
      {
        return t.str;
      }, TestBucket::UNIQUE);

      TestModel tm;
      tm.str = "4Head";

      WHEN("One TestModels with the same name is captured")
      {
        THEN("Exception is thrown")
        {
          EXPECTS_THROW_AS(bucket.capture(tm), bucket::ConstraintUnique);
        }
      }

      WHEN("Looking for str")
      {
        auto& result = bucket.look_for("str", "4Head"s);
        EXPECT( result.key == tm.key );
      }
    }
  }
}

int main(int argc, char * argv[])
{
  printf("Running tests of Bucket...\n");

  int res = lest::run(test_bucket, argc, argv);

  printf("Tests completed.\n");

  return res;
}
