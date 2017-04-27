#include "header/noob-constant.h"
#include "header/noob-value.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * Global Variables, store the results of the tests.
 */
std::string gCurrentTest = "";
int gResult = 0;
int gTestTotal = 0;
int gTestPass = 0;

/*
 * Test functions.
 */
#define TestEqualBase(equality, expect, actual)\
  do {\
    gTestTotal++;\
    if (equality)\
      gTestPass++;\
    else {\
      std::cout << __FILE__ << ":" << __LINE__ << " > Expect: " << expect << " Actual: " << actual << std::endl;\
      gResult++;\
    }\
  } while(0)


#define TestEqualInt(expect, actual)\
  do {\
    TestEqualBase((expect) == (actual), expect, actual);\
  } while(0)

#define TestEqualDouble(expect, actual)\
  do {\
    TestEqualBase((expect) == (actual), expect, actual);\
  } while(0)

#define TestEqualString(expect, actual, length) \
  do {\
    TestEqualBase(\
      sizeof(expect) - 1 == length && memcmp(expect, actual, length) == 0,\
      expect, actual);\
  } while(0)

#define TestNumber(expect, json)\
  do {\
    NoobValue v;\
    TestEqualInt(kNoobOk, v.NoobParse(json));\
    TestEqualInt(kNoobNumber, v.NoobGetType());\
    TestEqualDouble(expect, v.NoobGetNumber());\
  } while(0)

#define TestError(error, json)\
  do {\
    NoobValue v;\
    v.NoobSetType(kNoobFalse);\
    TestEqualInt(error, v.NoobParse(json));\
    TestEqualInt(kNoobNull, v.NoobGetType());\
  } while(0)

#define TestString(expect, json)\
  do {\
    NoobValue v;\
    TestEqualInt(kNoobOk, v.NoobParse(json));\
    TestEqualInt(kNoobString, v.NoobGetType());\
    TestEqualString(expect, v.NoobGetString().c_str(), v.NoobGetStringLength());\
  } while(0)

#define TestTrue(actual) TestEqualBase((actual) != 0, "true", "false")
#define TestFalse(actual) TestEqualBase((actual) == 0, "false", "true")

/*
 * Test cases.
 */

static void TestParseLiteral() {
  NoobValue v;
  TestEqualInt(kNoobOk, v.NoobParse("null"));
  TestEqualInt(kNoobNull, v.NoobGetType());
  TestEqualInt(kNoobOk, v.NoobParse("true"));
  TestEqualInt(kNoobTrue, v.NoobGetType());
  TestEqualInt(kNoobOk, v.NoobParse("false"));
  TestEqualInt(kNoobFalse, v.NoobGetType());
}

