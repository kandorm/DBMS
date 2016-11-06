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

	int allocIndexPage() {
		indexFileHead->firstFreePage++;
		indexFileHead->pageNum++;
		bufPageManager->markDirty(fhindex);
		return fep;
	}
	
	int fillData(BufType b,char* key,int recordID,int i) {
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int)),&recordID,sizeof(int));
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int))+sizeof(int),key,indexSize);
	}
	
	int getP(BufType b,int i) {
		return b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int));
	} 
	
	char* getK(BufType b,int i) {
		return b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int))+sizeof(int);
	}
	
	char* getMaxK(int fileID,int pageID) {
		int index;
		BufType b = bufPageManager->getPage(fileID,pageID,index);
		//bufPageManager->markDirty(index);
		(PageHead*) pageHead = (PageHead*)b;
		if(b->pageType == LEAF)
			return getK(b,pageHead->n-1);
		else
			return getK(b,pageHead->n);
	}
public:
	IndexController(BufPageManager* bpm, int fID) {
		bufPageManager = bpm;
		fileID = fID;
		indexFileHead = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		bufPageManager->access(fileHeadIndex);

	}
	
	int insertNode(int fileID,char* key,int recordID) {
		//markDirty
		if(IndexFileHead->root == -1) {
			IndexFileHead->root = allocIndexPage(fileID);
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
				BufType p = getP(b,1);
				(*p) = newPage;
				fillData(b,getMaxK(fileID,indexFileHead->root),indexFileHead->root,0);
				indexFileHead->root = newRoot;
			}
		}
	}
	
	int insertBPlus(int fileID,int pageID,char* key,int recordID) {
		int index;
		BufType b = bufPageManager->getPage(fileID,pageID,index);
		
	}

	int searchNode(int pageID) {
		int index;
		char* b = bufPageManager->getPage(fileID, pageID, index);
		bufPageManager->access(index);
		PageHead* pageHead = (PageHead*)b;
		int indexNum = pageHead->n;
		switch (pageHead->pageType)
		{
		case NODE:
			for (int i = 0; i < indexNum; i++) {

			}
			break;
		case LEAF:
			break;
		default:
			break;
		}
	}
	
	

};
#endif // !INDEX_CONTROLLER
