#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/utils/pagedef.h"
#include <string.h>

#define PAGE_AREA_RANGE 4096
#define PAGE_AREA_RANGE_HEAD 4097
#define PAGE_BEFORE_RECORD 1

//每隔4096页设置一ManagerPage，ManagerPage中每2个byte代表某页空闲空间
//故可以将这4097页划分为一个页区,依次进行区域管理
class RecordManager {
private:
	BufPageManager* bufPageManager;
	//创建文件后对文件进行初始化（页头+第一个FreePage）
	bool initFile(const char* name, int colSize[], int colNum) {
		
		//记录的长度(标志位+记录)
		int recordSize = 0;
		for (int i = 0; i < colNum; i++)
			recordSize += colSize[i];
		recordSize += sizeof(int)+1;

		//禁止跨页存储
		if (recordSize > PAGE_SIZE)return false;

		//初始化页头（即该文件中记录的信息）
		int fileID;
		int fileHeadIndex;
		bufPageManager->fileManager->openFile(name, fileID);
		BufType b = bufPageManager->allocPage(fileID, 0, fileHeadIndex, false);	//SIZE=PAGE_INT_NUM=2048
		b[0] = recordSize;												//文件中记录长度
		b[1] = -1;														//文件中最大RecordID
		b[2] = 1;														//文件中页区的个数
		b[3] = colNum;													//表的列数
		for (int i = 4; i < colNum + 4; i++)							//表每一列的size
			b[i] = colSize[i - 4];
		bufPageManager->markDirty(fileHeadIndex);
		//对紧跟头文件后的FreePage进行初始化
		int recordNumPerPage = PAGE_SIZE / recordSize;
		if (initManagerPage(fileID, 1, recordNumPerPage)) {
			bufPageManager->writeBack(fileHeadIndex);
			bufPageManager->fileManager->closeFile(fileID);
		}
		return true;
	}
	bool initManagerPage(int fileID, int pageID, int recordNumPerPage) {
		int index;
		BufType b = bufPageManager->allocPage(fileID, pageID, index, false);
		for (int i = 0; i < PAGE_INT_NUM; i++)
			b[i] = recordNumPerPage<<16 | recordNumPerPage;
		
		bufPageManager->markDirty(index);
		bufPageManager->writeBack(index);
		return true;
	}
	bool _insertRecord(int fileID, int pageID, int recordSize, const char* record, int recordFreeNum, int& recordID) {
		int index;
		BufType b = bufPageManager->getPage(fileID, pageID, index);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordNum = recordPerPage - recordFreeNum;
		int baseRecordID = _pageID2BaseRecordID(pageID,recordSize);

		for (int i = 0; i < recordNum; i++) {
			b = b + i * recordSize;
			
			if (b[0] == -1) {
				recordID = baseRecordID;
				memcpy(b, &baseRecordID, sizeof(int));
				memcpy(b+sizeof(int), record, (recordSize-sizeof(int)));
			}
			baseRecordID++;
		}

		recordID = baseRecordID;
		if (recordNum == 0) {
			memcpy(b, &baseRecordID, sizeof(int));
			memcpy(b + sizeof(int), record, (recordSize - sizeof(int)));
		}
		else {
			b += recordSize;
			memcpy(b, &baseRecordID, sizeof(int));
			memcpy(b + sizeof(int), record, (recordSize - sizeof(int)));
		}
		int headIndex;
		BufType headBuf = bufPageManager->getPage(fileID, 0, headIndex);
		headBuf[1] = headBuf[1] < recordID ? recordID : headBuf[1];
		bufPageManager->markDirty(headIndex);
		bufPageManager->markDirty(index);
		return true;
	}
	int _recordIDtoPageID(int recordID, int recordSize) {
		int recordPerPage = PAGE_SIZE / recordSize;
		int onlyRecordPageIndex = recordID / recordPerPage;
		int managerPageNum = onlyRecordPageIndex / PAGE_AREA_RANGE + 1;

		return PAGE_BEFORE_RECORD + onlyRecordPageIndex + managerPageNum;
	}
	
	int _pageID2BaseRecordID(int pageID, int recordSize)   	{
		int pageWithoutRecord = (pageID-1) / PAGE_AREA_RANGE_HEAD + 1 + PAGE_BEFORE_RECORD;
		int pageWithRecord = pageID - pageWithoutRecord;
		int recordPerPage = PAGE_SIZE / recordSize;
		
		int baseRecordID = pageWithRecord * recordPerPage;
		return baseRecordID;
	}
	
public:
	//File Operation
	bool createFile(const char* name, int colSize[], int colNum) {
		remove(name);
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
		bufPageManager->close();
		bufPageManager->fileManager->closeFile(fileID);
		return 0;
	}

	//Record Operation
	bool insertRecord(int fileID, int recordSize, const char* record, int& recordID) {
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
					int recordFreeNum;
					if (bFreePage[j] >> 16 != 0) {
						recordPageID = pageID + 2 * (j+1) - 1;
						recordFreeNum = bFreePage[j] >> 16;	
					}
					else {
						recordPageID = pageID + 2 * (j + 1);
						recordFreeNum = bFreePage[j];	
					}
					if (_insertRecord(fileID, recordPageID, recordSize, record, recordFreeNum, recordID)) {
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
		initManagerPage(fileID, newFreePageID, recordPerPage);
		if (_insertRecord(fileID, newFreePageID + 1, recordSize, record, recordPerPage, recordID)) {
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
		memcpy(recordPageBuf+recordIndex*recordSize, &newRecordID, sizeof(int));//change
		bufPageManager->markDirty(recordPageIndex);
		return true;


	}
	bool updateRecord(int fileID, int recordID, int recordSize, const char* record) {
		
		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		memcpy(recordPageBuf+recordIndex*recordSize+sizeof(int), record, recordSize-sizeof(int));
		bufPageManager->markDirty(recordPageIndex);
		return true;

	}
	bool queryRecord(int fileID, int recordID, int recordSize, char* record) {

		int headPageIndex;
		BufType headPageBuf = bufPageManager->getPage(fileID, 0, headPageIndex);
		if (recordID > headPageBuf[1])return false;

		int recordPageID = _recordIDtoPageID(recordID, recordSize);
		int recordPageIndex;
		BufType recordPageBuf = bufPageManager->getPage(fileID, recordPageID, recordPageIndex);

		int recordPerPage = PAGE_SIZE / recordSize;
		int recordIndex = recordID % recordPerPage;
		int flag;
		memcpy(&flag, recordPageBuf+recordIndex*recordSize, sizeof(int));
		if (flag == -1)return false;
		memcpy(record, recordPageBuf+recordIndex*recordSize+sizeof(int), recordSize-sizeof(int));
		return true;
	}
	RecordManager(BufPageManager* bpm) {
		bufPageManager = bpm;
	}
};