static void TestParseNumber() {
  TestNumber(0.0, "0");
  TestNumber(0.0, "-0");
  TestNumber(0.0, "-0.0");
  TestNumber(1.0, "1");
  TestNumber(-1.0, "-1");
  TestNumber(1.5, "1.5");
  TestNumber(-1.5, "-1.5");
  TestNumber(3.1416, "3.1416");
  TestNumber(1E10, "1E10");
  TestNumber(1e10, "1e10");
  TestNumber(1E+10, "1E+10");
  TestNumber(1E-10, "1E-10");
  TestNumber(-1E10, "-1E10");
  TestNumber(-1e10, "-1e10");
  TestNumber(-1E+10, "-1E+10");
  TestNumber(-1E-10, "-1E-10");
  TestNumber(1.234E+10, "1.234E+10");
  TestNumber(1.234E-10, "1.234E-10");
  TestNumber(0.0, "1e-10000"); /* must underflow */

  TestNumber(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
  TestNumber( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
  TestNumber(-4.9406564584124654e-324, "-4.9406564584124654e-324");
  TestNumber( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
  TestNumber(-2.2250738585072009e-308, "-2.2250738585072009e-308");
  TestNumber( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
  TestNumber(-2.2250738585072014e-308, "-2.2250738585072014e-308");
  TestNumber( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
  TestNumber(-1.7976931348623157e+308, "-1.7976931348623157e+308");
};

static void TestParseString() {
  TestString("", "\"\"");
  TestString("Hello", "\"Hello\"");
  TestString("Hello\nWorld", "\"Hello\\nWorld\"");
  TestString("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
  TestString("Hello\0World", "\"Hello\\u0000World\"");
  TestString("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
  TestString("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
  TestString("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
  TestString("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
  TestString("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void TestParseArray() {
  NoobValue v;
  TestEqualInt(kNoobOk, v.NoobParse("[ ]"));
  TestEqualInt(kNoobArray, v.NoobGetType());
  TestEqualInt(0, v.NoobGetArraySize());

  TestEqualInt(kNoobOk, v.NoobParse("[ null , false , true , 123 , \"abc\" ]"));
  TestEqualInt(kNoobArray, v.NoobGetType());
  TestEqualInt(5, v.NoobGetArraySize());
  TestEqualInt(kNoobNull,   v[0].NoobGetType());
  TestEqualInt(kNoobFalse,  v[1].NoobGetType());
  TestEqualInt(kNoobTrue,   v[2].NoobGetType());
  TestEqualInt(kNoobNumber, v[3].NoobGetType());
  TestEqualInt(kNoobString, v[4].NoobGetType());
  TestEqualDouble(123.0, v[3].NoobGetNumber());
  TestEqualString("abc",
    v[4].NoobGetString().c_str(),
    v[4].NoobGetStringLength());

  TestEqualInt(kNoobOk, v.NoobParse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
  TestEqualInt(kNoobArray, v.NoobGetType());
  TestEqualInt(4, v.NoobGetArraySize());
  for (int i = 0; i < 4; i++) {
    const NoobValue &a = v[i];
    TestEqualInt(kNoobArray, a.NoobGetType());
    TestEqualInt(i, a.NoobGetArraySize());
    for (int j = 0; j < i; j++) {
      const NoobValue &e = a[j];
      TestEqualInt(kNoobNumber, e.NoobGetType());
      TestEqualDouble((double)j, e.NoobGetNumber());
    }
  }
}

static void TestParseObject() {
  NoobValue v;

  TestEqualInt(kNoobOk, v.NoobParse(" { } "));
  TestEqualInt(kNoobObject, v.NoobGetType());
  TestEqualInt(0, v.NoobGetObjectSize());

  TestEqualInt(kNoobOk, v.NoobParse(
    " { "
      "\"n\" : null , "
      "\"f\" : false , "
      "\"t\" : true , "
      "\"i\" : 123 , "
      "\"s\" : \"abc\", "
      "\"a\" : [ 1, 2, 3 ],"
      "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
      " } "
  ));
  TestEqualInt(kNoobObject, v.NoobGetType());
  TestEqualInt(7, v.NoobGetObjectSize());
  TestTrue(v.NoobHasKey("n"));
  TestEqualInt(kNoobNull, v.NoobGetObjectValue("n").NoobGetType());
  TestTrue(v.NoobHasKey("f"));
  TestEqualInt(kNoobFalse, v.NoobGetObjectValue("f").NoobGetType());
  TestTrue(v.NoobHasKey("t"));
  TestEqualInt(kNoobTrue, v.NoobGetObjectValue("t").NoobGetType());
  TestTrue(v.NoobHasKey("i"));
  TestEqualInt(kNoobNumber, v.NoobGetObjectValue("i").NoobGetType());
  TestEqualDouble(123.0, v.NoobGetObjectValue("i").NoobGetNumber());
  TestTrue(v.NoobHasKey("s"));
  TestEqualInt(kNoobString, v.NoobGetObjectValue("s").NoobGetType());
  TestEqualString("abc",
    v.NoobGetObjectValue("s").NoobGetString().c_str(),
    v.NoobGetObjectValue("s").NoobGetString().length()
  );
  TestTrue(v.NoobHasKey("a"));
  TestEqualInt(kNoobArray, v.NoobGetObjectValue("a").NoobGetType());
  TestEqualInt(3, v.NoobGetObjectValue("a").NoobGetArraySize());
  for (size_t i = 0; i < 3; i++) {
    const NoobValue& e = v.NoobGetObjectValue("a").NoobGetArrayElement(i);
    TestEqualInt(kNoobNumber, e.NoobGetType());
    TestEqualDouble(i + 1.0, e.NoobGetNumber());
  }
  TestTrue(v.NoobHasKey("o"));
  {
    const NoobValue &o = v.NoobGetObjectValue("o");
    TestEqualInt(kNoobObject, o.NoobGetType());
    std::string key[] = {"1", "2", "3"};
    for (size_t i = 0; i < 3; i++) {
      const NoobValue &ov = o.NoobGetObjectValue(key[i]);
      TestEqualInt(kNoobNumber, ov.NoobGetType());
      TestEqualDouble(i + 1.0, ov.NoobGetNumber());
    }
  }
}

static void TestParseIllegalLiteral() {
  TestError(kNoobExpectValue, "");
  TestError(kNoobExpectValue, " ");

  TestError(kNoobInvalidValue, "nul");
  TestError(kNoobInvalidValue, "?");

  TestError(kNoobNotSigular, "null x");
}

static void TestParseIllegalNumber() {
  TestError(kNoobInvalidValue, "+0");
  TestError(kNoobInvalidValue, "+1");
  TestError(kNoobInvalidValue, ".123"); /* at least one digit before '.' */
  TestError(kNoobInvalidValue, "1.");   /* at least one digit after '.' */
  TestError(kNoobInvalidValue, "INF");
  TestError(kNoobInvalidValue, "inf");
  TestError(kNoobInvalidValue, "NAN");
  TestError(kNoobInvalidValue, "nan");

  TestError(kNoobNumberTooBig, "1e309");
  TestError(kNoobNumberTooBig, "-1e309");
  TestError(kNoobNotSigular, "0123"); /* after zero should be '.' or nothing */
  TestError(kNoobNotSigular, "0x0");
  TestError(kNoobNotSigular, "0x123");
}

static void TestParseIllegalString() {
  TestError(kNoobMissQuotationMark, "\"");
  TestError(kNoobMissQuotationMark, "\"abc");
  TestError(kNoobInvalidStringEscape, "\"\\v\"");
  TestError(kNoobInvalidStringEscape, "\"\\'\"");
  TestError(kNoobInvalidStringEscape, "\"\\0\"");
  TestError(kNoobInvalidStringEscape, "\"\\x12\"");
  TestError(kNoobInvalidStringChar, "\"\x01\"");
  TestError(kNoobInvalidStringChar, "\"\x1F\"");
}

static void TestParseIllegalUnicode() {
  TestError(kNoobInvalidUnicodeHex, "\"\\u\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u0\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u01\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u012\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u/000\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\uG000\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u0/00\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u0G00\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u0/00\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u00G0\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u000/\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u000G\"");
  TestError(kNoobInvalidUnicodeHex, "\"\\u 123\"");

  TestError(kNoobInvalidUnicodeSurrogate, "\"\\uD800\"");
  TestError(kNoobInvalidUnicodeSurrogate, "\"\\uDBFF\"");
  TestError(kNoobInvalidUnicodeSurrogate, "\"\\uD800\\\\\"");
  TestError(kNoobInvalidUnicodeSurrogate, "\"\\uD800\\uDBFF\"");
  TestError(kNoobInvalidUnicodeSurrogate, "\"\\uD800\\uE000\"");
}

static void TestParseIllegalArray() {
  TestError(kNoobMissCommaOrSquareBracket, "[1");
  TestError(kNoobMissCommaOrSquareBracket, "[1}");
  TestError(kNoobMissCommaOrSquareBracket, "[1 2");
  TestError(kNoobMissCommaOrSquareBracket, "[[]");
}

static void TestParseIllegalObject() {

  TestError(kNoobMissKey, "{:1,");
  TestError(kNoobMissKey, "{1:1,");
  TestError(kNoobMissKey, "{true:1,");
  TestError(kNoobMissKey, "{false:1,");
  TestError(kNoobMissKey, "{null:1,");
  TestError(kNoobMissKey, "{[]:1,");
  TestError(kNoobMissKey, "{{}:1,");
  TestError(kNoobMissKey, "{\"a\":1,");

  TestError(kNoobMissColon, "{\"a\"}");
  TestError(kNoobMissColon, "{\"a\",\"b\"}");

  TestError(kNoobMissCommaOrCurlyBracket, "{\"a\":1");
  TestError(kNoobMissCommaOrCurlyBracket, "{\"a\":1]");
  TestError(kNoobMissCommaOrCurlyBracket, "{\"a\":1 \"b\"");
  TestError(kNoobMissCommaOrCurlyBracket, "{\"a\":{}");
}

void CaseTest() {
  TestParseLiteral();
  TestParseNumber();
  TestParseString();
  TestParseArray();
  TestParseObject();
  TestParseIllegalLiteral();
  TestParseIllegalNumber();
  TestParseIllegalString();
  TestParseIllegalUnicode();
  TestParseIllegalArray();
  TestParseIllegalObject();
}

static double TestParseFile(const char *filename) {
  std::ifstream is(filename, std::ifstream::binary);

  is.seekg(0, is.end);
  long long length = is.tellg();
  is.seekg(0, is.beg);

  char *buffer = new char[length + 1];
  is.read(buffer, length);
  buffer[length] = '\0';

  NoobValue v;
  double time_used = 0.0;
  clock_t start, end;
  start = clock();
  NoobReturnValue result = v.NoobParse(buffer);
  end = clock();
  if(result == kNoobOk) {
    time_used = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("> Parsing %s successful!\n", filename);
    printf("> Time Used: %.4f ms.\n", time_used);
  } else {
    time_used = -1.0;
    printf("> Parsing %s failed!\n", filename);
  }

  delete[] buffer;
  is.close();
  return time_used;
}

void FileTest() {
  const char *files[] = {
    "test/twitter.json",
    "test/canada.json",
    "test/citm_catalog.json",
  };

  for(int j = 0; j < 3; ++j) {
    bool flag = true;
    double average_time = 0.0;
    const char *filename = files[j];
    for(int i = 0; i < 10; ++i) {
      double current_time = TestParseFile(filename);
      if(current_time == -1.0) {
        printf("> Parse %s failed at TEST: %d\n", filename, i + 1);
        flag = false;
        break;
      } else {
        average_time += current_time;
      }
    }
    if(flag) {
      printf("> Parse %s succeed! Average time: %.4f\n", filename, average_time / 10);
      printf(">>>>>>>>>>>>>>>>>>>>>>>>\n\n");
    }
  }
}

int main() {
  CaseTest();
  printf("%d/%d (%3.2f%%) Passed\n",
    gTestPass,
    gTestTotal,
    gTestPass * 100.0 / gTestTotal
  );
  FileTest();
  return gResult;
}
