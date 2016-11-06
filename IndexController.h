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


	BufType getValue(BufType b, int i) {
		return b + sizeof(PageHead) + i*(sizeof(int) + indexSize);
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
		pageHead1->n = (pageHead1->n - pageHead2->n +1);
		memcpy(b2+sizeof(PageHead),getValue(b1,pageHead1->n),pageHead2->n * (sizeof(int)+indexSize)+sizeof(int));
		//*getValue(b1,pageHead1->n)= pageID2;
		return true;
	}

	bool deleteNode() {

	}

	bool searchNode(int pageID, void* targetKey, int& recordID) {
		
		if (pageID < 1 || pageID > pageNum - 1)return false;

		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;
		PageType pageType = pageHead->pageType;
		int keyNum = pageHead->n;
		switch (pageType)
		{
		case NODE:
			if (greaterThanKey(targetKey, getKey(b, keyNum - 1))) {
				int afterKeyPage = getValue(b, nodeNum)[0];
				return searchNode(afterKeyPage, targetKey, recordID);
			}
			else {
				for (int i = 0; i < keyNum; i++) {
					char* tkey = getKey(b, i);
					if (lessThanKey(targetKey, tkey)) {
						int beforeKeyPage = getValue(b, i)[0];
						return searchNode(beforeKeyPage, targetKey, recordID);
					}
				}
			}
			break;
		case LEAF:
			for (int i = 0; i < keyNum; i++) {
				char* tkey = getKey(b, i);
				if (equal2Key(targetKey, tkey)) {
					recordID = getValue(b, i)[0];
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
