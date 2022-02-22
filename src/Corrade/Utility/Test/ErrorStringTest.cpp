/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Corrade/TestSuite/Tester.h"
#include "Corrade/Utility/DebugStl.h"
#include "Corrade/Utility/Implementation/ErrorString.h"

#ifdef CORRADE_TARGET_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Corrade { namespace Utility { namespace Test { namespace {

struct ErrorStringTest: TestSuite::Tester {
    explicit ErrorStringTest();

    #ifdef CORRADE_TARGET_WINDOWS
    void windowsString();
    #endif
};

ErrorStringTest::ErrorStringTest() {
    #ifdef CORRADE_TARGET_WINDOWS
    addTests({&ErrorStringTest::windowsString});
    #endif
}

#ifdef CORRADE_TARGET_WINDOWS
void ErrorStringTest::windowsString() {
    std::string string = Implementation::windowsErrorString(ERROR_FILE_NOT_FOUND);
    CORRADE_INFO("ERROR_FILE_NOT_FOUND error string is:" << string);

    /* FFS DO YOU HAVE TO YELL AT ME??? */
    const LANGID usEnglish = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    if(GetUserDefaultLangID() != usEnglish)
        CORRADE_SKIP("User language is not US English, can't test");

    CORRADE_COMPARE(string, "The system cannot find the file specified.");
}
#endif

}}}}

CORRADE_TEST_MAIN(Corrade::Utility::Test::ErrorStringTest)
