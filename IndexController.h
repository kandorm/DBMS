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
public:
	IndexController(BufPageManager* bpm, int fID) {
		bufPageManager = bpm;
		fileID = fID;
		indexFileHead = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		bufPageManager->access(fileHeadIndex);

	}
	int insertNode(int fileID, void* data, int recordID) {
		if (indexFileHead->root == -1) {
			indexFileHead->root = allocIndexPage(fileID);
			int index;
			BufType b = bufPageManager->getPage(fileID, IndexFileHead->root, index);
			bufPageManager->markDirty(index);
			PageHead* pageHead = (pageHead*)b;

		}
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
