#ifndef INDEX_MANAGER
#define INDEX_MANAGER

#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include "IndexController.h"
#include "IndexCommon.h"
#include <string.h>

class IndexManager {
private:
	BufPageManager* bufPageManager;

public:

	IndexManager(BufPageManager* bpm) {
		bufPageManager = bpm;
	}

	bool createIndex(const char* name, IndexType idxType, int idxSize) {

		//½ûÖ¹¿çÒ³´æ´¢
		if (idxSize + sizeof(int) + sizeof(int) > PAGE_SIZE)return false;

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

				bufPageManager->close();
				bufPageManager->fileManager->closeFile(fileID);
			}
		}
		return false;
	}

	bool deleteIndex(const char* name) {
		if (remove(name) == 0)
			return true;
		return false;
	}

	bool openIndex(const char* name, IndexController& idxController) {
		int fileID;
		bufPageManager->fileManager->openFile(name, fileID);
		IndexController indexController(bufPageManager, fileID);
		idxController = indexController;
		return true;
	}

	int closeIndex(int fileID) {
		bufPageManager->close();
		bufPageManager->fileManager->closeFile(fileID);
		return 0;
	}
};
#endif // !INDEX_MANAGER
