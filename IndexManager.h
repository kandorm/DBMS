#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include <string.h>

enum IndexType {
	INT,
	CHAR
};

enum Status {
	NOTHING,
	SPLIT,
	MERGE
};

enum PageType {
	NODE,
	LEAF
}

typedef struct IndexFileHead {
	IndexType indexType;
	int indexSize;
	int firstLeafNode;
	int root;
	int pageNum;
	int firstFreePage;
};

struct PageHead {
	PageType pageType;
	int n;
}

class IndexManager {
private:
	BufPageManager* bufPageManager;
	
	int allocIndexPage(int fileID) {
		BufType c = bufPageManager->getPage(fileID,0,fhindex);
		int fep = IndexFileHead->firstFreePage;
		int fhindex;
		
		IndexFileHead->firstFreePage++;
		IndexFileHead->pageNum++;
		bufPageManager->markDirty(fhindex);
		return fep;
	}
	
	int fillData(BufType b,char* key,int recordID,int i) {
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int)),&recordID,sizeof(int));
		memcpy(b+sizeof(IndexFileHead)+i*(indexSize+sizeof(int))+sizeof(int),key,indexSize);
	}

public:

	IndexManager(BufPageManager* bpm) {
		bufPageManager = bpm;
	}
	
	bool createIndex(const char* name, IndexType idxType, int idxSize) {

		//��ֹ��ҳ�洢
		if (idxSize+sizeof(int)+sizeof(int) > PAGE_SIZE)return false;

		if (bufPageManager->fileManager->createFile(name)) {
			int fileID;
			if (bufPageManager->fileManager->openFile(name, fileID)) {

				IndexFileHead indexFileHead;
				indexFileHead.indexType = idxType;
				indexFileHead.indexSize = idxSize;
				indexFileHead.firstLeafNode = -1;
				indexFileHead.root = -1;
				indexFileHead.pageNum = 1;
				indexFileHead.firstFreePage = 1;

				int fileHeadIndex;
				BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);
				memcpy(b, &filehead, sizeof(IndexFileHead));
				bufPageManager->markDirty(fileHeadIndex);

			}
		}
		return false;
	}

	bool deleteIndex(const char* name) {
		if (remove(name) == 0)
			return true;
		return false;
	}

	bool openIndex(const char* name, int& fileID) {
		return bufPageManager->fileManager->openFile(name, fileID);
	}

	int closeIndex(int fileID) {
		bufPageManager->close();
		bufPageManager->fileManager->closeFile(fileID);
		return 0;
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
			if(insertBPlus(IndexFileHead->root) == SPLIT) {
				int newPage = allocIndexPage();
				splitPage(IndexFileHead->root,newPage);
				int newRoot = allocIndexPage();
			}
		}
	}
};
