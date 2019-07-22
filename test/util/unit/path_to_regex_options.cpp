
#include <common.cxx>
#include <util/path_to_regex.hpp>

using namespace std;
using namespace path2regex;

// ------------ TESTING PATH_TO_REGEX WITH OPTIONS --------------

  CASE("Calling path_to_regex with options")
  {
    // Create with option strict

    GIVEN("An empty vector of Tokens (keys) and option strict set to true")
    {
      Tokens keys;
      Options options{ {"strict", true} };

      WHEN("Calling path_to_regex with path '/:test'")
      {
        std::regex r = path_to_regex("/:test", keys, options);

        THEN("")
        {

        }
      }
    }/*,

    GIVEN("An empty vector of Tokens (keys) and option strict set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with option sensitive

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with option end

    GIVEN("An empty vector of Tokens (keys) and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with options strict and sensitive

    GIVEN("An empty vector of Tokens (keys) and options strict and sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict and sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true and option sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false and option sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with options strict and end

    GIVEN("An empty vector of Tokens (keys) and options strict and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with options sensitive and end

    GIVEN("An empty vector of Tokens (keys) and options sensitive and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options sensitive and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to true and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to false and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    // Create with options strict, sensitive and end

    GIVEN("An empty vector of Tokens (keys) and options strict, sensitive and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict, sensitive and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to true and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to false and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to false and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to false and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to true and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true}, {"end", true} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to true and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true}, {"end", false} };

      WHEN("Calling path_to_regex with path ''")
      {
        std::regex r = PathToRegex::path_to_regex("", keys, options);

      }
    }*/
  }; // < SCENARIO (with options)

