#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
//每隔4096页设置一FreePage，FreePage中每2个byte代表某页空闲空间
//故可以将这4097页划分为一个页区
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//创建文件后对文件进行初始化（页头+第一个FreePage）
	bool initFile(const char* name, int[] colSize, int colNum) {
		//记录的长度
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		//每一页记录最大数目
		int recordNumPerPage = PAGE_SIZE / recordSize;
		//禁止跨页存储
		if (recordSize > PAGE_SIZE)return false;
		//初始化页头（即该文件中记录的信息）
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//文件中记录长度
		b[1] = recordNumPerPage;									//文件每一页可以存储最多记录数
		b[2] = 0;														//文件中记录总个数
		b[3] = 1;														//文件中页区的个数
		b[4] = colNum;													//表的列数
		for (int i = 5; i < colNum + 5; i++)							//表每一列的size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(index);
		//对紧跟头文件后的FreePage进行初始化
		if (initFreePage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(index);
			bufPageManager->fileManager->closeFile(fileID);
		}
	}
	bool initFreePage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[0] = recordNumPerPage<<16 + recordNumPerPage;
		bufPageManager->markDirty(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, unsigned char* record) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
	}
	int 
public:
	//File Operation
	bool createFile(const char* name, int[] colSize, int colNum) {
		if (bufPageManager->fileManager->createFile(name)) {
			if (initFile(name, colSize, colNum))
				return true;
			remove(name);
			return false;
		}
		return false;
	}
	bool deleteFile(const char* name) {
		if (remove(name) == 0)
			return true;
		return false;
	}
	bool openFile(const char* name, int& fileID) {
		return bufPageManager->fileManager->openFile(name, fileID);
	}
	int closeFile(int fileID) {
		return bufPageManager->fileManager->closeFile(fileID);
	}

	//Record Operation
	bool insertRecord(int fileID, unsigned char* record) {
		int headIndex;
		BufType b = bufPageManager->getPage(fileID, 0, headIndex);
		int recordSize = b[0];			//该文件中记录的长度（记录定长，最长UNSIGNED_MAX_INT）
		int recordNumPerPage = b[1];	//文件每一页可以存储最多记录数
		int pageAreaNum = b[3];			//文件中页区个数
		
		for (int i = 0; i < pageAreaNum; i++) {
			int pageID = 1 + pageAreaNum * PAGE_AREA_RANGE_HEAD;
			int freePageIndex;
			BufType bFreePage = bufPageManager->getPage(fileID, pageID, freePageIndex);
			for (int j = 0; j < PAGE_INT_NUM; j++) {
				if (bFreePage[j] != 0) {
					int recordPageID;
					if (bFreePage[j] >> 16 != 0)
						recordPageID = pageID + 2 * (j+1) - 1;
					else 
						recordPageID = pageID + 2 * (j + 1);
					if (_insertRecord(fileID, recordPageID, record)) {
						//文件头记录数加1
						b[2] += 1;
						bufPageManager->markDirty(headIndex);
						//添加记录页对应的FreePage中该页空闲空间减1
						if (bFreePage[j] >> 16 != 0)
							bFreePage[j] -= 1<<16;
						else
							bFreePage[j] -= 1;
						bufPageManager->markDirty(freePageIndex);
						return true;
					}
					return false;
				}
			}
		}
		//现存页区已满
		int newFreePageID = 1 + pageAreaNum * PAGE_AREA_RANGE_HEAD;
		initFreePage(fileID, newFreePageID, recordNumPerPage);
		if (_insertRecord(fileID, newFreePageID + 1, record)) {
			//文件头记录数加1
			b[2] += 1;
			bufPageManager->markDirty(headIndex);
		}


	}
	bool deleteRecord(int fileID, int pageID, int recordID) {
		int fileHeadIndex;
		int recordSize;
		BufType fileHeadBuf = bufPageManager->getPage(fileID, 0, fileHeadIndex);
		recordSize = fileHeadBuf[0];
		
		BufType buf = bufPageManager->getPage(fileID, pageID, index);

	}
	bool updateRecord() {

	}
	int** getRecord(int fileID, unsigned char* record, int[] condCol) {

	}
	RecordManager() {
		MyBitMap::initConst();
		FileManager* fm = new FileManager();
		bufPageManager = new BufPageManager(fm);
	}
}
