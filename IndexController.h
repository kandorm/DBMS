#ifndef INDEX_CONTROLLER
#define INDEX_CONTROLLER

#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include <string.h>

enum Status {
	NOTHING,
	SPLIT,
	MERGE
};

enum PageType {
	NODE,
	LEAF
}

struct PageHead {
	PageType pageType;
	int n;
}

class IndexController {
private:
	BufPageManager* bufPageManager;
	int fileID;

	IndexType indexType;
	int indexSize;
	int firstLeafNode;
	int root;
	int pageNum;
	int firstFreePage;

	int allocIndexPage(int fileID) {
		int fhindex;
		IndexFileHead* indexFileHead = bufPageManager->getPage(fileID, 0, fhindex);
		int fep = indexFileHead->firstFreePage;
		indexFileHead->firstFreePage++;
		indexFileHead->pageNum++;
		bufPageManager->markDirty(fhindex);
		return fep;
	}
public:
	IndexController(BufPageManager* bpm, int fID) {
		bufPageManager = bpm;
		fileID = fID;
	}
	int insertNode(int fileID, void* data, int recordID) {
		if (IndexFileHead->root == -1) {
			IndexFileHead->root = allocIndexPage(fileID);
			int index;
			BufType b = bufPageManager->getPage(fileID, IndexFileHead->root, index);
			bufPageManager->markDirty(index);
			PageHead* pageHead = (pageHead*)b;

		}
	}

	int searchNode(int fileID, int pageID) {
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
