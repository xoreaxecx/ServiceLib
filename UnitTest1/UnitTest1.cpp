#include "pch.h"
#include "CppUnitTest.h"
#include"../CppLib/ServiceControl.h"
#include "../CppLib/dllmain.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			ItemListHandle* hItems = new ItemListHandle();
			ServiceEntry** itemsFound = new ServiceEntry*;
			int* itemCount = new int();

			int* testItemsCount = new int();


			bool result;
			result = GenerateItems(hItems, itemsFound, itemCount);

			//auto services = ServiceControl::EnumServices();
			//std::vector<ServiceEntry>* items = &services;

			//*hItems = reinterpret_cast<ItemListHandle>(items);
			//*itemsFound = items->data();
			//*itemCount = items->size();

			ServiceEntry se = ServiceEntry();
			auto temp = *itemsFound;
			auto temp3 = temp->Name;

			BSTR bs = BSTR();
			//std::wstring temp2(bs, SysStringLen(bs));
			//std::wstring temp3 = L"qwe";

			Assert::AreEqual(temp3, bs);
		}

		TEST_METHOD(TestMethod2)
		{
			ItemListHandle* hItems = new ItemListHandle();
			const char* name = "wuauserv";
			ServiceEntry* entry = new ServiceEntry();
			DWORD result;
			DWORD err = DWORD{};

			result = CallStartService(hItems, name, entry);

			Assert::AreEqual(err, result);
		}
	};
}
