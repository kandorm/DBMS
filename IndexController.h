#ifndef INDEX_CONTROLLER
#define INDEX_CONTROLLER

#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include "IndexCommon.h"
#include <string.h>


class IndexController {
private:
	BufPageManager* bufPageManager;
	int fileID;
	int fileHeadIndex;
	IndexFileHead* indexFileHead;

	IndexType indexType;
	int indexSize;
	int indexPerPage;

	int pageNum;

	int allocIndexPage() {
		indexFileHead->firstFreePage++;
		indexFileHead->pageNum++;
		bufPageManager->markDirty(fhindex);
		return fep;
	}

	bool lessThanKey(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *(int*)key1;
			int b_int = *(int*)key2;
			return a_int < b_int;
			break;
		case CHAR:
			char* a_char = (char*)key1;
			char* b_char = (char*)key2;
			//std::cout << "less : a_char : " << a_char << "b_char : " << b_char << endl;
			return strcmp(a_char, b_char) < 0;
			break;
		default:
			break;
		}
		return false;
	}

	bool greaterThanKey(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *(int*)key1;
			int b_int = *(int*)key2;
			return a_int > b_int;
			break;
		case CHAR:
			char* a_char = (char*)key1;
			char* b_char = (char*)key2;
			//std::cout << "greater : a_char : " << a_char << "b_char : " << b_char << endl;
			return strcmp(a_char, b_char) > 0;
			break;
		default:
			break;
		}
		return false;
	}

	bool equal2Key(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *(int*)key1;
			int b_int = *(int*)key2;
			return a_int == b_int;
			break;
		case CHAR:
			char* a_char = (char*)key1;
			char* b_char = (char*)key2;
			//std::cout << "greater : a_char : " << a_char << "b_char : " << b_char << endl;
			return strcmp(a_char, b_char) == 0;
			break;
		default:
			break;
		}
		return false;
	}

	int getValue(char* b, int i) {
		return (int*)(b + sizeof(PageHead) + i*(sizeof(int) + indexSize))[0];
	}

	char* getKey(char* b, int i) {
		return (char*)(b + sizeof(PageHead) + i*(sizeof(int) + indexSize) + sizeof(int));
	}

public:
	IndexController(BufPageManager* bpm, int fID) {
		bufPageManager = bpm;
		fileID = fID;
		indexFileHead = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		bufPageManager->access(fileHeadIndex);

		indexType = indexFileHead->indexType;
		indexSize = indexFileHead->indexSize;
		indexPerPage = (PAGE_SIZE - sizeof(PageHead)-sizeof(int)) / (sizeof(int) + indexSize);

		pageNum = indexFileHead->pageNum;

	}

	int insertNode(void* data, int recordID) {
		if (indexFileHead->root == -1) {
			indexFileHead->root = allocIndexPage(fileID);
			int index;
			BufType b = bufPageManager->getPage(fileID, IndexFileHead->root, index);
			bufPageManager->markDirty(index);
			PageHead* pageHead = (pageHead*)b;

		}
	}

	bool deleteNode() {

	}

	bool searchNode(int pageID, void* targetKey, int& recordID) {
		
		if (pageID < 1 || pageID > pageNum - 1)return false;

		int index;
		char* b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;
		PageType pageType = pageHead->pageType;
		int keyNum = pageHead->n;
		switch (pageType)
		{
		case NODE:
			if (greaterThanKey(targetKey, getKey(b, keyNum - 1))) {
				int afterKeyPage = getValue(b, nodeNum);
				return searchNode(afterKeyPage, targetKey, recordID);
			}
			else {
				for (int i = 0; i < keyNum; i++) {
					char* tkey = getKey(b, i);
					if (lessThanKey(targetKey, tkey)) {
						int beforeKeyPage = getValue(b, i);
						return searchNode(beforeKeyPage, targetKey, recordID);
					}
				}
			}
			break;
		case LEAF:
			for (int i = 0; i < keyNum; i++) {
				char* tkey = getKey(b, i);
				if (equal2Key(targetKey, tkey)) {
					recordID = getValue(b, i);
					return true;
				}
			}
			break;
		default:
			break;
		}
		return false;
	}

};
#endif // !INDEX_CONTROLLER
