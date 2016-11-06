#ifndef INDEX_CONTROLLER
#define INDEX_CONTROLLER

#include "filesystem\bufmanager\BufPageManager.h"
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

	int root;
	int pageNum;

	int allocIndexPage() {
		int fep = indexFileHead->firstFreePage;
		indexFileHead->firstFreePage++;
		indexFileHead->pageNum++;
		bufPageManager->markDirty(fileHeadIndex);
		return fep;
	}

	int fillData(BufType b,char* key,int recordID,int i) {
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int)),&recordID,sizeof(int));
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int))+sizeof(int),key,indexSize);
	}
	
	char* getMaxK(int pageID) {
		int index;
		BufType b = bufPageManager->getPage(fileID,pageID,index);
		//bufPageManager->markDirty(index);
		(PageHead*) pageHead = (PageHead*)b;
		return getKey(b,pageHead->n-1);
	}


	bool lessThanKey(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *((int*)key1);
			int b_int = *((int*)key2);
			return a_int < b_int;
			break;
		case CHAR:
			char* a_buf = new char[indexSize + 5];
			memcpy(a_buf, key1, indexSize);
			a_buf[indexSize] = 0;

			char* b_buf = new char[indexSize + 5];
			memcpy(b_buf, key2, indexSize);
			b_buf[indexSize] = 0;

			//std::cout << "less : a_buf : " << a_buf << "b_buf : " << b_buf << endl;
			return strcmp(a_buf, b_buf) < 0;
			break;
		default:
			break;
		}
		return false;
	}

	bool greaterThanKey(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *((int*)key1);
			int b_int = *((int*)key2);
			return a_int > b_int;
			break;
		case CHAR:
			char* a_buf = new char[indexSize + 5];
			memcpy(a_buf, key1, indexSize);
			a_buf[indexSize] = 0;

			char* b_buf = new char[indexSize + 5];
			memcpy(b_buf, key2, indexSize);
			b_buf[indexSize] = 0;

			//std::cout << "greater : a_buf : " << a_buf << "b_buf : " << b_buf << endl;
			return strcmp(a_buf, b_buf) > 0;
			break;
		default:
			break;
		}
		return false;
	}

	bool equal2Key(void* key1, void* key2) {
		switch (indexType) {
		case INT:
			int a_int = *((int*)key1);
			int b_int = *((int*)key2);
			return a_int == b_int;
			break;
		case CHAR:
			char* a_buf = new char[indexSize + 5];
			memcpy(a_buf, key1, indexSize);
			a_buf[indexSize] = 0;

			char* b_buf = new char[indexSize + 5];
			memcpy(b_buf, key2, indexSize);
			b_buf[indexSize] = 0;

			//std::cout << "equal : a_buf : " << a_buf << "b_buf : " << b_buf << endl;
			return strcmp(a_buf, b_buf) == 0;
			break;
		default:
			break;
		}
		return false;
	}


	BufType getValue(BufType b, int i) {
		return b + sizeof(PageHead) + i*(sizeof(int) + indexSize);
	}

	char* getKey(char* b, int i) {
		return (char*)(b + sizeof(PageHead) + i*(sizeof(int) + indexSize) + sizeof(int));
	}

	int _searchNode(int pageID, void* targetKey, int& subIdx) {

		if (pageID < 1 || pageID > pageNum - 1)return -1;

		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;
		PageType pageType = pageHead->pageType;
		int keyNum = pageHead->n;
		switch (pageType)
		{
		case NODE:
			if (!lessThanKey(targetKey, getKey(b, keyNum - 1))) {
				int afterKeyPage = getValue(b, keyNum)[0];
				subIdx = keyNum;
				return _searchNode(afterKeyPage, targetKey, subIdx);
			}
			else {
				for (int i = 0; i < keyNum; i++) {
					char* tkey = getKey(b, i);
					if (lessThanKey(targetKey, tkey)) {
						int beforeKeyPage = getValue(b, i)[0];
						subIdx = i;
						return _searchNode(beforeKeyPage, targetKey, subIdx);
					}
				}
			}
			break;
		case LEAF:
			return pageID;
			break;
		default:
			break;
		}
		return -1;
	}

