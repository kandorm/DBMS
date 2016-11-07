#ifndef INDEX_CONTROLLER
#define INDEX_CONTROLLER

#include "filesystem\bufmanager\BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include "IndexCommon.h"
#include <string.h>

#define VALUE_SIZE 4
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
				if(!lessThanKey(key,k)) {
					pos = i;
					break;
				}
			}
			insertIntoPage(b,pos,key,recordID);
			bufPageManager->markDirty(index);
			if(pageHead->n > indexPerPage)
				return SPLIT;
			return NOTHING;
		}
		else {
			int pos = pageHead->n;
			for(int i = 0;i < pageHead->n;i++) {
				char* k = getKey(b,i);
				if(!lessThanKey(key,k)) {
					pos = i;
					break;
				}
			}
			int nextPage = getValue(b,pos);
			int status = insertBPlus(nextPage,key,recordID);
			if(status == SPLIT) {
				int newPage = allocIndexPage();
				splitPage(nextPage,newPage);
				insertIntoPage(b,pos,getMaxK(nextPage),recordID);
				*getValue(b,pos+1) = newPage;
				bufPageManager->markDirty(index);
				if(pageHead->n > indexPerPage)
					return SPLIT;
			}
			return NOTHING;
		}
		return NOTHING;
	}
	
	bool insertIntoPage(BufType b,int pos,char* key,int recordID) {
		PageHead* pageHead = (PageHead*)b;
		*getValue(b,pageHead->n+1) = *getValue(b,pageHead->n);//n or n+1?
		for(int i = (pageHead->n)-1;i >= pos;i--) {
			fillData(b,getKey(b,i-1),*getValue(b,i-1),i);//????!!!!
		}
		pageHead->n++;
		fillData(b,key,recordID,pos);
		return true;
	}
	
	bool splitPage(int pageID1,int pageID2) {
		int index;
		BufType b1 = bufPageManager->getPage(fileID,pageID1,index);
		bufPageManager->markDirty(index);
		BufType b2 = bufPageManager->getPage(fileID,pageID2,index);
		bufPageManager->markDirty(index);
		PageHead* pageHead1 = (PageHead*)b1;
		PageHead* pageHead2 = (PageHead*)b2;
		
		pageHead2->n = pageHead1->n / 2;
		pageHead2->pageType = pageHead1->pageType;
		pageHead1->n -= pageHead2->n +1;
		memcpy(b2+sizeof(PageHead),getValue(b1,pageHead1->n),pageHead2->n * (sizeof(int)+indexSize)+sizeof(int));
		//*getValue(b1,pageHead1->n)= pageID2;
		return true;
	}

	void adjustKey(int pageID, char* sm_key, int subIdx)
	{
		if (subIdx < 0 || subIdx > indexPerPage)return;
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);

		if (subIdx == 0) {
		}
		else if (subIdx == 1)
		{
			bufPageManager->markDirty(index);
			char* tkey = getKey(b, subIdx - 1);
			memcpy(tkey, sm_key, indexSize);
			PageHead* pageHead = (PageHead*)b;
			int fatherPageID = pageHead->fatherPage;
			if (fatherPageID > 0) {
				int fatherPageIndex;
				BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
				PageHead* fatherPageHead = (PageHead*)fatherPageBuf;
				int keyNum = fatherPageHead->n;
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
			bufPageManager->markDirty(index);
		}
		return;
	}

	void leafMergeAndDelete(int pageID, int subIdx, void* targetKey, int& recordID) {
		if (pageID < 0 || pageID > pageNum - 1)return;

		deleteEntry(pageID, subIdx, targetKey, recordID);

		int index;
		BufType pageBuf = bufPageManager->getPage(fileID, pageID, index);
		PageHead* pageHead = (PageHead*)pageBuf;
		int fatherPageID = pageHead->fatherPage;
		int fatherPageIndex;
		BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
		PageHead* fatherPageHead = (PageHead*)fatherPageBuf;
		int fatherKeyNum = fatherPageHead->n;

		if (subIdx == fatherKeyNum) {
			int leftBrotherPageID = getValue(fatherPageBuf, subIdx - 1)[0];
			int leftBrotherIndex;
			BufType leftBrotherPageBuf = bufPageManager->getPage(fileID, leftBrotherPageID, leftBrotherIndex);
			PageHead* leftBrotherPageHead = (PageHead*)leftBrotherPageBuf;
			for (int i = leftBrotherPageHead->n, j = 0; j < pageHead->n; i++, j++) {
				memcpy((char*)getValue(leftBrotherPageBuf, i), (char*)getValue(pageBuf, j), sizeof(int));
				memcpy((char*)getKey(leftBrotherPageBuf, i), (char*)getKey(pageBuf, j), indexSize);
				leftBrotherPageHead->n++;
			}
			fatherPageHead->n--;
			*getKey(fatherPageBuf, fatherPageHead->n) = NULL;
			*getValue(fatherPageBuf, fatherPageHead->n + 1) = 0;
		}
		else {
			int rightBrotherPageID = getValue(fatherPageBuf, subIdx + 1)[0];
			int rightBrotherIndex;
			BufType rightBrotherPageBuf = bufPageManager->getPage(fileID, rightBrotherPageID, rightBrotherIndex);
			PageHead* rightBrotherPageHead = (PageHead*)rightBrotherPageBuf;
			for (int i = pageHead->n, j = 0; j < rightBrotherPageHead->n; i++, j++) {
				memcpy((char*)getValue(pageBuf, i), (char*)getValue(rightBrotherPageBuf, j), sizeof(int));
				memcpy((char*)getKey(pageBuf, i), (char*)getKey(rightBrotherPageBuf, j), indexSize);
				pageHead->n++;
			}
			memcpy(getKey(fatherPageBuf, subIdx), getKey(fatherPageBuf, subIdx + 1), (fatherPageHead->n - subIdx - 1)*(sizeof(int) + indexSize));
			fatherPageHead->n--;
			*getKey(fatherPageBuf, fatherPageHead->n) = NULL;
			*getValue(fatherPageBuf, fatherPageHead->n + 1) = 0;
		}

		if (fatherPageHead->n < indexPerPage / 2) {
			mergeOrRotate(fatherPageID);
		}
		return;

	}

	bool checkBrotherIsRich(int pageID, int subIdx, bool* isLeft) {
		
		if (pageID < 0 || pageID > pageNum - 1) return false;
		
		int index;
		BufType pageBuf = bufPageManager->getPage(fileID, pageID, index);
		PageHead* pageHead = (PageHead*)pageBuf;
		int fatherPageID = pageHead->fatherPage;
		
		if (fatherPageID < 0)return false;

		int fatherPageIndex;
		BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
		PageHead* fatherPageHead = (PageHead*)fatherPageBuf;

		if (subIdx == 0) {
			int rightBrotherPageID = getValue(fatherPageBuf, subIdx + 1)[0];
			int rightBrotherIndex;
			BufType rightBrotherPageBuf = bufPageManager->getPage(fileID, rightBrotherPageID, rightBrotherIndex);
			PageHead* rightBrotherPageHead = (PageHead*)rightBrotherPageBuf;
			
			if (rightBrotherPageHead->n > indexPerPage / 2) {
				*isLeft = false;
				return true;
			}
			else
				return false;
		}
		else if (subIdx == fatherPageHead->n) {
			int leftBrotherPageID = getValue(fatherPageBuf, subIdx - 1)[0];
			int leftBrotherIndex;
			BufType leftBrotherPageBuf = bufPageManager->getPage(fileID, leftBrotherPageID, leftBrotherIndex);
			PageHead* leftBrotherPageHead = (PageHead*)leftBrotherPageBuf;

			if (leftBrotherPageHead->n > indexPerPage / 2) {
				*isLeft = true;
				return true;
			}
			else
				return false;
		}
		else {
			int leftBrotherPageID = getValue(fatherPageBuf, subIdx - 1)[0];
			int leftBrotherIndex;
			BufType leftBrotherPageBuf = bufPageManager->getPage(fileID, leftBrotherPageID, leftBrotherIndex);
			PageHead* leftBrotherPageHead = (PageHead*)leftBrotherPageBuf;

			if (leftBrotherPageHead->n > indexPerPage / 2) {
				*isLeft = true;
				return true;
			}

			int rightBrotherPageID = getValue(fatherPageBuf, subIdx + 1)[0];
			int rightBrotherIndex;
			BufType rightBrotherPageBuf = bufPageManager->getPage(fileID, rightBrotherPageID, rightBrotherIndex);
			PageHead* rightBrotherPageHead = (PageHead*)rightBrotherPageBuf;

			if (rightBrotherPageHead->n > indexPerPage / 2) {
				*isLeft = false;
				return true;
			}
			return false;
		}
		return false;
	}

	char* getSMKey(int pageID) {
		int index;
		BufType pageBuf = bufPageManager->getPage(fileID, pageID, index);
		return getKey(pageBuf, 0);
	}

	void mergeOrRotate(int pageID) {

		if (pageID < 0 || pageID > pageNum - 1) return;

		int index;
		BufType pageBuf = bufPageManager->getPage(fileID, pageID, index);
		PageHead* pageHead = (PageHead*)pageBuf;
		int fatherPageID = pageHead->fatherPage;
		//根节点
		if (fatherPageID < 0)return;
		//叶节点
		if (pageHead->pageType == LEAF)return;
		//已满
		if (pageHead->n >= indexPerPage)return;

		int fatherPageIndex;
		BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
		PageHead* fatherPageHead = (PageHead*)fatherPageBuf;
		int fatherKeyNum = fatherPageHead->n;

		int subIndex = 0;
		for (; subIndex <= fatherKeyNum; subIndex++) {
			if (getValue(fatherPageBuf, subIndex)[0] == pageID)
				break;
		}

		bool isLeft = false;
		if (checkBrotherIsRich(pageID, subIndex, &isLeft)) {
			if (isLeft) {

				int leftBrotherPageID = getValue(fatherPageBuf, subIndex - 1)[0];
				int leftBrotherIndex;
				BufType leftBrotherPageBuf = bufPageManager->getPage(fileID, leftBrotherPageID, leftBrotherIndex);
				PageHead* leftBrotherPageHead = (PageHead*)leftBrotherPageBuf;
				
				/* 将左兄弟的最右的分支借过来(右旋操作) */
				memcpy(getValue(pageBuf, 1), getValue(pageBuf, 0), pageHead->n*(VALUE_SIZE + indexSize)+VALUE_SIZE);
				memcpy(getKey(pageBuf, 0), getSMKey(getValue(pageBuf, 1)[0]), indexSize);

				memcpy(getValue(pageBuf, 0), getValue(leftBrotherBuf, leftBrotherPageHead->n), VALUE_SIZE);
				
				int leftChildPageID = getValue(pageBuf, 0)[0];
				int leftChildIndex;
				BufType leftChildPageBuf = bufPageManager->getPage(fileID, leftChildPageID, leftChildIndex);
				PageHead* leftChildPageHead = (PageHead*)leftChildPageBuf;
				leftChildPageHead->fatherPage = pageID;

				pageHead->n++;
				leftBrotherPageHead->n--;
				*getKey(leftBrotherPageBuf, leftBrotherPageHead->n) = NULL;
				*getValue(leftBrotherPageBuf, leftBrotherPageHead->n + 1) = 0;

				bufPageManager->markDirty(leftChildIndex);
				bufPageManager->markDirty(index);
				bufPageManager->markDirty(leftBrotherIndex);

				adjustKey(fatherPageID, getKey(pageBuf, 0), subIndex);
			}
			else {
				int rightBrotherPageID = getValue(fatherPageBuf, subIndex + 1)[0];
				int rightBrotherIndex;
				BufType rightBrotherPageBuf = bufPageManager->getPage(fileID, rightBrotherPageID, rightBrotherIndex);
				PageHead* rightBrotherPageHead = (PageHead*)rightBrotherPageBuf;

				memcpy(getKey(pageBuf, pageHead->n), getSMKey(getValue(rightBrotherPageBuf, 0)[0]), indexSize);
				memcpy(getValue(pageBuf, pageHead->n + 1), getValue(rightBrotherPageBuf, 0), VALUE_SIZE);

				memcpy(getValue(rightBrotherPageBuf, 0), getValue(rightBrotherPageBuf, 1), (rightBrotherPageHead->n - 1)*(indexSize + VALUE_SIZE)+VALUE_SIZE);

				int rightChildPageID = getValue(pageBuf, pageHead->n+1)[0];
				int rightChildIndex;
				BufType rightChildPageBuf = bufPageManager->getPage(fileID, rightChildPageID, rightChildIndex);
				PageHead* rightChildPageHead = (PageHead*)rightChildPageBuf;
				rightChildPageHead->fatherPage = pageID;

				pageHead->n++;
				rightBrotherPageHead->n--;
				*getKey(rightBrotherPageBuf, rightBrotherPageHead->n) = NULL;
				*getValue(rightBrotherPageBuf, rightBrotherPageHead->n + 1) = 0;

				bufPageManager->markDirty(rightChildIndex);
				bufPageManager->markDirty(index);
				bufPageManager->markDirty(rightBrotherIndex);

				adjustKey(fatherPageID, getKey(rightBrotherPageBuf, 0), subIndex+1);
			}
		}
		else {
			int leftPageID = -1, rightPageID = -1;
			int keepID = 0;
			if (subIndex == 0) {
				leftPageID = pageID;
				rightPageID = getValue(fatherPageBuf, subIndex + 1)[0];
				keepID = subIndex;
			}
			else {
				leftPageID = getValue(fatherPageBuf, subIndex - 1)[0];
				rightPageID = pageID;
				keepID = subIndex - 1;
			}

			int rightPageIndex;
			BufType rightPageBuf = bufPageManager->getPage(fileID, rightPageID, rightPageIndex);
			PageHead* rightPageHead = (PageHead*)rightPageBuf;
			int rightPageKeyNum = rightPageHead->n;

			int leftPageIndex;
			BufType leftPageBuf = bufPageManager->getPage(fileID, leftPageID, leftPageIndex);
			PageHead* leftPageHead = (PageHead*)leftPageBuf;
			int leftPageKeyNum = leftPageHead->n;

			memcpy(getKey(leftPageBuf, leftPageKeyNum), getSMKey(getValue(rightPageBuf, 0)[0]), indexSize);
			memcpy((char*)getValue(leftPageBuf, leftPageKeyNum + 1), (char*)getValue(rightPageBuf, 0), rightPageKeyNum*(indexSize + VALUE_SIZE) + VALUE_SIZE);

			leftPageHead->n += rightPageHead + 1;

			memcpy(getKey(fatherPageBuf, subIndex), getKey(fatherPageBuf, subIndex + 1), (fatherPageHead->n - subIndex - 1)*(indexSize + VALUE_SIZE));

			fatherPageHead->n--;
			bufPageManager->markDirty(fatherPageIndex);
			if (fatherPageHead->n < indexPerPage / 2) {
				mergeOrRotate(fatherPageID);
			}
		}
		return;
	}

	void deleteEntry(int pageID, int subIdx, void* targetKey, int& recordID) {

		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		PageHead* pageHead = (PageHead*)b;
		int keyNum = pageHead->n;
		int fatherPageID = pageHead->fatherPage;

		if (pageHead->pageType == NODE)return;

		char* oldSmKey = getKey(b, 0);
		for (int i = 0; i < keyNum; i++) {
			char* tkey = getKey(b, i);
			if (equal2Key(targetKey, tkey)) {
				BufType targetValueAddr = getValue(b, i);
				recordID = targetValueAddr[0];
				memcpy((char*)targetValueAddr, (char*)getValue(b, i + 1), (keyNum - 1 - i) * (sizeof(int) + indexSize));
				pageHead->n--;
				bufPageManager->markDirty(index);
			}
		}
		if (!equal2Key(oldSmKey, getKey(b, 0))) {
			adjustKey(fatherPageID, getKey(b, 0), subIdx);
		}

	}
	
	bool deleteNode(void* targetKey, int& recordID) {
		
		int subIndex = -1;
		int targetPageID = _searchNode(root, targetKey, subIndex);

		int index;
		BufType pageBuf = bufPageManager->getPage(fileID, targetPageID, index);
		PageHead* pageHead = (PageHead*)pageBuf;
		int fatherPageID = pageHead->fatherPage;

		if (pageHead->n > indexPerPage /2) {
			deleteEntry(targetPageID, subIndex, targetKey, recordID);
			return true;
		}
		else {
			int fatherPageIndex;
			BufType fatherPageBuf = bufPageManager->getPage(fileID, fatherPageID, fatherPageIndex);
			PageHead* fatherPageHead = (PageHead*)fatherPageBuf;
			int fatherKeyNum = fatherPageHead->n;

			bool isLeft = false;
			if (checkBrotherIsRich(targetPageID, subIndex, &isLeft)) {
				if (isLeft) {
					int leftBrotherPageID = getValue(fatherPageBuf, subIndex - 1)[0];
					int leftBrotherIndex;
					BufType leftBrotherPageBuf = bufPageManager->getPage(fileID, leftBrotherPageID, leftBrotherIndex);
					PageHead* leftBrotherPageHead = (PageHead*)leftBrotherPageBuf;

					char* borrowKey = getKey(leftBrotherPageBuf, leftBrotherPageHead->n - 1);
					BufType borrowValue = getValue(leftBrotherPageBuf, leftBrotherPageHead->n - 1);

					memcpy(getKey(fatherPageBuf, subIndex - 1), borrowKey, indexSize);
					*getKey(leftBrotherPageBuf, leftBrotherPageHead->n-1) = NULL;
					*getValue(leftBrotherPageBuf, leftBrotherPageHead->n-1) = 0;
					leftBrotherPageHead->n--;

					memcpy((char*)getValue(pageBuf, 1), (char*)getValue(pageBuf, 0), pageHead->n*(indexSize + VALUE_SIZE));
					memcpy((char*)getValue(pageBuf, 0),(char*)borrowValue, VALUE_SIZE);
					memcpy(getKey(pageBuf, 0), borrowKey, indexSize);
					pageHead->n++;

					bufPageManager->markDirty(leftBrotherIndex);
					bufPageManager->markDirty(index);
					bufPageManager->markDirty(fatherPageIndex);

					deleteEntry(targetPageID, subIndex, targetKey, recordID);
					return true;

				}
				else {
					int rightBrotherPageID = getValue(fatherPageBuf, subIndex + 1)[0];
					int rightBrotherIndex;
					BufType rightBrotherPageBuf = bufPageManager->getPage(fileID, rightBrotherPageID, rightBrotherIndex);
					PageHead* rightBrotherPageHead = (PageHead*)rightBrotherPageBuf;

					char* borrowKey = getKey(rightBrotherPageBuf, 0);
					BufType borrowValue = getValue(rightBrotherPageBuf, 0);

					memcpy(getKey(fatherPageBuf, subIndex), getKey(rightBrotherPageBuf, 1), indexSize);
					memcpy((char*)getValue(rightBrotherPageBuf, 0), (char*)getValue(rightBrotherPageBuf, 1), (rightBrotherPageHead->n - 1)*(indexSize + VALUE_SIZE));
					rightBrotherPageHead->n--;

					memcpy(getKey(pageBuf, pageHead->n), borrowKey, indexSize);
					memcpy(getValue(pageBuf, pageHead->n), borrowKey, VALUE_SIZE);
					pageHead->n++;

					bufPageManager->markDirty(rightBrotherIndex);
					bufPageManager->markDirty(index);
					bufPageManager->markDirty(fatherPageIndex);

					deleteEntry(targetPageID, subIndex, targetKey, recordID);
					return true;
				}
			}
			else {
				leafMergeAndDelete(targetPageID, subIndex, targetKey, recordID);
				return true;
			}
		}
		return false;
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
