#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
#define PAGE_BEFORE_RECORD 1

//每隔4096页设置一ManagerPage，ManagerPage中每2个byte代表某页空闲空间
//故可以将这4097页划分为一个页区,依次进行区域管理
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//创建文件后对文件进行初始化（页头+第一个FreePage）
	bool initFile(const char* name, int[] colSize, int colNum) {
		
		//记录的长度(标志位+记录)
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		recordSize += sizeof(int);

		//禁止跨页存储
		if (recordSize > PAGE_SIZE)return false;

		//初始化页头（即该文件中记录的信息）
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//文件中记录长度
		b[1] = 0;														//文件中记录总个数
		b[2] = 1;														//文件中页区的个数
		b[3] = colNum;													//表的列数
		for (int i = 4; i < colNum + 4; i++)							//表每一列的size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(index);
		//对紧跟头文件后的FreePage进行初始化
		if (initFreePageManagerPage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(index);
			bufPageManager->fileManager->closeFile(fileID);
		}
		return true;
	}
	bool initFreePageManagerPage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[i] = recordNumPerPage<<16 + recordNumPerPage;
		bufPageManager->markDirty(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, int recordSize, unsigned char* record) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);
		int recordPerPage = PAGE_SIZE / recordSize;
		for (int i = 0; i < recordPerPage; i++) {
			b = b + i * recordSize;
			if (b[0] == -1) {
				memcpy(b, record, recordSize);
				break;
			}
		}
		bufPageManager->markDirty(index);
		return true;
	}
	int _recordIDtoPageID(int recordID, int recordSize) {
		int recordPerPage = PAGE_SIZE / recordSize;
		int onlyRecordPageIndex = recordID / recordPerPage;
		int managerPageNum = onlyRecordPageIndex / PAGE_AREA_RANGE + 1;

		return PAGE_BEFORE_RECORD + onlyRecordPageIndex + managerPageNum;
	}
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
	bool insertRecord(int fileID, int recordSize, unsigned char* record) {
		int headIndex;
		BufType b = bufPageManager->getPage(fileID, 0, headIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int pageAreaNum = b[2];			//文件中页区个数
		
		for (int i = 0; i < pageAreaNum; i++) {
			int pageID = 1 + i * PAGE_AREA_RANGE_HEAD;
			int freePageIndex;
			BufType bFreePage = bufPageManager->getPage(fileID, pageID, freePageIndex);
			for (int j = 0; j < PAGE_INT_NUM; j++) {
				if (bFreePage[j] != 0) {
					int recordPageID;
					if (bFreePage[j] >> 16 != 0)
						recordPageID = pageID + 2 * (j+1) - 1;
					else 
						recordPageID = pageID + 2 * (j + 1);
					if (_insertRecord(fileID, recordPageID, recordSize, record)) {
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
		if (_insertRecord(fileID, newFreePageID + 1, recordSize, record)) {
			//文件头记录数加1
			b[2] += 1;
			bufPageManager->markDirty(headIndex);
		}
		return true;


	}
	bool deleteRecord(int fileID, int recordID, int recordSize) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);
		
		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		int newRecordID = -1;
		memcpy(recordPageBuf[recordIndex*recordSize], &newRecordID, sizeof(int));
		bufPageManager->markDirty(recordPageIndex);
		return true;


	}
	bool updateRecord(int fileID, int recordID, int recordSize, unsigned char* record) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		memcpy(recordPageBuf[recordIndex*recordSize], record, recordSize);
		bufPageManager->markDirty(recordPageIndex);
		return true;

	}
	char* queryRecord(int fileID, int recordID, int recordSize) {

		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		char* record;
		memcpy(record, recordPageBuf[recordIndex*recordSize], recordSize);
		return record;
	}
	RecordManager(BufPageManager bpm) {
		bufPageManager = bpm;
	}
}
