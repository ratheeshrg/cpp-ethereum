/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file
 * Fixture class for boost output when running testeth
 */

#include <boost/test/unit_test.hpp>
#include <libethashseal/Ethash.h>
#include <libethcore/BasicAuthority.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/Options.h>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::eth;
using namespace boost;

void TestOutputHelper::initTest(size_t)
{
	Ethash::init();
	BasicAuthority::init();
	NoProof::init();
	if (!Options::get().createRandomTest)
		std::cout << "Test Case \"\": \n";
}

bool TestOutputHelper::checkTest(std::string const& _testName)
{
	if (test::Options::get().singleTest && test::Options::get().singleTestName != _testName)
		return false;

	cnote << _testName;
	return true;
}

void TestOutputHelper::showProgress()
{
	if (!test::Options::get().createRandomTest)
	{
		std::cout << "...";
		std::cout << "\n";
	}
}

void TestOutputHelper::finishTest()
{
}

void TestOutputHelper::printTestExecStats()
{
	if (Options::get().exectimelog)
	{
		std::cout << std::left;
	}
}