public:
	IndexController(BufPageManager* bpm, int fID) {
		bufPageManager = bpm;
		fileID = fID;
		indexFileHead = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		bufPageManager->access(fileHeadIndex);
		indexType = indexFileHead->indexType;
		indexSize = indexFileHead->indexSize;
		indexPerPage = (PAGE_SIZE - sizeof(PageHead) - sizeof(int)) / (sizeof(int) + indexSize);

		root = indexFileHead->root;
		pageNum = indexFileHead->pageNum;
	}
	
	int insertNode(int fileID,char* key,int recordID) {
		//markDirty
		if(IndexFileHead->root == -1) {
			IndexFileHead->root = allocIndexPage();
			int index;
			BufType b = bufPageManager->getPage(fileID,IndexFileHead->root,index);
			bufPageManager->markDirty(index);
			PageHead* pageHead = (PageHead*) b;
			pageHead->n = 1;
			pageHead->pageType = LEAF;
			fillData(b,key,recordID,0);
		}
		else {
			if(insertBPlus(fileID,IndexFileHead->root,key,recordID) == SPLIT) {
				int newPage = allocIndexPage();
				splitPage(IndexFileHead->root,newPage);
				int newRoot = allocIndexPage();
				int index;
				BufType b = bufPageManager->getPage(fileID,newRoot,index);
				bufPageManager->markDirty(index);
				(PageHead*) pageHead = (PageHead*)b;
				pageHead->n = 1;
				pageHead->pageType = NODE;
				BufType p = getValue(b,1);
				(*p) = newPage;
				fillData(b,getMaxK(fileID,indexFileHead->root),indexFileHead->root,0);
				indexFileHead->root = newRoot;
			}
		}
	}
	
	int insertBPlus(int pageID,char* key,int recordID) {
		int index;
		BufType b = bufPageManager->getPage(fileID,pageID,index);
		PageHead* pageHead = (PageHead*)b;
		if(pageHead->pageType == LEAF) {
			int pos = pageHead->n;
			for(int i = 0;i < pageHead->n;i++) {
				char* k = getKey(b,i);
				if(lessThanKey()){}
			}
		}
	}

	bool adjustKey(int pageID, char* sm_key, int subIdx)
	{
		if (subIdx < 0 || subIdx > indexPerPage)return false;
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;

		if (subIdx == 0) {
		}
		else if (subIdx == 1)
		{
			char* tkey = getKey(b, subIdx - 1);
			memcpy(tkey, sm_key, indexSize);
			int fatherPageID = pageHead->fatherPage;
			if (fatherPageID > 0) {
				int fatherPageIndex;
				BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
				PageHead* pageHead = (PageHead*)fatherPageBuf;
				int keyNum = pageHead->n;
				int i = 0;
				for (; i <= keyNum; i++) {
					if (getValue(fatherPageBuf, i)[0] == pageID)
						break;
				}
				adjustKey(fatherPageID, sm_key, i);
			}
		}
		else {
			char* tkey = getKey(b, subIdx - 1);
			memcpy(tkey, sm_key, indexSize);
		}
	}
	bool deleteNode(void* targetKey, int& recordID) {
		
		bool deleteSuccessFlag = false;

		int subIndex = -1;
		int targetPageID = _searchNode(root, targetKey, subIndex);
		int index;
		BufType b = bufPageManager->getPage(fileID, targetPageID, index);
		PageHead* pageHead = (PageHead*)b;
		int keyNum = pageHead->n;
		int fatherPageID = pageHead->fatherPage;
		if (keyNum > indexPerPage /2) {
			char* oldSmKey = getKey(b, 0);
			for (int i = 0; i < keyNum; i++) {
				char* tkey = getKey(b, i);
				if (equal2Key(targetKey, tkey)) {
					BufType targetValueAddr = getValue(b, i);
					recordID = targetValueAddr[0];
					memcpy((char*)targetValueAddr, (char*)getValue(b, i + 1), (keyNum - 1 - i) * (sizeof(int) + indexSize));
					pageHead->n--;
					bufPageManager->markDirty(index);
					deleteSuccessFlag = true;
				}
			}
			if (!equal2Key(oldSmKey, getKey(b, 0))) {
				adjustKey(fatherPageID, getKey(b, 0), subIndex);
			}
		}
		else {

		}
		return deleteSuccessFlag;
	}

	bool searchNode(void* targetKey, int& recordID) {
		int subIndex = -1;
		int targetPageID = _searchNode(root, targetKey, subIndex);
		if (targetPageID < 0)return false;
		int index;
		BufType b = bufPageManager->getPage(fileID, targetPageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;
		int keyNum = pageHead->n;
		for (int i = 0; i < keyNum; i++) {
			char* tkey = getKey(b, i);
			if (equal2Key(targetKey, tkey)) {
				recordID = getValue(b, i);
				return true;
			}
		}
		
		return false;
	}
	
	

};
#endif // !INDEX_CONTROLLER
